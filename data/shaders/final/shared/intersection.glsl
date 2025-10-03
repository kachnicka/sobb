#ifndef INTERSECTION_GLSL
#define INTERSECTION_GLSL

#include "types.glsl"

#define EPS_BV_INTERSECT 1e-5f

#ifdef INTERSECTION_AABB
#define HAVE_RAY_DETAIL
struct RayDetail {
    vec3 dir;
    vec3 origin;
    vec3 idir;
    vec3 ood;
    f32 tmin;
};

RayDetail initRayDetail(in Ray ray) {
    RayDetail rd;
    rd.origin = ray.o.xyz;
    rd.tmin = ray.o.w;
    rd.dir = ray.d.xyz;
    rd.idir.x = 1.f / (abs(ray.d.x) > EPS_BV_INTERSECT ? ray.d.x : EPS_BV_INTERSECT * sign(ray.d.x));
    rd.idir.y = 1.f / (abs(ray.d.y) > EPS_BV_INTERSECT ? ray.d.y : EPS_BV_INTERSECT * sign(ray.d.y));
    rd.idir.z = 1.f / (abs(ray.d.z) > EPS_BV_INTERSECT ? ray.d.z : EPS_BV_INTERSECT * sign(ray.d.z));
    rd.ood = rd.origin * rd.idir;
    return rd;
}
#endif

#ifdef INTERSECTION_DOP14
#define HAVE_RAY_DETAIL
struct RayDetail {
    f32[7] idir;
    f32[7] ood;
    vec3 dir;
    vec3 origin;
    f32 tmin;
};

RayDetail initRayDetail(in Ray ray) {
    RayDetail rd;
    rd.origin = ray.o.xyz;
    rd.tmin = ray.o.w;
    rd.dir = ray.d.xyz;

    f32 dotResult;
    dotResult = dot_dop14_n0(rd.dir);
    rd.idir[0] = 1.f / (abs(dotResult) > EPS_BV_INTERSECT ? dotResult : EPS_BV_INTERSECT * sign(dotResult));
    dotResult = dot_dop14_n1(rd.dir);
    rd.idir[1] = 1.f / (abs(dotResult) > EPS_BV_INTERSECT ? dotResult : EPS_BV_INTERSECT * sign(dotResult));
    dotResult = dot_dop14_n2(rd.dir);
    rd.idir[2] = 1.f / (abs(dotResult) > EPS_BV_INTERSECT ? dotResult : EPS_BV_INTERSECT * sign(dotResult));
    dotResult = dot_dop14_n3(rd.dir);
    rd.idir[3] = 1.f / (abs(dotResult) > EPS_BV_INTERSECT ? dotResult : EPS_BV_INTERSECT * sign(dotResult));
    dotResult = dot_dop14_n4(rd.dir);
    rd.idir[4] = 1.f / (abs(dotResult) > EPS_BV_INTERSECT ? dotResult : EPS_BV_INTERSECT * sign(dotResult));
    dotResult = dot_dop14_n5(rd.dir);
    rd.idir[5] = 1.f / (abs(dotResult) > EPS_BV_INTERSECT ? dotResult : EPS_BV_INTERSECT * sign(dotResult));
    dotResult = dot_dop14_n6(rd.dir);
    rd.idir[6] = 1.f / (abs(dotResult) > EPS_BV_INTERSECT ? dotResult : EPS_BV_INTERSECT * sign(dotResult));

    rd.ood[0] = dot_dop14_n0(rd.origin) * rd.idir[0];
    rd.ood[1] = dot_dop14_n1(rd.origin) * rd.idir[1];
    rd.ood[2] = dot_dop14_n2(rd.origin) * rd.idir[2];
    rd.ood[3] = dot_dop14_n3(rd.origin) * rd.idir[3];
    rd.ood[4] = dot_dop14_n4(rd.origin) * rd.idir[4];
    rd.ood[5] = dot_dop14_n5(rd.origin) * rd.idir[5];
    rd.ood[6] = dot_dop14_n6(rd.origin) * rd.idir[6];

    return rd;
}
#endif

