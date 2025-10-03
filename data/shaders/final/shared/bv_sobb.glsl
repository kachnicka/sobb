#ifndef BV_SOBB_GLSL
#define BV_SOBB_GLSL

#extension GL_EXT_control_flow_attributes: require
#define UNROLL_NEXT_LOOP [[unroll]]

struct SOBB {
    vec4 b0;
    vec4 b1;
    vec4 b2;
};

struct SOBBi {
    vec2 b0;
    vec2 b1;
    vec2 b2;
    i32 normalIds;
};

#define EPS_D_SLAB 1e-7f
#define SLAB_SCALE 1e3f
// #define SLAB_SCALE 1.f

vec4 dSlabEncode(in vec3 n, in f32 min_, in f32 max_)
{
    f32 k = SLAB_SCALE / max(max_ - min_, EPS_D_SLAB);
    return vec4(n, min_) * k;
}

vec4 dSlabEncode(in vec3 n, in vec3 v0, in vec3 v1)
{
    const f32 d0 = dot(n, v0);
    const f32 d1 = dot(n, v1);
    return dSlabEncode(n, min(d0, d1), max(d0, d1));
}

vec4 dSlabEncodeWider(in vec3 n, in vec3 v0, in vec3 v1)
{
    const f32 d0 = dot(n, v0);
    const f32 d1 = dot(n, v1);
    const f32 min_ = min(d0, d1) - 1e-7f;
    const f32 max_ = max(d0, d1) + 1e-7f;

    return dSlabEncode(n, min_, max_);
}

f32 bvArea(in vec4 b0, in vec4 b1, in vec4 b2)
{
    const vec3 n1 = normalize(b0.xyz);
    const vec3 n2 = normalize(b1.xyz);
    const vec3 n3 = normalize(b2.xyz);
    f32 det = abs(-n1.z * n2.y * n3.x + n1.y * n2.z * n3.x + n1.z * n2.x * n3.y - n1.x * n2.z * n3.y - n1.y * n2.x * n3.z + n1.x * n2.y * n3.z);

    const f32 d0 = SLAB_SCALE / max(length(b0.xyz), EPS_D_SLAB);
    const f32 d1 = SLAB_SCALE / max(length(b1.xyz), EPS_D_SLAB);
    const f32 d2 = SLAB_SCALE / max(length(b2.xyz), EPS_D_SLAB);

    f32 area = d0 * d1 + d0 * d2 + d1 * d2;
    area = 2.f * abs(area) / det;
    if (isinf(area))
        return 0.f;
    return area;
}

f32 bvArea(in SOBB sobb)
{
    return bvArea(sobb.b0, sobb.b1, sobb.b2);
}

#ifdef HAVE_DOP

f32 bvArea(in SOBBi sobb)
{
    const vec3 n1 = DOP_NORMALS[(sobb.normalIds >> 20) & 0x3FF];
    const vec3 n2 = DOP_NORMALS[(sobb.normalIds >> 10) & 0x3FF];
    const vec3 n3 = DOP_NORMALS[(sobb.normalIds) & 0x3FF];
    f32 det = abs(-n1.z * n2.y * n3.x + n1.y * n2.z * n3.x + n1.z * n2.x * n3.y - n1.x * n2.z * n3.y - n1.y * n2.x * n3.z + n1.x * n2.y * n3.z);

    const f32 d0 = sobb.b0.y - sobb.b0.x;
    const f32 d1 = sobb.b1.y - sobb.b1.x;
    const f32 d2 = sobb.b2.y - sobb.b2.x;

    f32 area = d0 * d1 + d0 * d2 + d1 * d2;
    area = 2.f * abs(area) / det;
    if (isinf(area))
        return 0.f;
    return area;
}

// #define FitSOBB FitSOBB_k3
#define FitSOBB FitSOBB_k2
// #define FitSOBB FitSOBB_k1

