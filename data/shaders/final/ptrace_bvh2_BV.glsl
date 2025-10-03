#ifndef PTRACE_BVH2_BV_GLSL
#define PTRACE_BVH2_BV_GLSL

#define INCLUDE_FROM_SHADER
#include "shared/data_bvh.h"

layout(push_constant, scalar) uniform uPushConstant
{
    PC_Trace data;
    i32 depth;
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
    i32 nodeDepthStack[STACK_SIZE];
    traversalStack[0] = BOTTOM_OF_STACK;
    nodeDepthStack[0] = 0;

    i32 nodeId = BOTTOM_OF_STACK;
    i32 leafId;
    u32 rayId;
    RayDetail rayDetail;
    i32 nodeDepth;

    BVH_TYPE bvh = BVH_TYPE(pc.data.bvhAddress);
    BvhTriangles triangles = BvhTriangles(pc.data.bvhTrianglesAddress);
    BvhTriangleIndices triangleIndices = BvhTriangleIndices(pc.data.bvhTriangleIndicesAddress);

    RayBufferMetadata_ref rayBufMeta = RayBufferMetadata_ref(pc.data.rayBufferMetadataAddress);
    RayBuffer_ref rayBuffer = RayBuffer_ref(pc.data.rayBufferAddress);
    RayTraceResults_BV_ref results = RayTraceResults_BV_ref(pc.data.rayTraceResultAddress);
    RayTraceResult_BV result;

    const u32 rayCount = rayBufMeta.data.rayCount;

    while (true) {
        const bool isTerminated = (nodeId == BOTTOM_OF_STACK);
        const uvec4 ballot = subgroupBallot(isTerminated);
        const u32 terminatedCount = subgroupBallotBitCount(ballot);
        const u32 terminatedId = subgroupBallotExclusiveBitCount(ballot);

        if (isTerminated) {
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
            nodeDepth = 0;

            result.tId = INVALID_ID;
            result.bvId = INVALID_ID;
            result.normal = vec3(0.f);
        }

        while (nodeId != BOTTOM_OF_STACK)
        {
            while (u32(nodeId) < u32(BOTTOM_OF_STACK))
            // while (nodeId >= 0 && nodeId != BOTTOM_OF_STACK)
            {
                nodeDepth++;

                vec3 c0normal;
                const vec2 c0minmax = intersect(bvh.node[nodeId].bv[0], rayDetail, result.t, c0normal);
                vec3 c1normal;
                const vec2 c1minmax = intersect(bvh.node[nodeId].bv[1], rayDetail, result.t, c1normal);

                ivec2 cnodes = ivec2(bvh.node[nodeId].c[0], bvh.node[nodeId].c[1]);
                const bool swp = (c1minmax[0] < c0minmax[0]);
                const bool traverseC0 = (c0minmax[1] >= c0minmax[0]);
                const bool traverseC1 = (c1minmax[1] >= c1minmax[0]);

                if (!traverseC0 && !traverseC1) {
                    nodeId = traversalStack[stackId];
                    nodeDepth = nodeDepthStack[stackId];
                    --stackId;
                }
                else {
                    f32 tMin = (traverseC0) ? c0minmax[0] : c1minmax[0];
                    nodeId = (traverseC0) ? cnodes.x : cnodes.y;

                    if (traverseC0 && traverseC1) {
                        tMin = min(c0minmax[0], c1minmax[0]);

                        if (swp) {
                            i32 tmp = nodeId;
                            nodeId = cnodes.y;
                            cnodes.y = tmp;
                        }
                        if ((pc.depth == 0) || (pc.depth > nodeDepth && cnodes.y >= 0))
                        {
                            ++stackId;
                            traversalStack[stackId] = cnodes.y;
                            nodeDepthStack[stackId] = nodeDepth;
                        }
                    }

                    // store bv intersection
                    if ((pc.depth > 0) && (nodeDepth == pc.depth || nodeId < 0)) {
                        if (tMin > rayDetail.tmin)
                        {
                            result.t = tMin;
                            if (nodeId < 0)
                                result.bvId = nodeId & 0x07FFFFFF;
                            else
                                result.bvId = nodeId;

                            if (traverseC0 && tMin == c0minmax[0])
                                result.normal = c0normal;
                            if (traverseC1 && tMin == c1minmax[0])
                                result.normal = c1normal;
                        }
                    }
                }
                bool fetchFromStack = false;
                if (pc.depth == 0) {
                    if (nodeId < 0 && leafId >= 0) {
                        leafId = nodeId;
                        fetchFromStack = true;
                    }
                }
                else if (pc.depth == nodeDepth || nodeId < 0)
                    fetchFromStack = true;

                if (fetchFromStack) {
                    nodeId = traversalStack[stackId];
                    nodeDepth = nodeDepthStack[stackId];
                    --stackId;
                }

                if (subgroupAll(leafId < 0))
                    break;
            }

            while (leafId < 0) {
                const i32 triStartId = leafId & 0x07FFFFFF;
                const i32 triCount = ((leafId >> 27) & 0xF) + 1;

                for (i32 triId = triStartId; triId < triStartId + triCount; ++triId) {
                    const vec4 v00 = triangles.t[triId].v0;
                    const vec4 v11 = triangles.t[triId].v1;
                    const vec4 v22 = triangles.t[triId].v2;

                    const f32 t = (v00.w - dot(rayDetail.origin, v00.xyz)) / dot(rayDetail.dir, v00.xyz);
                    if (t > rayDetail.tmin && t < result.t) {
                        const f32 u = v11.w + dot(rayDetail.origin, v11.xyz) + t * dot(rayDetail.dir, v11.xyz);
                        if (u >= 0.f) {
                            const f32 v = v22.w + dot(rayDetail.origin, v22.xyz) + t * dot(rayDetail.dir, v22.xyz);
                            if (v >= 0.f && u + v <= 1.f) {
                                result.t = t;
                                result.bvId = leafId & 0x07FFFFFF;
                                result.tId = triId;
                            }
                        }
                    }
                }
                leafId = nodeId;

                if (nodeId < 0)
                {
                    nodeId = traversalStack[stackId];
                    nodeDepth = nodeDepthStack[stackId];
                    --stackId;
                }
            }
            // dynamic fetch
            if (subgroupBallotBitCount(subgroupBallot(true)) < DYNAMIC_FETCH_THRESHOLD)
                break;
        }

        if (nodeId == BOTTOM_OF_STACK) {
            results.result[rayId] = result;
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
    }
    barrier();

    trace();

    barrier();
    if (gl_LocalInvocationIndex == 0) {
        atomicMax(times.val.timer, u32(clockRealtimeEXT() - times.val.tStart));
    }
}

#endif
