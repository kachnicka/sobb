#ifndef RT_COMMON_GLSL
#define RT_COMMON_GLSL

#define M_PI 3.1415926535897932384626433832795f
#define M_TWO_PI 6.283185307179586476925286766559f
#define M_PI_INV 0.31830988618379067153776752674503f
#define M_TWO_PI_INV 0.15915494309189533576888376337251f

vec3 OffsetRay(in vec3 p, in vec3 n)
{
    const f32 intScale = 256.f;
    const f32 f32Scale = 1.f / 65536.f;
    const f32 origin = 1.f / 32.f;

    ivec3 of_i = ivec3(intScale * n.x, intScale * n.y, intScale * n.z);

    vec3 p_i = vec3(intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
            intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
            intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

    return vec3(abs(p.x) < origin ? p.x + f32Scale * n.x : p_i.x,
        abs(p.y) < origin ? p.y + f32Scale * n.y : p_i.y,
        abs(p.z) < origin ? p.z + f32Scale * n.z : p_i.z);
}

vec3 SampleHemisphereCosineWorldSpace(in f32 r1, in f32 r2, in vec3 n)
{
    const f32 factor = 0.9999f;
    const f32 a = (1.f - 2.f * r1) * factor;
    const f32 b = sqrt(1.f - a * a) * factor;
    const f32 phi = M_TWO_PI * r2;

    return normalize(n + vec3(b * cos(phi), b * sin(phi), a));
}

#endif
