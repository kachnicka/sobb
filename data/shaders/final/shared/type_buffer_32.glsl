#ifndef TYPE_BUFFER_32_GLSL
#define TYPE_BUFFER_32_GLSL

#include "type_32.glsl"

layout(buffer_reference, scalar) buffer vec3_buf {
    vec3 val[];
};
layout(buffer_reference, scalar) buffer ivec3_buf {
    ivec3 val[];
};
layout(buffer_reference, scalar) buffer uvec3_buf {
    uvec3 val[];
};

layout(buffer_reference, scalar) buffer f32_buf {
    f32 val[];
};
layout(buffer_reference, scalar) buffer i32_buf {
    i32 val[];
};
layout(buffer_reference, scalar) buffer u32_buf {
    u32 val[];
};

#endif
