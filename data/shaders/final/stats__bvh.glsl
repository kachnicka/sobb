#ifndef STATS_GLSL
#define STATS_GLSL

#define CONCAT(a, b) a##b
#define EXPAND_AND_CONCAT(a, b) CONCAT(a, b)
#define BVH_NODE EXPAND_AND_CONCAT(Node, BVH_TYPE)

#define INCLUDE_FROM_SHADER
#include "shared/data_bvh.h"

layout(local_size_x_id = 0) in;

layout(push_constant, scalar) uniform uPushConstant {
    PC_BvhStats data;
} pc;

shared float satAggregate;
shared float saiAggregate;

shared float ciAggregate;

shared uint leafSizeSum;
shared uint leafSizeMin;
shared uint leafSizeMax;

void stats() {
    if (gl_GlobalInvocationID.x >= (pc.data.nodeCount - 1))
        return;
    if (gl_LocalInvocationID.x == 0) {
        satAggregate = 0.f;
        saiAggregate = 0.f;
        ciAggregate = 0.f;

        leafSizeSum = 0;
        leafSizeMin = 0xFFFFFFFF;
        leafSizeMax = 0;
    }
    barrier();

    BVH_TYPE bvh = BVH_TYPE(pc.data.bvhAddress);
    BVH_NODE node = bvh.node[gl_GlobalInvocationID.x];

    float nodeSA = bvArea(node.bv);

    if (node.size <= 1) {
        atomicAdd(saiAggregate, nodeSA);
        atomicAdd(ciAggregate, abs(node.size) * nodeSA);

        atomicAdd(leafSizeSum, abs(node.size));
        atomicMin(leafSizeMin, abs(node.size));
        atomicMax(leafSizeMax, abs(node.size));
    }
    else
        atomicAdd(satAggregate, nodeSA);

    barrier();
    BvhStats stats = BvhStats(pc.data.resultBufferAddress);
    if (gl_LocalInvocationID.x == 0) {
        atomicAdd(stats.sat, satAggregate / pc.data.sceneAABBSurfaceArea);
        atomicAdd(stats.sai, saiAggregate / pc.data.sceneAABBSurfaceArea);

        atomicAdd(stats.ct, pc.data.c_t * satAggregate / pc.data.sceneAABBSurfaceArea);
        atomicAdd(stats.ci, pc.data.c_i * ciAggregate / pc.data.sceneAABBSurfaceArea);

        atomicAdd(stats.leafSizeSum, leafSizeSum);
        atomicMin(stats.leafSizeMin, leafSizeMin);
        atomicMax(stats.leafSizeMax, leafSizeMax);
    }
}
#endif