#ifndef HAVE_RAY_DETAIL
struct RayDetail {
    vec3 dir;
    vec3 origin;
    f32 tmin;
};
RayDetail initRayDetail(in Ray ray) {
    RayDetail rd;
    rd.origin = ray.o.xyz;
    rd.tmin = ray.o.w;
    rd.dir = ray.d.xyz;
    return rd;
}
#endif

#ifdef INTERSECTION_AABB
vec2 intersect(in AABB bv, in RayDetail rd, in f32 tmax)
{
    const vec2 t0 = vec2(bv.min.x, bv.max.x) * rd.idir.x - rd.ood.x;
    const vec2 t1 = vec2(bv.min.y, bv.max.y) * rd.idir.y - rd.ood.y;
    const vec2 t2 = vec2(bv.min.z, bv.max.z) * rd.idir.z - rd.ood.z;
    return vec2(
        max(max(min(t0.x, t0.y), min(t1.x, t1.y)), max(min(t2.x, t2.y), rd.tmin)),
        min(min(max(t0.x, t0.y), max(t1.x, t1.y)), min(max(t2.x, t2.y), tmax))
    );
}
vec2 intersect(in AABB bv, in RayDetail rd, in f32 tmax, out vec3 normal)
{
    const vec2 t0 = vec2(bv.min.x, bv.max.x) * rd.idir.x - rd.ood.x;
    const vec2 t1 = vec2(bv.min.y, bv.max.y) * rd.idir.y - rd.ood.y;
    const vec2 t2 = vec2(bv.min.z, bv.max.z) * rd.idir.z - rd.ood.z;

    vec2 result = vec2(
            max(max(min(t0.x, t0.y), min(t1.x, t1.y)), max(min(t2.x, t2.y), rd.tmin)),
            min(min(max(t0.x, t0.y), max(t1.x, t1.y)), min(max(t2.x, t2.y), tmax))
        );

    if (result[0] == t0.x) normal = vec3(-1.f, 0.f, 0.f);
    if (result[0] == t0.y) normal = vec3(1.f, 0.f, 0.f);
    if (result[0] == t1.x) normal = vec3(0.f, -1.f, 0.f);
    if (result[0] == t1.y) normal = vec3(0.f, 1.f, 0.f);
    if (result[0] == t2.x) normal = vec3(0.f, 0.f, -1.f);
    if (result[0] == t2.y) normal = vec3(0.f, 0.f, 1.f);

    return result;
}
#endif

