#ifndef DATA_PTRACE_H
#define DATA_PTRACE_H

#ifndef INCLUDE_FROM_SHADER
#    include <berries/util/types.h>
using ivec4 = i32[4];
using uvec3 = u32[3];
using uvec2 = u32[2];
using vec2 = f32[2];
using vec3 = f32[3];
using vec4 = f32[4];
#    pragma pack(push, 1)
namespace data_ptrace {
#else
#endif

#define RAY_TYPE_PRIMARY 0
#define RAY_TYPE_SHADOW 1
#define RAY_TYPE_SECONDARY 2

struct Ray {
    vec4 o;
    vec4 d;
};

struct RayBufferMetadata {
    u32 rayCount;
    u32 rayTracedCount;
};

struct RayTraceResult {
    u32 tId;
    f32 t;
    f32 u;
    f32 v;
};

struct RayTraceResult_BV {
    vec3 normal;
    f32 t;
    u32 tId;
    u32 bvId;
};

struct RayTraceResult_Intersections {
    u32 tId;
    f32 t;
    f32 u;
    f32 v;
    u32 boundingVolumes;
    u32 triangles;
};

struct RayPayload {
    vec3 throughput;
    u32 packedPosition;
    u32 seed;
};

struct TraceTime {
    u64 tStart;
    u32 timer;
    u32 rayCount;
};

struct TraceTimes {
    TraceTime times[8];
};

struct TraceStats {
    u32 testedNodes;
    u32 testedTriangles;
    u32 testedBVolumes;
    u32 reserved;
};

struct PC_GenPrimary {
    u64 rayBufferMetadataAddress;
    u64 rayBufferAddress;
    u64 rayPayloadAddress;
    u32 samplesComputed;
    u32 samplesToComputeThisFrame;
};

struct PC_Trace {
    u64 rayBufferMetadataAddress;
    u64 rayBufferAddress;
    u64 rayTraceResultAddress;

    u64 bvhAddress;
    u64 bvhTrianglesAddress;
    u64 bvhTriangleIndicesAddress;
    u64 bvhAuxAddress;

    u64 traceTimeAddress;
    u64 traceStatsAddress;
};

struct PC_ShadeCast {
    vec4 dirLight;

    u64 read_rayBufferMetadataAddress;
    u64 read_rayBufferAddress;
    u64 read_rayPayloadAddress;

    u64 write_rayBufferMetadataAddress;
    u64 write_rayBufferAddress;
    u64 write_rayPayloadAddress;

    u64 rayTraceResultAddress;
    u64 geometryDescriptorAddress;
    u64 bvhTriangleIndicesAddress;

    u32 depth;
    u32 depthMax;
    u32 samplesComputed;
};

#ifndef INCLUDE_FROM_SHADER
static_assert(sizeof(Ray) == 32);
static_assert(sizeof(RayBufferMetadata) == 8);
static_assert(sizeof(RayTraceResult) == 16);
static_assert(sizeof(RayTraceResult_BV) == 24);
static_assert(sizeof(RayTraceResult_Intersections) == 24);
static_assert(sizeof(RayPayload) == 20);
static_assert(sizeof(TraceTime) == 16);
static_assert(sizeof(TraceTimes) == 128);
static_assert(sizeof(TraceStats) == 16);
static_assert(sizeof(PC_GenPrimary) == 32);
static_assert(sizeof(PC_Trace) == 72);
static_assert(sizeof(PC_ShadeCast) == 100);
}
#    pragma pack(pop)
#else
layout(buffer_reference, scalar) buffer RayBuffer_ref { Ray ray[]; };
layout(buffer_reference, scalar) buffer RayBufferMetadata_ref { RayBufferMetadata data; };
layout(buffer_reference, scalar) buffer RayTraceResults_ref { RayTraceResult result[]; };
layout(buffer_reference, scalar) buffer RayTraceResults_BV_ref { RayTraceResult_BV result[]; };
layout(buffer_reference, scalar) buffer RayTraceResults_Intersections_ref { RayTraceResult_Intersections result[]; };
layout(buffer_reference, scalar) buffer RayPayloadBuffer_ref { RayPayload val[]; };
layout(buffer_reference, scalar) buffer TraceTime_ref { TraceTime val; };
layout(buffer_reference, scalar) buffer TraceStats_ref { TraceStats val; };
#endif

#endif
