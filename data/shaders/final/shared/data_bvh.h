#ifndef DATA_BVH_NEW_H
#define DATA_BVH_NEW_H

#ifndef INCLUDE_FROM_SHADER
#    include <berries/util/types.h>
using ivec4 = i32[4];
using uvec3 = u32[3];
using uvec2 = u32[2];
using vec2 = f32[2];
using vec3 = f32[3];
using vec4 = f32[4];
using mat4x3 = f32[12];
using AABB = f32[6];
using DOP = f32[14];
using DOP14_SPLIT = f32[8];
using SOBB = f32[12];
using SOBBi = f32[7];
#    pragma pack(push, 1)
namespace data_bvh {
#else
#    include "bv_aabb.glsl"
#    include "bv_sobb.glsl"
#    ifndef DOP
#        define DOP f32[14]
#    endif
#    define DOP14_SPLIT f32[8]
#endif

struct PC_BvhStats {
    u64 bvhAddress;
    u64 bvhAuxAddress;
    u64 resultBufferAddress;
    u32 nodeCount;
    f32 c_t;
    f32 c_i;
    f32 sceneAABBSurfaceArea;
};

struct NodeBVH2_AABB {
    AABB bv;
    i32 size;
    i32 parent;
    i32 c0;
    i32 c1;
};

struct NodeBVH2_AABB_c {
    AABB bv[2];
    i32 c[2];
};

struct NodeBVH2_DOP14 {
    DOP bv;
    i32 size;
    i32 parent;
    i32 c0;
    i32 c1;
};

struct NodeBVH2_DOP14_c {
    DOP bv[2];
    i32 c[2];
};

struct DOP3 {
    vec2 slab[3];
};

struct DOP4 {
    vec2 slab[4];
};

struct NodeBVH2_DOP3_c {
    DOP3 bv[2];
    i32 c[2];
};

struct NodeBVH2_DOP14_SPLIT_c {
   DOP4 bv[2];
};

struct NodeBVH2_OBB {
    mat4x3 bv;
    i32 size;
    i32 parent;
    i32 c0;
    i32 c1;
};

struct NodeBVH2_OBB_c {
    mat4x3 bv[2];
    i32 c[2];
};

struct BvhTriangleIndex {
    u32 nodeId;
    u32 triangleId;
};

struct BvhTriangle {
    vec4 v0;
    vec4 v1;
    vec4 v2;
};

struct NodeBVH2_SOBB {
    SOBB bv;
    i32 size;
    i32 parent;
    i32 c0;
    i32 c1;
};

// TODO: i can only guess, why this structure performs significantly better padded to 112 bytes
//  instead of 104 minimal size or 128 fully aligned size
struct NodeBVH2_SOBB_c {
    SOBB bv[2];
    i32 c[2];
    i32 padd[2];
};

struct NodeBVH2_SOBBi {
    SOBBi bv;
    i32 size;
    i32 parent;
    i32 c0;
    i32 c1;
};

struct NodeBVH2_SOBBi_c {
    SOBBi bv[2];
    i32 c[2];
};

#ifndef INCLUDE_FROM_SHADER
static_assert(sizeof(PC_BvhStats) == 40);
static_assert(sizeof(NodeBVH2_AABB) == 40);
static_assert(sizeof(NodeBVH2_AABB_c) == 56);
static_assert(sizeof(NodeBVH2_OBB) == 64);
static_assert(sizeof(NodeBVH2_OBB_c) == 104);
static_assert(sizeof(NodeBVH2_DOP3_c) == 56);
static_assert(sizeof(NodeBVH2_DOP14) == 72);
static_assert(sizeof(NodeBVH2_DOP14_c) == 120);
static_assert(sizeof(NodeBVH2_DOP14_SPLIT_c) == 64);
static_assert(sizeof(NodeBVH2_SOBB) == 64);
// static_assert(sizeof(NodeBVH2_SOBB_c) == 104);
static_assert(sizeof(NodeBVH2_SOBB_c) == 112);
static_assert(sizeof(NodeBVH2_SOBBi) == 44);
static_assert(sizeof(NodeBVH2_SOBBi_c) == 64);
}
#    pragma pack(pop)
#else
layout(buffer_reference, scalar) buffer BVH2_AABB { NodeBVH2_AABB node[]; };
layout(buffer_reference, scalar) buffer BVH2_AABB_c { NodeBVH2_AABB_c node[]; };

layout(buffer_reference, scalar) buffer BVH2_DOP14 { NodeBVH2_DOP14 node[]; };
layout(buffer_reference, scalar) buffer BVH2_DOP14_c { NodeBVH2_DOP14_c node[]; };
layout(buffer_reference, scalar) buffer BVH2_DOP14_SPLIT_c { NodeBVH2_DOP14_SPLIT_c node[]; };
layout(buffer_reference, scalar) buffer BVH2_DOP3_c { NodeBVH2_DOP3_c node[]; };

layout(buffer_reference, scalar) buffer BVH2_OBB { NodeBVH2_OBB node[]; };
layout(buffer_reference, scalar) buffer BVH2_OBB_c { NodeBVH2_OBB_c node[]; };

layout(buffer_reference, scalar) buffer BVH2_SOBB { NodeBVH2_SOBB node[]; };
layout(buffer_reference, scalar) buffer BVH2_SOBB_c { NodeBVH2_SOBB_c node[]; };

layout(buffer_reference, scalar) buffer BVH2_SOBBi { NodeBVH2_SOBBi node[]; };
layout(buffer_reference, scalar) buffer BVH2_SOBBi_c { NodeBVH2_SOBBi_c node[]; };

layout(buffer_reference, scalar) buffer BvhStats
{
    f32 sat;
    f32 sai;
    f32 ct;
    f32 ci;
    uint leafSizeSum;
    uint leafSizeMin;
    uint leafSizeMax;
};
layout(buffer_reference, scalar) buffer BvhTriangles { BvhTriangle t[]; };
layout(buffer_reference, scalar) buffer BvhTriangleIndices { BvhTriangleIndex val[]; };
#endif

#endif