#ifdef INTERSECTION_DOP14
vec2 intersect(in DOP bv, in RayDetail rd, in f32 tmax)
{
    const vec2 t0 = vec2(dop_min(bv, 0), dop_max(bv, 0)) * rd.idir[0] - rd.ood[0];
    const vec2 t1 = vec2(dop_min(bv, 1), dop_max(bv, 1)) * rd.idir[1] - rd.ood[1];
    const vec2 t2 = vec2(dop_min(bv, 2), dop_max(bv, 2)) * rd.idir[2] - rd.ood[2];

    vec2 result = vec2(
            max(max(min(t0.x, t0.y), min(t1.x, t1.y)), max(min(t2.x, t2.y), rd.tmin)),
            min(min(max(t0.x, t0.y), max(t1.x, t1.y)), min(max(t2.x, t2.y), tmax))
        );
    if (result.x <= result.y) {
        const vec2 t3 = vec2(dop_min(bv, 3), dop_max(bv, 3)) * rd.idir[3] - rd.ood[3];
        const vec2 t4 = vec2(dop_min(bv, 4), dop_max(bv, 4)) * rd.idir[4] - rd.ood[4];
        const vec2 t5 = vec2(dop_min(bv, 5), dop_max(bv, 5)) * rd.idir[5] - rd.ood[5];
        const vec2 t6 = vec2(dop_min(bv, 6), dop_max(bv, 6)) * rd.idir[6] - rd.ood[6];
        result.x = max(result.x, max(max(min(t3.x, t3.y), min(t4.x, t4.y)), max(min(t5.x, t5.y), min(t6.x, t6.y))));
        result.y = min(result.y, min(min(max(t3.x, t3.y), max(t4.x, t4.y)), min(max(t5.x, t5.y), max(t6.x, t6.y))));
    }
    return result;
}
vec2 intersect(in DOP bv, in RayDetail rd, in f32 tmax, out vec3 normal)
{
    const vec2 t0 = vec2(dop_min(bv, 0), dop_max(bv, 0)) * rd.idir[0] - rd.ood[0];
    const vec2 t1 = vec2(dop_min(bv, 1), dop_max(bv, 1)) * rd.idir[1] - rd.ood[1];
    const vec2 t2 = vec2(dop_min(bv, 2), dop_max(bv, 2)) * rd.idir[2] - rd.ood[2];

    vec2 result = vec2(
            max(max(min(t0.x, t0.y), min(t1.x, t1.y)), max(min(t2.x, t2.y), rd.tmin)),
            min(min(max(t0.x, t0.y), max(t1.x, t1.y)), min(max(t2.x, t2.y), tmax))
        );

    const vec2 t3 = vec2(dop_min(bv, 3), dop_max(bv, 3)) * rd.idir[3] - rd.ood[3];
    const vec2 t4 = vec2(dop_min(bv, 4), dop_max(bv, 4)) * rd.idir[4] - rd.ood[4];
    const vec2 t5 = vec2(dop_min(bv, 5), dop_max(bv, 5)) * rd.idir[5] - rd.ood[5];
    const vec2 t6 = vec2(dop_min(bv, 6), dop_max(bv, 6)) * rd.idir[6] - rd.ood[6];
    result.x = max(result.x, max(max(min(t3.x, t3.y), min(t4.x, t4.y)), max(min(t5.x, t5.y), min(t6.x, t6.y))));
    result.y = min(result.y, min(min(max(t3.x, t3.y), max(t4.x, t4.y)), min(max(t5.x, t5.y), max(t6.x, t6.y))));

    if (result[0] == t0.x) normal = -DOP_NORMALS[0];
    if (result[0] == t0.y) normal = DOP_NORMALS[0];
    if (result[0] == t1.x) normal = -DOP_NORMALS[1];
    if (result[0] == t1.y) normal = DOP_NORMALS[1];
    if (result[0] == t2.x) normal = -DOP_NORMALS[2];
    if (result[0] == t2.y) normal = DOP_NORMALS[2];
    if (result[0] == t3.x) normal = -DOP_NORMALS[3];
    if (result[0] == t3.y) normal = DOP_NORMALS[3];
    if (result[0] == t4.x) normal = -DOP_NORMALS[4];
    if (result[0] == t4.y) normal = DOP_NORMALS[4];
    if (result[0] == t5.x) normal = -DOP_NORMALS[5];
    if (result[0] == t5.y) normal = DOP_NORMALS[5];
    if (result[0] == t6.x) normal = -DOP_NORMALS[6];
    if (result[0] == t6.y) normal = DOP_NORMALS[6];

    return result;
}
#endif

