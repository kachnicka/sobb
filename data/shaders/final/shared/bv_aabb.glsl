#ifndef BV_AABB_GLSL
#define BV_AABB_GLSL

struct AABB {
    vec3 min;
    vec3 max;
};

void bvFit(inout AABB aabb, in vec3 v)
{
    aabb.min = min(aabb.min, v);
    aabb.max = max(aabb.max, v);
}

void bvFit(inout AABB aabb, in AABB aabbToFit)
{
    bvFit(aabb, aabbToFit.min);
    bvFit(aabb, aabbToFit.max);
}

AABB bvOverlap(in AABB a, in AABB b)
{
    AABB result;
    result.min = max(a.min, b.min);
    result.max = min(a.max, b.max);

    if (result.min.x > result.max.x || result.min.y > result.max.y || result.min.z > result.max.z) {
        result.min = vec3(0.f);
        result.max = vec3(0.f);
    }
    return result;
}

vec3 bvCentroid(in AABB aabb)
{
    return (aabb.min + aabb.max) * .5f;
}

float bvArea(in AABB aabb)
{
    vec3 d = aabb.max - aabb.min;
    return 2.f * (d.x * d.y + d.x * d.z + d.z * d.y);
}

AABB dummyAABB()
{
    return AABB(vec3(-BIG_FLOAT), vec3(BIG_FLOAT));
}

#endif
