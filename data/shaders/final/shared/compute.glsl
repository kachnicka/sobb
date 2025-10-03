#ifndef COMPUTE_GLSL
#define COMPUTE_GLSL

#define INVALID_ID -1
#define INVALID_VALUE_I32 0x7FFFFFFF
#define BIG_FLOAT 1e38f

u32 divCeil(in u32 a, in u32 b)
{
    return (a + b - 1) / b;
}

#endif