#ifdef INTERSECTION_OBB
vec2 intersect(in mat4x3 bv, in RayDetail rd, in f32 tmax)
{
    const mat4 m = mat4(vec4(bv[0], 0.f), vec4(bv[1], 0.f), vec4(bv[2], 0.f), vec4(bv[3], 1.f));
    const vec3 dir = vec3(m * vec4(rd.dir, 0.f));
    const vec3 origin = vec3(m * vec4(rd.origin, 1.f));
    const vec3 idir = vec3(
            1.f / (abs(dir.x) > EPS_BV_INTERSECT ? dir.x : EPS_BV_INTERSECT * sign(dir.x)),
            1.f / (abs(dir.y) > EPS_BV_INTERSECT ? dir.y : EPS_BV_INTERSECT * sign(dir.y)),
            1.f / (abs(dir.z) > EPS_BV_INTERSECT ? dir.z : EPS_BV_INTERSECT * sign(dir.z)));
    const vec3 ood = origin * idir;

    const vec2 unitCubeSlab = vec2(-.5f, .5f);
    const vec2 t0 = unitCubeSlab * idir.x - ood.x;
    const vec2 t1 = unitCubeSlab * idir.y - ood.y;
    const vec2 t2 = unitCubeSlab * idir.z - ood.z;

    return vec2(
        max(max(min(t0.x, t0.y), min(t1.x, t1.y)), max(min(t2.x, t2.y), rd.tmin)),
        min(min(max(t0.x, t0.y), max(t1.x, t1.y)), min(max(t2.x, t2.y), tmax))
    );
}
vec2 intersect(in mat4x3 bv, in RayDetail rd, in f32 tmax, out vec3 normal)
{
    const mat4 m = mat4(vec4(bv[0], 0.f), vec4(bv[1], 0.f), vec4(bv[2], 0.f), vec4(bv[3], 1.f));
    const vec3 dir = vec3(m * vec4(rd.dir, 0.f));
    const vec3 origin = vec3(m * vec4(rd.origin, 1.f));
    const vec3 idir = vec3(
            1.f / (abs(dir.x) > EPS_BV_INTERSECT ? dir.x : EPS_BV_INTERSECT * sign(dir.x)),
            1.f / (abs(dir.y) > EPS_BV_INTERSECT ? dir.y : EPS_BV_INTERSECT * sign(dir.y)),
            1.f / (abs(dir.z) > EPS_BV_INTERSECT ? dir.z : EPS_BV_INTERSECT * sign(dir.z)));
    const vec3 ood = origin * idir;

    const vec2 unitCubeSlab = vec2(-.5f, .5f);
    const vec2 t0 = unitCubeSlab * idir.x - ood.x;
    const vec2 t1 = unitCubeSlab * idir.y - ood.y;
    const vec2 t2 = unitCubeSlab * idir.z - ood.z;

    vec2 result = vec2(
            max(max(min(t0.x, t0.y), min(t1.x, t1.y)), max(min(t2.x, t2.y), rd.tmin)),
            min(min(max(t0.x, t0.y), max(t1.x, t1.y)), min(max(t2.x, t2.y), tmax))
        );

    mat3 mi = inverse(mat3(m));
    if (result[0] == t0.x) normal = -normalize(mi[0]);
    if (result[0] == t0.y) normal = normalize(mi[0]);
    if (result[0] == t1.x) normal = -normalize(mi[1]);
    if (result[0] == t1.y) normal = normalize(mi[1]);
    if (result[0] == t2.x) normal = -normalize(mi[2]);
    if (result[0] == t2.y) normal = normalize(mi[2]);

    return result;
}
#endif

#ifdef INTERSECTION_SOBB
vec2 dSlabIntersect(in vec4 eSlab, in vec3 dir, in vec3 origin)
{
    vec2 slab = vec2(eSlab.w);
    slab.y += SLAB_SCALE;

    f32 dotResult = dot(eSlab.xyz, dir);
    return ((slab - dot(eSlab.xyz, origin)) / (abs(dotResult) > EPS_BV_INTERSECT ? dotResult : EPS_BV_INTERSECT * sign(dotResult)));
}

