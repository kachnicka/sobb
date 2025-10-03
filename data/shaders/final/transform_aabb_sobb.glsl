#ifndef TRANSFORM_AABB_SOBB_GLSL
#define TRANSFORM_AABB_SOBB_GLSL

#extension GL_EXT_buffer_reference2: require
#extension GL_EXT_scalar_block_layout: require
#extension GL_KHR_memory_scope_semantics : require

#define INCLUDE_FROM_SHADER
#include "shared/types.glsl"
#include "shared/compute.glsl"
#include "shared/bv_aabb.glsl"
#include "shared/bv_dop.glsl"
#include "shared/bv_sobb.glsl"
#include "shared/data_bvh.h"
#include "shared/data_plocpp.h"
#include "shared/data_scene.h"

layout(local_size_x_id = 0) in;

layout(push_constant) uniform uPushConstant {
    PC_TransformToSOBB data;
} pc;

#ifdef TRANSFORM_SOBB
#define BVH_TYPE BVH2_SOBB
SOBB bvEncode(in DOP dop, in int i, in int j, in int k)
{
    return SOBB(
        dSlabEncode(DOP_NORMALS[i], dop_min(dop, i), dop_max(dop, i)),
        dSlabEncode(DOP_NORMALS[j], dop_min(dop, j), dop_max(dop, j)),
        dSlabEncode(DOP_NORMALS[k], dop_min(dop, k), dop_max(dop, k))
    );
}
#endif

#ifdef TRANSFORM_SOBBi
#define BVH_TYPE BVH2_SOBBi
SOBBi bvEncode(in DOP dop, in int i, in int j, in int k)
{
    return SOBBi(
        vec2(dop_min(dop, i), dop_max(dop, i)),
        vec2(dop_min(dop, j), dop_max(dop, j)),
        vec2(dop_min(dop, k), dop_max(dop, k)),
        (i << 20) | (j << 10) | k
    );
}
#endif

void transform(in u32 taskId)
{
    u32 nodeId = taskId * gl_WorkGroupSize.x + gl_LocalInvocationID.x;
    if (nodeId >= pc.data.leafNodeCount)
        return;

    BvhTriangleIndices triangleIndices = BvhTriangleIndices(pc.data.bvhTriangleIndicesAddress);
    u32_buf counter = u32_buf(pc.data.countersAddress);
    GeometryDescriptor gDesc = GeometryDescriptor(pc.data.geometryDescriptorAddress);

    BVH2_AABB bvh = BVH2_AABB(pc.data.bvhAddress);
    NodeBVH2_AABB node = bvh.node[nodeId];

    BVH_TYPE bvhSOBB = BVH_TYPE(pc.data.bvhSOBBAddress);
    bvhSOBB.node[nodeId].size = node.size;
    bvhSOBB.node[nodeId].parent = node.parent;
    bvhSOBB.node[nodeId].c0 = node.c0;
    bvhSOBB.node[nodeId].c1 = node.c1;

    DOP dop;
    {
        i32 triStartId = node.c0;
        i32 triCount = abs(node.size) - 1;
        // init the dop with the first triangle, increment the triangle index
        {
            BvhTriangleIndex ids = triangleIndices.val[triStartId++];
            Geometry g = gDesc.g[ids.nodeId];
            uvec3_buf indices = uvec3_buf(g.idxAddress);
            vec3_buf vertices = vec3_buf(g.vtxAddress);

            const uvec3 idx = indices.val[ids.triangleId];
            dop = dopInit(vertices.val[idx.x], vertices.val[idx.y], vertices.val[idx.z]);
        }
        // fit the rest of the triangles
        for (i32 triId = triStartId; triId < triStartId + triCount; triId++) {
            BvhTriangleIndex ids = triangleIndices.val[triId];
            Geometry g = gDesc.g[ids.nodeId];
            uvec3_buf indices = uvec3_buf(g.idxAddress);
            vec3_buf vertices = vec3_buf(g.vtxAddress);

            const uvec3 idx = indices.val[ids.triangleId];
            bvFit(dop, vertices.val[idx.x], vertices.val[idx.y], vertices.val[idx.z]);
        }
    }
    u32_buf dopIds = u32_buf(pc.data.dopRefAddress);
    DOP_ref dops = DOP_ref(pc.data.dopBaseAddress);

    dopIds.val[nodeId] = nodeId;
    dops.val[nodeId] = dop;

    i32 i, j, k;
    FitSOBB(dop, i, j, k);
    bvhSOBB.node[nodeId].bv = bvEncode(dop, i, j, k);

    // '~' encoded leafId
    i32 cId = ~(i32(nodeId));
    nodeId = node.parent;
    while (atomicAdd(counter.val[nodeId], 1) > 0) {
        node = bvh.node[nodeId];
        bvhSOBB.node[nodeId].size = node.size;
        bvhSOBB.node[nodeId].parent = node.parent;
        bvhSOBB.node[nodeId].c0 = node.c0;
        bvhSOBB.node[nodeId].c1 = node.c1;

        i32 nodeIdToLoad = (cId == node.c0) ? node.c1 : node.c0;
        if (nodeIdToLoad < 0)
            nodeIdToLoad = ~nodeIdToLoad;
        if (cId < 0)
            cId = ~cId;

        const DOP cDop = dops.val[dopIds.val[nodeIdToLoad]];
        bvFit(dop, cDop);

        dopIds.val[nodeId] = dopIds.val[cId];
        dops.val[dopIds.val[nodeId]] = dop;

        FitSOBB(dop, i, j, k);
        bvhSOBB.node[nodeId].bv = bvEncode(dop, i, j, k);

        memoryBarrier(gl_ScopeQueueFamily, gl_StorageSemanticsBuffer, gl_SemanticsAcquireRelease | gl_SemanticsMakeAvailable | gl_SemanticsMakeVisible);

        cId = i32(nodeId);
        nodeId = node.parent;
        if (nodeId == INVALID_ID)
            return;
    }
}

#endif