f32 FitSOBB_k1(in DOP dop, out i32 besti, out i32 bestj, out i32 bestk)
{
    f32 d[DOP_SLABS];
    besti = 0;

    UNROLL_NEXT_LOOP
    for (i32 i = 0; i < DOP_SLABS; i++) {
        d[i] = dop_slab_d(dop, i);
        if (d[i] < d[besti])
            besti = i;
    }

    f32 best = BIG_FLOAT;

    UNROLL_NEXT_LOOP
    for (i32 j = 0; j < DOP_SLABS; j++)
        if (j != besti) {
            f32 cosTheta = dot(DOP_NORMALS[j], DOP_NORMALS[besti]);
            f32 dd = d[j] * d[j] / (1.f - cosTheta * cosTheta);
            if (dd < best) {
                best = dd;
                bestj = j;
            }
        }

    const vec3 n1 = DOP_NORMALS[besti];
    const vec3 n2 = DOP_NORMALS[bestj];
    const f32 detX = n1.y * n2.z - n1.z * n2.y;
    const f32 detY = n1.z * n2.x - n1.x * n2.z;
    const f32 detZ = n1.x * n2.y - n1.y * n2.x;

    f32 bestArea = BIG_FLOAT;
    f32 a1 = d[besti] * d[bestj];
    f32 a2 = d[besti] + d[bestj];

    UNROLL_NEXT_LOOP
    for (i32 k = 0; k < DOP_SLABS; k++)
        if (k != besti && k != bestj) {
            const vec3 n3 = DOP_NORMALS[k];
            const f32 det = abs(detX * n3.x + detY * n3.y + detZ * n3.z);
            f32 area = a1 + d[k] * a2;
            area = 2 * abs(area) / det;
            if (area < bestArea) {
                bestArea = area;
                bestk = k;
            }
        }
    return bestArea;
}

f32 FitSOBB_k2(in DOP dop, out i32 besti, out i32 bestj, out i32 bestk)
{
    f32 d[DOP_SLABS];
    besti = 0;
    bestj = 1;
    bestk = 2;

    UNROLL_NEXT_LOOP
    for (i32 i = 0; i < DOP_SLABS; i++) {
        d[i] = dop_slab_d(dop, i);
        if (d[i] < d[besti])
            besti = i;
    }
    const vec3 n1 = DOP_NORMALS[besti];

    f32 bestArea = BIG_FLOAT;
    UNROLL_NEXT_LOOP
    for (i32 j = 0; j < DOP_SLABS; j++) {
        if (j != besti) {
            const vec3 n2 = DOP_NORMALS[j];
            const f32 detX = n1.y * n2.z - n1.z * n2.y;
            const f32 detY = n1.z * n2.x - n1.x * n2.z;
            const f32 detZ = n1.x * n2.y - n1.y * n2.x;
            const f32 a1 = d[besti] * d[j];
            const f32 a2 = d[besti] + d[j];

            UNROLL_NEXT_LOOP
            for (i32 k = 0; k < DOP_SLABS; k++)
                if (k != besti && k != j) {
                    const vec3 n3 = DOP_NORMALS[k];
                    const f32 det = abs(detX * n3.x + detY * n3.y + detZ * n3.z);
                    {
                        f32 area = a1 + d[k] * a2;
                        area = 2 * abs(area) / det;
                        if (area < bestArea) {
                            bestArea = area;
                            bestk = k;
                            bestj = j;
                        }
                    }
                }
        }
    }
    return bestArea;
}

f32 FitSOBB_k3(in DOP dop, out i32 besti, out i32 bestj, out i32 bestk)
{
    f32 bestArea = BIG_FLOAT;
    i32 tests = 0;
    for (i32 i = 0; i < DOP_SLABS; i++)
        for (i32 j = i + 1; j < DOP_SLABS; j++)
            for (i32 k = j + 1; k < DOP_SLABS; k++)
            {
                const vec3 n1 = DOP_NORMALS[i];
                const vec3 n2 = DOP_NORMALS[j];
                const vec3 n3 = DOP_NORMALS[k];
                f32 det = abs(-n1.z * n2.y * n3.x + n1.y * n2.z * n3.x + n1.z * n2.x * n3.y - n1.x * n2.z * n3.y - n1.y * n2.x * n3.z + n1.x * n2.y * n3.z);
                const f32 d0 = dop_slab_d(dop, i);
                const f32 d1 = dop_slab_d(dop, j);
                const f32 d2 = dop_slab_d(dop, k);
                f32 area = d0 * d1 + d0 * d2 + d1 * d2;
                area = 2 * abs(area) / det;
                if (area < bestArea) {
                    bestArea = area;
                    besti = i;
                    bestj = j;
                    bestk = k;
                }
            }
    return bestArea;
}

#endif

#endif