vec2 intersect(in SOBB bv, in RayDetail rd, in f32 tmax)
{
    const vec2 t0 = dSlabIntersect(bv.b0, rd.dir, rd.origin);
    const vec2 t1 = dSlabIntersect(bv.b1, rd.dir, rd.origin);
    const vec2 t2 = dSlabIntersect(bv.b2, rd.dir, rd.origin);
    return vec2(
        max(max(min(t0.x, t0.y), min(t1.x, t1.y)), max(min(t2.x, t2.y), rd.tmin)),
        min(min(max(t0.x, t0.y), max(t1.x, t1.y)), min(max(t2.x, t2.y), tmax))
    );
}
vec2 intersect(in SOBB bv, in RayDetail rd, in f32 tmax, out vec3 normal)
{
    const vec2 t0 = dSlabIntersect(bv.b0, rd.dir, rd.origin);
    const vec2 t1 = dSlabIntersect(bv.b1, rd.dir, rd.origin);
    const vec2 t2 = dSlabIntersect(bv.b2, rd.dir, rd.origin);

    vec2 result = vec2(
            max(max(min(t0.x, t0.y), min(t1.x, t1.y)), max(min(t2.x, t2.y), rd.tmin)),
            min(min(max(t0.x, t0.y), max(t1.x, t1.y)), min(max(t2.x, t2.y), tmax))
        );

    if (result[0] == t0.x) normal = -normalize(bv.b0.xyz);
    if (result[0] == t0.y) normal = normalize(bv.b0.xyz);
    if (result[0] == t1.x) normal = -normalize(bv.b1.xyz);
    if (result[0] == t1.y) normal = normalize(bv.b1.xyz);
    if (result[0] == t2.x) normal = -normalize(bv.b2.xyz);
    if (result[0] == t2.y) normal = normalize(bv.b2.xyz);

    return result;
}
#endif

#ifdef INTERSECTION_SOBBi
#define BVH_TYPE BVH2_SOBBi_c
vec2 testSlab(in vec3 normal, in vec2 slab, in vec3 dir, in vec3 origin)
{
    float dotResult = dot(normal, dir);
    return ((slab - dot(normal, origin)) / (abs(dotResult) > EPS_BV_INTERSECT ? dotResult : EPS_BV_INTERSECT * sign(dotResult)));
}

vec2 intersect(in SOBBi bv, in RayDetail rd, in f32 tmax)
{
    const i32 encodedIds = bv.normalIds;
    const vec2 t0 = testSlab(DOP_NORMALS[(encodedIds >> 20) & 0x3FF], bv.b0, rd.dir, rd.origin);
    const vec2 t1 = testSlab(DOP_NORMALS[(encodedIds >> 10) & 0x3FF], bv.b1, rd.dir, rd.origin);
    const vec2 t2 = testSlab(DOP_NORMALS[(encodedIds) & 0x3FF], bv.b2, rd.dir, rd.origin);
    return vec2(
        max(max(min(t0.x, t0.y), min(t1.x, t1.y)), max(min(t2.x, t2.y), rd.tmin)),
        min(min(max(t0.x, t0.y), max(t1.x, t1.y)), min(max(t2.x, t2.y), tmax))
    );
}

vec2 intersect(in SOBBi bv, in RayDetail rd, in f32 tmax, out vec3 normal)
{
    const i32 encodedIds = bv.normalIds;
    const vec2 t0 = testSlab(DOP_NORMALS[(encodedIds >> 20) & 0x3FF], bv.b0, rd.dir, rd.origin);
    const vec2 t1 = testSlab(DOP_NORMALS[(encodedIds >> 10) & 0x3FF], bv.b1, rd.dir, rd.origin);
    const vec2 t2 = testSlab(DOP_NORMALS[(encodedIds) & 0x3FF], bv.b2, rd.dir, rd.origin);

    vec2 result = vec2(
            max(max(min(t0.x, t0.y), min(t1.x, t1.y)), max(min(t2.x, t2.y), rd.tmin)),
            min(min(max(t0.x, t0.y), max(t1.x, t1.y)), min(max(t2.x, t2.y), tmax))
        );

    if (result[0] == t0.x) normal = -DOP_NORMALS[(encodedIds >> 20) & 0x3FF];
    if (result[0] == t0.y) normal = DOP_NORMALS[(encodedIds >> 20) & 0x3FF];
    if (result[0] == t1.x) normal = -DOP_NORMALS[(encodedIds >> 10) & 0x3FF];
    if (result[0] == t1.y) normal = DOP_NORMALS[(encodedIds >> 10) & 0x3FF];
    if (result[0] == t2.x) normal = -DOP_NORMALS[(encodedIds) & 0x3FF];
    if (result[0] == t2.y) normal = DOP_NORMALS[(encodedIds) & 0x3FF];

    return result;
}
#endif

#endif
