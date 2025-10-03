#ifndef RANDOM_GLSL
#define RANDOM_GLSL

// Generate a random unsigned int from two unsigned int values, using 16 pairs
// of rounds of the Tiny Encryption Algorithm. See Zafar, Olano, and Curtis,
// "GPU Random Numbers via the Tiny Encryption Algorithm"
u32 tea(in u32 v0, in u32 v1)
{
    u32 s0 = 0;

    for (u32 n = 0; n < 16; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }

    return v0;
}

// Generate a random unsigned int in [0, 2^24) given the previous RNG state
// using the Numerical Recipes linear congruential generator
u32 lcg(inout u32 prev)
{
    u32 LCG_A = 1664525u;
    u32 LCG_C = 1013904223u;
    prev = (LCG_A * prev + LCG_C);
    return prev & 0x00FFFFFF;
}

// Generate a random f32 in [0, 1) given the previous RNG state
f32 rnd(inout u32 prev)
{
    return (f32(lcg(prev)) / f32(0x01000000));
}

#endif
