#ifndef HEADER_BVH_TRAVERSAL_GLSL
#define HEADER_BVH_TRAVERSAL_GLSL

#extension GL_EXT_buffer_reference2: require
#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_shader_atomic_int64 : require
#extension GL_EXT_shader_realtime_clock: require

#extension GL_KHR_memory_scope_semantics : require
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_vote : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_arithmetic : require

#include "shared/types.glsl"
#include "shared/compute.glsl"

#define INCLUDE_FROM_SHADER
#include "shared/data_ptrace.h"

#endif
