#ifndef PTRACE_BVH2_GLSL
#define PTRACE_BVH2_GLSL

#define INCLUDE_FROM_SHADER
#include "shared/data_bvh.h"
#define STATS 1
#include "shared/stats.glsl"

layout(push_constant, scalar) uniform uPushConstant
{
    PC_Trace data;
} pc;

layout(local_size_x = 32, local_size_y_id = 0) in;

#define STACK_SIZE 64
#define DYNAMIC_FETCH_THRESHOLD 20
#define BOTTOM_OF_STACK 0x76543210

shared u32 nextRay[gl_WorkGroupSize.y];

void trace()
{
    u32 stackId;
    i32 traversalStack[STACK_SIZE];
    traversalStack[0] = BOTTOM_OF_STACK;

    STATS_LOCAL_DEF;

    i32 nodeId = BOTTOM_OF_STACK;
    i32 leafId;
    u32 rayId;
    RayDetail rayDetail;

    BVH_TYPE bvh = BVH_TYPE(pc.data.bvhAddress);
    BvhTriangles triangles = BvhTriangles(pc.data.bvhTrianglesAddress);
    BvhTriangleIndices triangleIndices = BvhTriangleIndices(pc.data.bvhTriangleIndicesAddress);

    RayBufferMetadata_ref rayBufMeta = RayBufferMetadata_ref(pc.data.rayBufferMetadataAddress);
    RayBuffer_ref rayBuffer = RayBuffer_ref(pc.data.rayBufferAddress);
    RayTraceResults_ref results = RayTraceResults_ref(pc.data.rayTraceResultAddress);
    RayTraceResult result;

    const u32 rayCount = rayBufMeta.data.rayCount;

    while (true) {
        const bool isTerminated = (nodeId == BOTTOM_OF_STACK);
        const uvec4 ballot = subgroupBallot(isTerminated);
        const u32 terminatedCount = subgroupBallotBitCount(ballot);
        const u32 terminatedId = subgroupBallotExclusiveBitCount(ballot);

        if (isTerminated) {
            STATS_LOCAL_RESET;

            if (terminatedId == 0)
                nextRay[gl_SubgroupID] = atomicAdd(rayBufMeta.data.rayTracedCount, terminatedCount);
            memoryBarrier(gl_ScopeSubgroup, gl_StorageSemanticsShared, gl_SemanticsAcquireRelease);

            rayId = nextRay[gl_SubgroupID] + terminatedId;
            if (rayId >= rayCount)
                break;

            const Ray ray = rayBuffer.ray[rayId];

            rayDetail = initRayDetail(ray);
            result.t = ray.d.w;

            stackId = 0;
            leafId = 0;
            nodeId = 0;

            result.tId = INVALID_ID;
            result.u = -1.f;
            result.v = -1.f;
        }

        while (nodeId != BOTTOM_OF_STACK)
        {
            while (u32(nodeId) < u32(BOTTOM_OF_STACK))
            {
                STATS_NODE_PP;

                const vec2 c0minmax = intersect(bvh.node[nodeId].bv[0], rayDetail, result.t);
                STATS_BV_PP;
                const vec2 c1minmax = intersect(bvh.node[nodeId].bv[1], rayDetail, result.t);
                STATS_BV_PP;

                ivec2 cnodes = ivec2(bvh.node[nodeId].c[0], bvh.node[nodeId].c[1]);
                const bool swp = (c1minmax[0] < c0minmax[0]);
                const bool traverseC0 = (c0minmax[1] >= c0minmax[0]);
                const bool traverseC1 = (c1minmax[1] >= c1minmax[0]);

                if (!traverseC0 && !traverseC1) {
                    nodeId = traversalStack[stackId];
                    --stackId;
                }
                else {
                    nodeId = (traverseC0) ? cnodes.x : cnodes.y;

                    if (traverseC0 && traverseC1) {
                        if (swp) {
                            i32 tmp = nodeId;
                            nodeId = cnodes.y;
                            cnodes.y = tmp;
                        }
                        ++stackId;
                        traversalStack[stackId] = cnodes.y;
                    }
                }

                // postpone one leaf
                if (nodeId < 0 && leafId >= 0) {
                    leafId = nodeId;
                    nodeId = traversalStack[stackId];
                    --stackId;
                }

                if (subgroupAll(leafId < 0))
                    break;
            }

            while (leafId < 0) {
                const i32 triStartId = leafId & 0x07FFFFFF;
                const i32 triCount = ((leafId >> 27) & 0xF) + 1;

                for (i32 triId = triStartId; triId < triStartId + triCount; ++triId) {
                    STATS_TRI_PP;

                    const vec4 v00 = triangles.t[triId].v0;
                    const vec4 v11 = triangles.t[triId].v1;
                    const vec4 v22 = triangles.t[triId].v2;

                    const f32 t = (v00.w - dot(rayDetail.origin, v00.xyz)) / dot(rayDetail.dir, v00.xyz);
                    if (t > rayDetail.tmin && t < result.t) {
                        const f32 u = v11.w + dot(rayDetail.origin, v11.xyz) + t * dot(rayDetail.dir, v11.xyz);
                        if (u >= 0.f) {
                            const f32 v = v22.w + dot(rayDetail.origin, v22.xyz) + t * dot(rayDetail.dir, v22.xyz);
                            if (v >= 0.f && u + v <= 1.f) {
                                result.tId = triId;
                                result.t = t;
                                result.u = u;
                                result.v = v;
                            }
                        }
                    }
                }
                leafId = nodeId;

                // Another leaf was postponed => process it as well.
                if (nodeId < 0) {
                    nodeId = traversalStack[stackId];
                    --stackId;
                }
            }
            // dynamic fetch
            if (subgroupBallotBitCount(subgroupBallot(true)) < DYNAMIC_FETCH_THRESHOLD)
                break;
        }

        if (nodeId == BOTTOM_OF_STACK) {
            results.result[rayId] = result;

            STATS_SHARED_STORE;
        }
    }
}

void traceTimed()
{
    TraceTime_ref times = TraceTime_ref(pc.data.traceTimeAddress);
    if (gl_LocalInvocationIndex == 0) {
        atomicMin(times.val.tStart, clockRealtimeEXT());
        RayBufferMetadata_ref rayBufMeta = RayBufferMetadata_ref(pc.data.rayBufferMetadataAddress);
        times.val.rayCount = rayBufMeta.data.rayCount;

        STATS_SHARED_RESET;
    }
    barrier();

    trace();

    barrier();
    if (gl_LocalInvocationIndex == 0) {
        atomicMax(times.val.timer, u32(clockRealtimeEXT() - times.val.tStart));

        STATS_GLOBAL_STORE;
    }
}

#endif
