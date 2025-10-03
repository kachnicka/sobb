#ifndef REARRANGE_BVH2_GLSL
#define REARRANGE_BVH2_GLSL

#define CONCAT(a, b) a##b
#define EXPAND_AND_CONCAT(a, b) CONCAT(a, b)
#define DST_BVH_TYPE EXPAND_AND_CONCAT(SRC_BVH_TYPE, _c)
#define DST_BVH_NODE EXPAND_AND_CONCAT(Node, DST_BVH_TYPE)

#extension GL_EXT_buffer_reference2: require
#extension GL_EXT_scalar_block_layout: require

#extension GL_KHR_memory_scope_semantics : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_arithmetic : require
#extension GL_EXT_control_flow_attributes: require

#define INCLUDE_FROM_SHADER
#include "shared/data_bvh.h"
#include "shared/data_plocpp.h"

layout(local_size_x_id = 0) in;

layout(push_constant) uniform uPushConstant {
    PC_Rearrange data;
} pc;

struct WorkItem {
    i32 bvhId;
    i32 bvhWideId;
};

struct RuntimeData {
    u32 workgroupId;
    u32 workItemId;
    i32 wideId;
};
layout(buffer_reference, scalar) buffer RuntimeData_ref {
    RuntimeData data;
};

#define UNROLL_NEXT_LOOP [[unroll]]
#define WIDTH 2

shared u32 partitionId;

void rearrange()
{
    RuntimeData_ref runtime = RuntimeData_ref(pc.data.runtimeDataAddress);
    if (gl_LocalInvocationIndex == 0)
        partitionId = atomicAdd(runtime.data.workgroupId, 1);
    barrier();
    const u32 wgId = partitionId;
    const u32 globalThreadId = partitionId * gl_WorkGroupSize.x + gl_LocalInvocationIndex;

    if (globalThreadId >= pc.data.leafNodeCount)
        return;

    SRC_BVH_TYPE bvh = SRC_BVH_TYPE(pc.data.bvhAddress);
    DST_BVH_TYPE bvhWide = DST_BVH_TYPE(pc.data.bvhWideAddress);
    u64_buf workItems = u64_buf(pc.data.workBufferAddress);

    WorkItem wi;
    u64 wiPacked;

    while (true) {
        wiPacked = atomicLoad(workItems.val[globalThreadId],
                gl_ScopeQueueFamily, gl_StorageSemanticsBuffer, gl_SemanticsAcquire | gl_SemanticsMakeVisible);
        wi.bvhId = i32(wiPacked);
        wi.bvhWideId = i32(wiPacked >> 32);

        if (wi.bvhId < 0)
            break;

        if (wi.bvhId >= 0 && wi.bvhId < INVALID_VALUE_I32) {
            i32 c[WIDTH];
            f32 sa[WIDTH];
            i32 nonLeafCount = 0;
            c[0] = bvh.node[wi.bvhId].c0;
            c[1] = bvh.node[wi.bvhId].c1;

            UNROLL_NEXT_LOOP
            for (i32 i = 0; i < WIDTH; i++) {
                i32 cId = c[i];
                if (c[i] < 0)
                    cId = ~cId;
                else
                    nonLeafCount++;
                sa[i] = bvArea(bvh.node[cId].bv);
            }

            // bigger node first
            if (sa[0] < sa[1]) {
                i32 tmp = c[0];
                c[0] = c[1];
                c[1] = tmp;
            }

            DST_BVH_NODE node;

            i32 wideIdOffset;
            i32 interiorCount = subgroupAdd(nonLeafCount);
            if (subgroupElect())
                wideIdOffset = atomicAdd(runtime.data.wideId, interiorCount);
            wideIdOffset = subgroupBroadcastFirst(wideIdOffset) + subgroupExclusiveAdd(nonLeafCount);
            i32 wideIdLocalOffset = 0;

            u32 wiOffset;
            const uvec4 ballot = subgroupBallot(true);
            const u32 ballotBitCount = subgroupBallotBitCount(ballot);
            if (subgroupElect())
                wiOffset = atomicAdd(runtime.data.workItemId, ballotBitCount);
            wiOffset = subgroupBroadcastFirst(wiOffset) + subgroupBallotExclusiveBitCount(ballot);

            // int wideIdOffset;
            // if (nonLeafCount > 0)
            //     wideIdOffset = atomicAdd(runtime.data.wideId, nonLeafCount);
            // int wideIdLocalOffset = 0;
            //
            // uint wiOffset = atomicAdd(runtime.data.workItemId, 1);

            UNROLL_NEXT_LOOP
            for (i32 i = 0; i < WIDTH; i++) {
                const i32 cId = c[i] < 0 ? ~c[i] : c[i];
                node.bv[i] = bvh.node[cId].bv;

                if (c[i] < 0) {
                    i32 size = abs(bvh.node[cId].size) - 1;
                    i32 c0 = bvh.node[cId].c0;
                    node.c[i] = (1 << 31) | (size << 27) | c0;
                    wiPacked = (u64(wi.bvhWideId) << 32) | u32(c[i]);
                }
                else {
                    i32 wideId = wideIdOffset + (wideIdLocalOffset++);
                    node.c[i] = wideId;
                    wiPacked = (u64(wideId) << 32) | u32(c[i]);
                }

                u32 workItemId = globalThreadId;
                if (i > 0)
                    workItemId = wiOffset + i - 1;

                atomicStore(workItems.val[workItemId], wiPacked,
                    gl_ScopeQueueFamily, gl_StorageSemanticsBuffer, gl_SemanticsRelease | gl_SemanticsMakeAvailable);
            }
            bvhWide.node[wi.bvhWideId] = node;
        }
    }
}
#endif
