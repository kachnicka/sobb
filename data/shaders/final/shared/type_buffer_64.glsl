#ifndef TYPE_BUFFER_64_GLSL
#define TYPE_BUFFER_64_GLSL

#include "type_64.glsl"

layout(buffer_reference, scalar) buffer u64_buf {
    u64 val[];
};

#endif
