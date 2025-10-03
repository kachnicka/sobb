#ifndef BV_DOP_GLSL
#define BV_DOP_GLSL

#extension GL_EXT_control_flow_attributes: require
#define UNROLL_NEXT_LOOP [[unroll]]

#define dop_min(dop, a) (dop[2 * (a)])
#define dop_max(dop, a) (dop[2 * (a) + 1])
#define dop_slab_d(dop, a) ((dop_max(dop, a)) - (dop_min(dop, a)))

#ifdef DOP_32
#define HAVE_DOP
#define DOP_SIZE 32
#define DOP_SLABS 16

vec3 DOP_NORMALS[DOP_SLABS] = {
        {
            1.0000000,
            0.0000000,
            0.0000000
        },
        {
            0.0000000,
            1.0000000,
            0.0000000
        },
        {
            0.0000000,
            0.0000000,
            1.0000000
        },
        {
            0.5773500,
            0.5773500,
            0.5773500
        },
        {
            0.5773500,
            0.5773500,
            -0.5773500
        },
        {
            0.5773500,
            -0.5773500,
            0.5773500
        },
        {
            0.5773500,
            -0.5773500,
            -0.5773500
        },
        {
            0.8832792,
            0.2360192,
            0.4051083
        },
        {
            0.5164694,
            -0.1144751,
            -0.8486195
        },
        {
            0.8587868,
            0.1252680,
            -0.4967829
        },
        {
            0.5534041,
            -0.0057654,
            0.8328929
        },
        {
            0.1407341,
            -0.8645653,
            -0.4824112
        },
        {
            0.5901749,
            -0.8072627,
            -0.0045526
        },
        {
            0.7372449,
            0.6756153,
            0.0037376
        },
        {
            0.8835392,
            -0.2404428,
            0.4019275
        },
        {
            0.0025974,
            -0.5183646,
            -0.8551558
        },
    };
#endif

#ifdef DOP_48
#define HAVE_DOP
#define DOP_SIZE 48
#define DOP_SLABS 24

vec3 DOP_NORMALS[DOP_SLABS] = {
        {
            1.0000000,
            0.0000000,
            0.0000000
        },
        {
            0.0000000,
            1.0000000,
            0.0000000
        },
        {
            0.0000000,
            0.0000000,
            1.0000000
        },
        {
            0.5773500,
            0.5773500,
            0.5773500
        },
        {
            0.5773500,
            0.5773500,
            -0.5773500
        },
        {
            0.5773500,
            -0.5773500,
            0.5773500
        },
        {
            0.5773500,
            -0.5773500,
            -0.5773500
        },
        {
            0.3151027,
            -0.8890942,
            -0.3319970
        },
        {
            0.3744746,
            0.8879738,
            -0.2669666
        },
        {
            0.5549889,
            -0.1636851,
            0.8155946
        },
        {
            0.0026004,
            0.5628635,
            0.8265458
        },
        {
            0.2936770,
            0.3534005,
            -0.8881789
        },
        {
            0.5281368,
            0.8283544,
            0.1868171
        },
        {
            0.8809254,
            -0.0775734,
            0.4668540
        },
        {
            0.2596817,
            -0.8822397,
            0.3927068
        },
        {
            0.5877236,
            -0.8069243,
            0.0587744
        },
        {
            0.8886316,
            0.3715994,
            0.2687894
        },
        {
            0.1723558,
            -0.4482670,
            0.8771262
        },
        {
            0.8838728,
            -0.4390940,
            0.1611376
        },
        {
            0.3741660,
            0.2800074,
            0.8840790
        },
        {
            0.1237812,
            0.7427050,
            -0.6580786
        },
        {
            0.1380939,
            0.8700653,
            0.4731982
        },
        {
            0.8101774,
            -0.0044429,
            -0.5861679
        },
        {
            0.7952373,
            0.5894910,
            -0.1417668
        },
    };
#endif

#ifdef DOP_64
#define HAVE_DOP
#define DOP_SIZE 64
#define DOP_SLABS 32

vec3 DOP_NORMALS[DOP_SLABS] = {
        {
            1.0000000,
            0.0000000,
            0.0000000
        },
        {
            0.0000000,
            1.0000000,
            0.0000000
        },
        {
            0.0000000,
            0.0000000,
            1.0000000
        },
        {
            0.5773500,
            0.5773500,
            0.5773500
        },
        {
            0.5773500,
            0.5773500,
            -0.5773500
        },
        {
            0.5773500,
            -0.5773500,
            0.5773500
        },
        {
            0.5773500,
            -0.5773500,
            -0.5773500
        },
        {
            0.8781121,
            0.4222081,
            0.2250765
        },
        {
            0.4189224,
            0.9013987,
            0.1094726
        },
        {
            0.3942707,
            0.3350082,
            0.8557570
        },
        {
            0.6506034,
            -0.7361511,
            0.1865395
        },
        {
            0.4086958,
            -0.9040436,
            -0.1251910
        },
        {
            0.8960515,
            0.3126782,
            -0.3151574
        },
        {
            0.6998061,
            -0.1960467,
            0.6869040
        },
        {
            0.9188009,
            -0.0382776,
            0.3928610
        },
        {
            0.7262040,
            0.2162261,
            0.6525903
        },
        {
            0.5429468,
            0.1993937,
            -0.8157517
        },
        {
            0.2861640,
            -0.7912006,
            -0.5404736
        },
        {
            0.3952306,
            -0.0781091,
            0.9152551
        },
        {
            0.0039680,
            0.9378997,
            0.3468838
        },
        {
            0.0101463,
            0.6762564,
            0.7365965
        },
        {
            0.2592854,
            -0.8995353,
            0.3515786
        },
        {
            0.0001768,
            0.3569931,
            0.9341070
        },
        {
            0.1543407,
            -0.6660178,
            0.7297938
        },
        {
            0.9061446,
            -0.3975331,
            0.1444632
        },
        {
            0.4407558,
            -0.2489748,
            -0.8624070
        },
        {
            0.2349309,
            0.8781776,
            -0.4166673
        },
        {
            0.8643772,
            -0.4023126,
            -0.3016564
        },
        {
            0.1655813,
            0.3981661,
            -0.9022452
        },
        {
            0.7067736,
            0.6912504,
            -0.1504794
        },
        {
            0.2925457,
            0.7942019,
            0.5325980
        },
        {
            0.8164847,
            -0.0584882,
            -0.5743970
        },
    };
#endif

#ifdef HAVE_DOP

#define DOP f32[DOP_SIZE]

layout(buffer_reference, scalar) buffer DOP_ref {
    DOP val[];
};

DOP dopInit()
{
    DOP dop;
    UNROLL_NEXT_LOOP
    for (i32 i = 0; i < DOP_SLABS; i++) {
        dop_min(dop, i) = BIG_FLOAT;
        dop_max(dop, i) = -BIG_FLOAT;
    }
    return dop;
}

DOP dopInit(in vec3 v)
{
    DOP dop;
    f32 d;
    UNROLL_NEXT_LOOP
    for (i32 i = 0; i < DOP_SLABS; i++) {
        d = dot(DOP_NORMALS[i], v);
        dop_min(dop, i) = d;
        dop_max(dop, i) = d;
    }
    return dop;
}

DOP dopInit(in vec3 v0, in vec3 v1, in vec3 v2)
{
    DOP dop;
    f32 d0, d1, d2;
    UNROLL_NEXT_LOOP
    for (i32 i = 0; i < DOP_SLABS; i++) {
        d0 = dot(DOP_NORMALS[i], v0);
        d1 = dot(DOP_NORMALS[i], v1);
        d2 = dot(DOP_NORMALS[i], v2);
        dop_min(dop, i) = min(d0, min(d1, d2));
        dop_max(dop, i) = max(d0, max(d1, d2));
    }
    return dop;
}

void bvFit(inout DOP dop, in vec3 v)
{
    f32 d;
    UNROLL_NEXT_LOOP
    for (i32 i = 0; i < DOP_SLABS; i++) {
        d = dot(DOP_NORMALS[i], v);
        dop_min(dop, i) = min(dop_min(dop, i), d);
        dop_max(dop, i) = max(dop_max(dop, i), d);
    }
}

void bvFit(inout DOP dop, in vec3 v0, in vec3 v1, in vec3 v2)
{
    f32 d0, d1, d2;
    UNROLL_NEXT_LOOP
    for (i32 i = 0; i < DOP_SLABS; i++) {
        d0 = dot(DOP_NORMALS[i], v0);
        d1 = dot(DOP_NORMALS[i], v1);
        d2 = dot(DOP_NORMALS[i], v2);
        dop_min(dop, i) = min(min(dop_min(dop, i), d0), min(d1, d2));
        dop_max(dop, i) = max(max(dop_max(dop, i), d0), max(d1, d2));
    }
}

void bvFit(inout DOP dop, in DOP dopToFit)
{
    UNROLL_NEXT_LOOP
    for (i32 i = 0; i < DOP_SLABS; i++) {
        dop_min(dop, i) = min(dop_min(dop, i), dop_min(dopToFit, i));
        dop_max(dop, i) = max(dop_max(dop, i), dop_max(dopToFit, i));
    }
}
#endif

#ifdef DOP_14
#define DOP_SIZE 14
#define DOP_SLABS 7

vec3 DOP_NORMALS[DOP_SLABS] = {
        {
            1.0000000,
            0.0000000,
            0.0000000
        },
        {
            0.0000000,
            1.0000000,
            0.0000000
        },
        {
            0.0000000,
            0.0000000,
            1.0000000
        },
        {
            0.5773500,
            0.5773500,
            0.5773500
        },
        {
            0.5773500,
            0.5773500,
            -0.5773500
        },
        {
            0.5773500,
            -0.5773500,
            0.5773500
        },
        {
            0.5773500,
            -0.5773500,
            -0.5773500
        },
    };

#define DOP f32[DOP_SIZE]
#define VtxIds u32[DOP_SIZE]

f32 dot_dop14_n0(in vec3 v) {
    return v.x;
}
f32 dot_dop14_n1(in vec3 v) {
    return v.y;
}
f32 dot_dop14_n2(in vec3 v) {
    return v.z;
}
f32 dot_dop14_n3(in vec3 v) {
    return (v.x + v.y + v.z);
}
f32 dot_dop14_n4(in vec3 v) {
    return (v.x + v.y - v.z);
}
f32 dot_dop14_n5(in vec3 v) {
    return (v.x - v.y + v.z);
}
f32 dot_dop14_n6(in vec3 v) {
    return (v.x - v.y - v.z);
}

#extension GL_EXT_control_flow_attributes: require
#define UNROLL_NEXT_LOOP [[unroll]]

DOP dopInitWithVertId(in vec3 v, in u32 vId, out VtxIds vIds)
{
    UNROLL_NEXT_LOOP
    for (i32 i = 0; i < DOP_SIZE; ++i)
        vIds[i] = vId;

    DOP dop;
    dop[0] = dot_dop14_n0(v);
    dop[1] = dop[0];
    dop[2] = dot_dop14_n1(v);
    dop[3] = dop[2];
    dop[4] = dot_dop14_n2(v);
    dop[5] = dop[4];
    dop[6] = dot_dop14_n3(v);
    dop[7] = dop[6];
    dop[8] = dot_dop14_n4(v);
    dop[9] = dop[8];
    dop[10] = dot_dop14_n5(v);
    dop[11] = dop[10];
    dop[12] = dot_dop14_n6(v);
    dop[13] = dop[12];
    return dop;
}

void bvFitWithVertId(inout DOP dop, in vec3 v, in u32 vId, inout VtxIds vIds)
{
    f32 d;
    d = dot_dop14_n0(v);
    dop[0] = min(dop[0], d);
    if (d == dop[0]) vIds[0] = vId;
    dop[1] = max(dop[1], d);
    if (d == dop[1]) vIds[1] = vId;

    d = dot_dop14_n1(v);
    dop[2] = min(dop[2], d);
    if (d == dop[2]) vIds[2] = vId;
    dop[3] = max(dop[3], d);
    if (d == dop[3]) vIds[3] = vId;

    d = dot_dop14_n2(v);
    dop[4] = min(dop[4], d);
    if (d == dop[4]) vIds[4] = vId;
    dop[5] = max(dop[5], d);
    if (d == dop[5]) vIds[5] = vId;

    d = dot_dop14_n3(v);
    dop[6] = min(dop[6], d);
    if (d == dop[6]) vIds[6] = vId;
    dop[7] = max(dop[7], d);
    if (d == dop[7]) vIds[7] = vId;

    d = dot_dop14_n4(v);
    dop[8] = min(dop[8], d);
    if (d == dop[8]) vIds[8] = vId;
    dop[9] = max(dop[9], d);
    if (d == dop[9]) vIds[9] = vId;

    d = dot_dop14_n5(v);
    dop[10] = min(dop[10], d);
    if (d == dop[10]) vIds[10] = vId;
    dop[11] = max(dop[11], d);
    if (d == dop[11]) vIds[11] = vId;

    d = dot_dop14_n6(v);
    dop[12] = min(dop[12], d);
    if (d == dop[12]) vIds[12] = vId;
    dop[13] = max(dop[13], d);
    if (d == dop[13]) vIds[13] = vId;
}

void bvFitWithVertId(inout DOP dop, inout VtxIds vIds, in DOP dopToFit, in VtxIds vIdsToFit)
{
    UNROLL_NEXT_LOOP
    for (i32 i = 0; i < DOP_SLABS; i++) {
        if (dop_min(dopToFit, i) < dop_min(dop, i)) {
            dop_min(dop, i) = dop_min(dopToFit, i);
            dop_min(vIds, i) = dop_min(vIdsToFit, i);
        }
        if (dop_max(dopToFit, i) > dop_max(dop, i)) {
            dop_max(dop, i) = dop_max(dopToFit, i);
            dop_max(vIds, i) = dop_max(vIdsToFit, i);
        }
    }
}

DOP dopInit()
{
    DOP dop;
    UNROLL_NEXT_LOOP
    for (i32 i = 0; i < DOP_SLABS; i++) {
        dop_min(dop, i) = BIG_FLOAT;
        dop_max(dop, i) = -BIG_FLOAT;
    }
    return dop;
}

DOP dopInit(in vec3 v)
{
    DOP dop;
    dop[0] = dot_dop14_n0(v);
    dop[1] = dop[0];
    dop[2] = dot_dop14_n1(v);
    dop[3] = dop[2];
    dop[4] = dot_dop14_n2(v);
    dop[5] = dop[4];
    dop[6] = dot_dop14_n3(v);
    dop[7] = dop[6];
    dop[8] = dot_dop14_n4(v);
    dop[9] = dop[8];
    dop[10] = dot_dop14_n5(v);
    dop[11] = dop[10];
    dop[12] = dot_dop14_n6(v);
    dop[13] = dop[12];
    return dop;
}

DOP dopInit(in vec3 v0, in vec3 v1, in vec3 v2)
{
    DOP dop;
    f32 d0, d1, d2;

    d0 = dot_dop14_n0(v0);
    d1 = dot_dop14_n0(v1);
    d2 = dot_dop14_n0(v2);
    dop_min(dop, 0) = min(d0, min(d1, d2));
    dop_max(dop, 0) = max(d0, max(d1, d2));

    d0 = dot_dop14_n1(v0);
    d1 = dot_dop14_n1(v1);
    d2 = dot_dop14_n1(v2);
    dop_min(dop, 1) = min(d0, min(d1, d2));
    dop_max(dop, 1) = max(d0, max(d1, d2));

    d0 = dot_dop14_n2(v0);
    d1 = dot_dop14_n2(v1);
    d2 = dot_dop14_n2(v2);
    dop_min(dop, 2) = min(d0, min(d1, d2));
    dop_max(dop, 2) = max(d0, max(d1, d2));

    d0 = dot_dop14_n3(v0);
    d1 = dot_dop14_n3(v1);
    d2 = dot_dop14_n3(v2);
    dop_min(dop, 3) = min(d0, min(d1, d2));
    dop_max(dop, 3) = max(d0, max(d1, d2));

    d0 = dot_dop14_n4(v0);
    d1 = dot_dop14_n4(v1);
    d2 = dot_dop14_n4(v2);
    dop_min(dop, 4) = min(d0, min(d1, d2));
    dop_max(dop, 4) = max(d0, max(d1, d2));

    d0 = dot_dop14_n5(v0);
    d1 = dot_dop14_n5(v1);
    d2 = dot_dop14_n5(v2);
    dop_min(dop, 5) = min(d0, min(d1, d2));
    dop_max(dop, 5) = max(d0, max(d1, d2));

    d0 = dot_dop14_n6(v0);
    d1 = dot_dop14_n6(v1);
    d2 = dot_dop14_n6(v2);
    dop_min(dop, 6) = min(d0, min(d1, d2));
    dop_max(dop, 6) = max(d0, max(d1, d2));

    return dop;
}

void bvFit(inout DOP dop, in vec3 v)
{
    f32 d;

    d = dot_dop14_n0(v);
    dop_min(dop, 0) = min(dop_min(dop, 0), d);
    dop_max(dop, 0) = max(dop_max(dop, 0), d);

    d = dot_dop14_n1(v);
    dop_min(dop, 1) = min(dop_min(dop, 1), d);
    dop_max(dop, 1) = max(dop_max(dop, 1), d);

    d = dot_dop14_n2(v);
    dop_min(dop, 2) = min(dop_min(dop, 2), d);
    dop_max(dop, 2) = max(dop_max(dop, 2), d);

    d = dot_dop14_n3(v);
    dop_min(dop, 3) = min(dop_min(dop, 3), d);
    dop_max(dop, 3) = max(dop_max(dop, 3), d);

    d = dot_dop14_n4(v);
    dop_min(dop, 4) = min(dop_min(dop, 4), d);
    dop_max(dop, 4) = max(dop_max(dop, 4), d);

    d = dot_dop14_n5(v);
    dop_min(dop, 5) = min(dop_min(dop, 5), d);
    dop_max(dop, 5) = max(dop_max(dop, 5), d);

    d = dot_dop14_n6(v);
    dop_min(dop, 6) = min(dop_min(dop, 6), d);
    dop_max(dop, 6) = max(dop_max(dop, 6), d);
}

void bvFit(inout DOP dop, in vec3 v0, in vec3 v1, in vec3 v2)
{
    f32 d0, d1, d2;

    d0 = dot_dop14_n0(v0);
    d1 = dot_dop14_n0(v1);
    d2 = dot_dop14_n0(v2);
    dop_min(dop, 0) = min(min(dop_min(dop, 0), d0), min(d1, d2));
    dop_max(dop, 0) = max(max(dop_max(dop, 0), d0), max(d1, d2));

    d0 = dot_dop14_n1(v0);
    d1 = dot_dop14_n1(v1);
    d2 = dot_dop14_n1(v2);
    dop_min(dop, 1) = min(min(dop_min(dop, 1), d0), min(d1, d2));
    dop_max(dop, 1) = max(max(dop_max(dop, 1), d0), max(d1, d2));

    d0 = dot_dop14_n2(v0);
    d1 = dot_dop14_n2(v1);
    d2 = dot_dop14_n2(v2);
    dop_min(dop, 2) = min(min(dop_min(dop, 2), d0), min(d1, d2));
    dop_max(dop, 2) = max(max(dop_max(dop, 2), d0), max(d1, d2));

    d0 = dot_dop14_n3(v0);
    d1 = dot_dop14_n3(v1);
    d2 = dot_dop14_n3(v2);
    dop_min(dop, 3) = min(min(dop_min(dop, 3), d0), min(d1, d2));
    dop_max(dop, 3) = max(max(dop_max(dop, 3), d0), max(d1, d2));

    d0 = dot_dop14_n4(v0);
    d1 = dot_dop14_n4(v1);
    d2 = dot_dop14_n4(v2);
    dop_min(dop, 4) = min(min(dop_min(dop, 4), d0), min(d1, d2));
    dop_max(dop, 4) = max(max(dop_max(dop, 4), d0), max(d1, d2));

    d0 = dot_dop14_n5(v0);
    d1 = dot_dop14_n5(v1);
    d2 = dot_dop14_n5(v2);
    dop_min(dop, 5) = min(min(dop_min(dop, 5), d0), min(d1, d2));
    dop_max(dop, 5) = max(max(dop_max(dop, 5), d0), max(d1, d2));

    d0 = dot_dop14_n6(v0);
    d1 = dot_dop14_n6(v1);
    d2 = dot_dop14_n6(v2);
    dop_min(dop, 6) = min(min(dop_min(dop, 6), d0), min(d1, d2));
    dop_max(dop, 6) = max(max(dop_max(dop, 6), d0), max(d1, d2));
}

void bvFit(inout DOP dop, in DOP dopToFit)
{
    UNROLL_NEXT_LOOP
    for (i32 i = 0; i < DOP_SLABS; i++) {
        dop_min(dop, i) = min(dop_min(dop, i), dop_min(dopToFit, i));
        dop_max(dop, i) = max(dop_max(dop, i), dop_max(dopToFit, i));
    }
}

vec3 aabbCentroid(in DOP dop)
{
    return .5f * vec3((dop[0] + dop[1]), (dop[2] + dop[3]), (dop[4] + dop[5]));
}

DOP dummyDOP14()
{
    DOP dop;
    dop[0] = -1e30f;
    dop[1] = 1e30f;
    dop[2] = -1e30f;
    dop[3] = 1e30f;
    dop[4] = -1e30f;
    dop[5] = 1e30f;
    dop[6] = -1.73205e30f;
    dop[7] = 1.73205e30f;
    dop[8] = -5.7735e29f;
    dop[9] = 5.7735e29f;
    dop[10] = -5.7735e29f;
    dop[11] = 5.7735e29f;
    dop[12] = -5.7735e29f;
    dop[13] = 5.7735e29f;
    return dop;
}

#include "bv_dop14_area.glsl"

// void bvFit_wDop14(inout f32 dop[DOP_SIZE], in f32[14] dop14, in vec3 v)
// {
//     UNROLL_NEXT_LOOP
//     for (i32 i = 0; i < 14; i++)
//         dop[i] = dop14[i];
//
//     f32 d;
//     UNROLL_NEXT_LOOP
//     for (i32 i = 7; i < DOP_SLABS; i++) {
//         d = dot(DOP_NORMALS[i], v);
//         dop_min(dop, i) = min(dop_min(dop, i), d);
//         dop_max(dop, i) = max(dop_max(dop, i), d);
//     }
// }
//
// void bvFit_wDop14(inout f32 dop[DOP_SIZE], in f32[14] dop14, in f32 dopToFit[DOP_SIZE])
// {
//     UNROLL_NEXT_LOOP
//     for (i32 i = 0; i < 14; i++)
//         dop[i] = dop14[i];
//
//     UNROLL_NEXT_LOOP
//     for (i32 i = 7; i < DOP_SLABS; i++) {
//         dop_min(dop, i) = min(dop_min(dop, i), dop_min(dopToFit, i));
//         dop_max(dop, i) = max(dop_max(dop, i), dop_max(dopToFit, i));
//     }
// }
//
// void bvFit_wAabb(inout f32 dop[DOP_SIZE], in vec3 boxmin, in vec3 boxmax, in vec3 v)
// {
//     dop[0] = boxmin.x;
//     dop[1] = boxmax.x;
//     dop[2] = boxmin.y;
//     dop[3] = boxmax.y;
//     dop[4] = boxmin.z;
//     dop[5] = boxmax.z;
//
//     f32 d;
//     UNROLL_NEXT_LOOP
//     for (i32 i = 3; i < DOP_SLABS; i++) {
//         d = dot(DOP_NORMALS[i], v);
//         dop_min(dop, i) = min(dop_min(dop, i), d);
//         dop_max(dop, i) = max(dop_max(dop, i), d);
//     }
// }
//
// void bvFit_woAabb(inout f32 dop[DOP_SIZE], in vec3 v)
// {
//     f32 d;
//     UNROLL_NEXT_LOOP
//     for (i32 i = 3; i < DOP_SLABS; i++) {
//         d = dot(DOP_NORMALS[i], v);
//         dop_min(dop, i) = min(dop_min(dop, i), d);
//         dop_max(dop, i) = max(dop_max(dop, i), d);
//     }
// }
//
// void bvFit_wAabb(inout f32 dop[DOP_SIZE], in vec3 boxmin, in vec3 boxmax, in f32 dopToFit[DOP_SIZE])
// {
//     dop[0] = boxmin.x;
//     dop[1] = boxmax.x;
//     dop[2] = boxmin.y;
//     dop[3] = boxmax.y;
//     dop[4] = boxmin.z;
//     dop[5] = boxmax.z;
//
//     UNROLL_NEXT_LOOP
//     for (i32 i = 3; i < DOP_SLABS; i++) {
//         dop_min(dop, i) = min(dop_min(dop, i), dop_min(dopToFit, i));
//         dop_max(dop, i) = max(dop_max(dop, i), dop_max(dopToFit, i));
//     }
// }
//
// void bvFitWithDOP14Vertices(inout f32 dop[DOP_SIZE], in vec3 v, out vec3[14] projectedVerts)
// {
//     f32 d;
//     UNROLL_NEXT_LOOP
//     for (i32 i = 0; i < 7; i++) {
//         d = dot(DOP_NORMALS[i], v);
//         dop_min(dop, i) = min(dop_min(dop, i), d);
//         if (d == dop_min(dop, i)) dop_min(projectedVerts, i) = v;
//         dop_max(dop, i) = max(dop_max(dop, i), d);
//         if (d == dop_max(dop, i)) dop_max(projectedVerts, i) = v;
//     }
//
//     UNROLL_NEXT_LOOP
//     for (i32 i = 7; i < DOP_SLABS; i++) {
//         d = dot(DOP_NORMALS[i], v);
//         dop_min(dop, i) = min(dop_min(dop, i), d);
//         dop_max(dop, i) = max(dop_max(dop, i), d);
//     }
// }
//
// vec3 aabbCentroid(in f32 dop[DOP_SIZE])
// {
//     return .5f * vec3((dop[0] + dop[1]), (dop[2] + dop[3]), (dop[4] + dop[5]));
// }
//
// u32 getBucket(f32 val)
// {
//     // if (val > 0.95f)
//     //     return 0;
//     // if (val > 0.90f)
//     //     return 1;
//     // if (val > 0.85f)
//     //     return 2;
//     // if (val > 0.80f)
//     //     return 3;
//     // return 4;
//     // return u32(trunc(det * 10.f))
//     if (val > 0.9f)
//         return 9;
//     if (val > 0.8f)
//         return 8;
//     if (val > 0.7f)
//         return 7;
//     if (val > 0.6f)
//         return 6;
//     if (val > 0.5f)
//         return 5;
//     if (val > 0.4f)
//         return 4;
//     if (val > 0.3f)
//         return 3;
//     if (val > 0.2f)
//         return 2;
//     if (val > 0.1f)
//         return 1;
//     return 0;
// }

#endif

// struct sOBB {
//     vec3 b0;
//     vec3 b1;
//     vec3 b2;
//     vec3 min;
//     vec3 max;
// };
//
// void sobbRefit(inout sOBB sobb, in vec3 v)
// {
//     vec3 proj = vec3(dot(v, sobb.b0), dot(v, sobb.b1), dot(v, sobb.b2));
//     sobb.min = min(sobb.min, proj);
//     sobb.max = max(sobb.max, proj);
// }
//
// f32 distancePoi32ToEdge(in vec3 poi32, in vec3 edgePoi32, in vec3 edgeDir)
// {
//     vec3 u = poi32 - edgePoi32;
//     f32 t = dot(edgeDir, u);
//     f32 distSq = dot(edgeDir, edgeDir);
//     return dot(u, u) - t * t / distSq;
// }
//
// // surface area
// f32 sobbCost(in sOBB sobb)
// {
//     const vec3 d = abs(sobb.max - sobb.min);
//     const f32 det = abs(-sobb.b0.z * sobb.b1.y * sobb.b2.x + sobb.b0.y * sobb.b1.z * sobb.b2.x + sobb.b0.z * sobb.b1.x * sobb.b2.y - sobb.b0.x * sobb.b1.z * sobb.b2.y - sobb.b0.y * sobb.b1.x * sobb.b2.z + sobb.b0.x * sobb.b1.y * sobb.b2.z);
//     return 2.f * (d.x * d.y + d.y * d.z + d.z * d.x) / det;
// }
//
// f32 extremalPoi32sDistance(in vec3[14] poi32s, in vec3 normalizedAxis)
// {
//     f32 minDist = dot(poi32s[0], normalizedAxis);
//     f32 maxDist = minDist;
//
//     UNROLL_NEXT_LOOP
//     for (i32 i = 1; i < 14; ++i) {
//         f32 dist = dot(poi32s[i], normalizedAxis);
//         minDist = min(minDist, dist);
//         maxDist = max(maxDist, dist);
//     }
//     return maxDist - minDist;
// }
//
// f32 extremalPoi32sDistance(in vec3[14] poi32s, in vec3 normalizedAxis, out ivec2 idx, out vec2 mDist)
// {
//     mDist.x = dot(poi32s[0], normalizedAxis);
//     mDist.y = mDist.x;
//     idx = ivec2(0, 0);
//
//     f32 dist;
//     UNROLL_NEXT_LOOP
//     for (i32 i = 1; i < 14; ++i) {
//         dist = dot(poi32s[i], normalizedAxis);
//         mDist.x = min(mDist.x, dist);
//         mDist.y = max(mDist.y, dist);
//         if (dist == mDist.x) idx.x = i;
//         if (dist == mDist.y) idx.y = i;
//     }
//
//     return mDist.y - mDist.x;
// }
//
// vec2 extremalPoi32sMinMax(in vec3[14] poi32s, in vec3 normalizedAxis)
// {
//     vec2 dMinMax = vec2(dot(poi32s[0], normalizedAxis));
//     f32 d;
//
//     UNROLL_NEXT_LOOP
//     for (i32 i = 1; i < 14; ++i) {
//         d = dot(poi32s[i], normalizedAxis);
//         dMinMax.x = min(dMinMax.x, d);
//         dMinMax.y = max(dMinMax.y, d);
//     }
//     return dMinMax;
// }
//
// sOBB sobbFromNormals(in vec3[14] poi32s, in vec3 n0, in vec3 n1, in vec3 n2)
// {
//     vec2 d0 = extremalPoi32sMinMax(poi32s, n0);
//     vec2 d1 = extremalPoi32sMinMax(poi32s, n1);
//     vec2 d2 = extremalPoi32sMinMax(poi32s, n2);
//     return sOBB(n0, n1, n2, vec3(d0.x, d1.x, d2.x), vec3(d0.y, d1.y, d2.y));
// }
//
// void sobbTest(in vec3[14] poi32s, in vec3 n0, in vec3 n1, in vec3 n2, inout f32 minCost, inout sOBB sobb)
// {
//     sOBB toTest = sobbFromNormals(poi32s, n0, n1, n2);
//     f32 testCost = sobbCost(toTest);
//     if (testCost < minCost) {
//         minCost = testCost;
//         sobb = toTest;
//     }
// }

// void sobbByDiTsO14(inout sOBB sobb, in vec3[14] poi32s)
// {
//     f32 minCost = sobbCost(sobb);
//
//     // find the ditetrahedron base triangle
//     // 1. find the two poi32s that are furthest apart
//     ivec3 baseTriangleIdx = ivec3(0, 1, 0);
//     vec3 dVec = poi32s[1] - poi32s[0];
//     f32 maxDistSq = dot(dVec, dVec);
//     for (i32 i = 1; i < 7; ++i) {
//         dVec = poi32s[i * 2 + 1] - poi32s[i * 2 + 0];
//         f32 distSq = dot(dVec, dVec);
//         if (distSq > maxDistSq) {
//             maxDistSq = distSq;
//             baseTriangleIdx.x = i * 2 + 0;
//             baseTriangleIdx.y = i * 2 + 1;
//         }
//     }
//     const vec3 p0 = poi32s[baseTriangleIdx.x];
//     const vec3 p1 = poi32s[baseTriangleIdx.y];
//     const vec3 e0 = normalize(p1 - p0);
//     // 2. find the poi32 furthest from the line between the two poi32s
//     maxDistSq = distancePoi32ToEdge(poi32s[0], p0, e0);
//     for (i32 i = 1; i < 14; ++i) {
//         f32 distSq = distancePoi32ToEdge(poi32s[i], p0, e0);
//         if (distSq > maxDistSq) {
//             maxDistSq = distSq;
//             baseTriangleIdx.z = i;
//         }
//     }
//     const vec3 p2 = poi32s[baseTriangleIdx.z];
//     const vec3 e1 = normalize(p2 - p0);
//     const vec3 e2 = normalize(p2 - p1);
//     // const vec3 normal = normalize(cross(e0, e1));
//
//     const vec3 n3 = normalize(cross(e0, e1));
//     const vec3 n0 = normalize(cross(e0, n3));
//     const vec3 n1 = normalize(cross(e1, n3));
//     const vec3 n2 = normalize(cross(e2, n3));
//
//     sobbTest(poi32s, n0, n1, n3, minCost, sobb);
//     sobbTest(poi32s, n1, n2, n3, minCost, sobb);
//     sobbTest(poi32s, n2, n0, n3, minCost, sobb);
//
//     // 3. find the top and bottom tetrahedron poi32s
//     ivec2 ditPoi32sIdx;
//     vec2 ditPoi32sDist;
//     f32 ditDist = extremalPoi32sDistance(poi32s, n3, ditPoi32sIdx, ditPoi32sDist);
//
//     const vec3 q0 = poi32s[ditPoi32sIdx.x];
//     const vec3 q1 = poi32s[ditPoi32sIdx.y];
//
//     if (abs(ditPoi32sDist.x) > .01f)
//     {
//         vec3 m0 = normalize(q0 - p0);
//         vec3 m1 = normalize(q0 - p1);
//         vec3 m2 = normalize(q0 - p2);
//         vec3 n10 = normalize(cross(m0, m1));
//         vec3 n11 = normalize(cross(m1, m2));
//         vec3 n12 = normalize(cross(m2, m0));
//
//         sobbTest(poi32s, n0, n1, n10, minCost, sobb);
//         sobbTest(poi32s, n0, n1, n11, minCost, sobb);
//         sobbTest(poi32s, n0, n1, n12, minCost, sobb);
//         sobbTest(poi32s, n1, n2, n10, minCost, sobb);
//         sobbTest(poi32s, n1, n2, n11, minCost, sobb);
//         sobbTest(poi32s, n1, n2, n12, minCost, sobb);
//         sobbTest(poi32s, n2, n0, n10, minCost, sobb);
//         sobbTest(poi32s, n2, n0, n11, minCost, sobb);
//         sobbTest(poi32s, n2, n0, n12, minCost, sobb);
//
//         sobbTest(poi32s, n10, n11, n0, minCost, sobb);
//         sobbTest(poi32s, n10, n11, n1, minCost, sobb);
//         sobbTest(poi32s, n10, n11, n2, minCost, sobb);
//         sobbTest(poi32s, n11, n12, n0, minCost, sobb);
//         sobbTest(poi32s, n11, n12, n1, minCost, sobb);
//         sobbTest(poi32s, n11, n12, n2, minCost, sobb);
//         sobbTest(poi32s, n12, n10, n0, minCost, sobb);
//         sobbTest(poi32s, n12, n10, n1, minCost, sobb);
//         sobbTest(poi32s, n12, n10, n2, minCost, sobb);
//     }
//     if (abs(ditPoi32sDist.y) > .01f)
//     {
//         vec3 m0 = normalize(q1 - p0);
//         vec3 m1 = normalize(q1 - p1);
//         vec3 m2 = normalize(q1 - p2);
//         vec3 n10 = normalize(cross(m0, m1));
//         vec3 n11 = normalize(cross(m1, m2));
//         vec3 n12 = normalize(cross(m2, m0));
//
//         sobbTest(poi32s, n0, n1, n10, minCost, sobb);
//         sobbTest(poi32s, n0, n1, n11, minCost, sobb);
//         sobbTest(poi32s, n0, n1, n12, minCost, sobb);
//         sobbTest(poi32s, n1, n2, n10, minCost, sobb);
//         sobbTest(poi32s, n1, n2, n11, minCost, sobb);
//         sobbTest(poi32s, n1, n2, n12, minCost, sobb);
//         sobbTest(poi32s, n2, n0, n10, minCost, sobb);
//         sobbTest(poi32s, n2, n0, n11, minCost, sobb);
//         sobbTest(poi32s, n2, n0, n12, minCost, sobb);
//
//         sobbTest(poi32s, n10, n11, n0, minCost, sobb);
//         sobbTest(poi32s, n10, n11, n1, minCost, sobb);
//         sobbTest(poi32s, n10, n11, n2, minCost, sobb);
//         sobbTest(poi32s, n11, n12, n0, minCost, sobb);
//         sobbTest(poi32s, n11, n12, n1, minCost, sobb);
//         sobbTest(poi32s, n11, n12, n2, minCost, sobb);
//         sobbTest(poi32s, n12, n10, n0, minCost, sobb);
//         sobbTest(poi32s, n12, n10, n1, minCost, sobb);
//         sobbTest(poi32s, n12, n10, n2, minCost, sobb);
//     }
// }

// // dop26 with normal coordinates [-1, 0, 1]
// f32 dot_dop26_n0(in vec3 v) {
//     return v.x;
// }
// f32 dot_dop26_n1(in vec3 v) {
//     return v.y;
// }
// f32 dot_dop26_n2(in vec3 v) {
//     return v.z;
// }
// f32 dot_dop26_n3(in vec3 v) {
//     return (v.x + v.y + v.z);
// }
// f32 dot_dop26_n4(in vec3 v) {
//     return (v.x + v.y - v.z);
// }
// f32 dot_dop26_n5(in vec3 v) {
//     return (v.x - v.y + v.z);
// }
// f32 dot_dop26_n6(in vec3 v) {
//     return (v.x - v.y - v.z);
// }
//
// f32 dot_dop26_n7(in vec3 v) {
//     return (v.x + v.y);
// }
// f32 dot_dop26_n8(in vec3 v) {
//     return (v.x - v.z);
// }
// f32 dot_dop26_n9(in vec3 v) {
//     return (v.x - v.y);
// }
// f32 dot_dop26_n10(in vec3 v) {
//     return (v.x + v.z);
// }
//
// f32 dot_dop26_n11(in vec3 v) {
//     return (v.y + v.z);
// }
// f32 dot_dop26_n12(in vec3 v) {
//     return (v.y - v.z);
// }

//f32[26] dop26Init(in vec3 v)
//{
//    f32 dop[26];
//    dop[0] = dot_dop26_n0(v);
//    dop[1] = dop[0];
//    dop[2] = dot_dop26_n1(v);
//    dop[3] = dop[2];
//    dop[4] = dot_dop26_n2(v);
//    dop[5] = dop[4];
//    dop[6] = dot_dop26_n3(v);
//    dop[7] = dop[6];
//    dop[8] = dot_dop26_n4(v);
//    dop[9] = dop[8];
//    dop[10] = dot_dop26_n5(v);
//    dop[11] = dop[10];
//    dop[12] = dot_dop26_n6(v);
//    dop[13] = dop[12];
//    return dop;
//}

// f32[26] dop26Init()
// {
//     f32 dop[26];
//     dop[0] = BIG_FLOAT;
//     dop[1] = -BIG_FLOAT;
//     dop[2] = BIG_FLOAT;
//     dop[3] = -BIG_FLOAT;
//     dop[4] = BIG_FLOAT;
//     dop[5] = -BIG_FLOAT;
//     dop[6] = BIG_FLOAT;
//     dop[7] = -BIG_FLOAT;
//     dop[8] = BIG_FLOAT;
//     dop[9] = -BIG_FLOAT;
//     dop[10] = BIG_FLOAT;
//     dop[11] = -BIG_FLOAT;
//     dop[12] = BIG_FLOAT;
//     dop[13] = -BIG_FLOAT;
//     dop[14] = BIG_FLOAT;
//     dop[15] = -BIG_FLOAT;
//     dop[16] = BIG_FLOAT;
//     dop[17] = -BIG_FLOAT;
//     dop[18] = BIG_FLOAT;
//     dop[19] = -BIG_FLOAT;
//     dop[20] = BIG_FLOAT;
//     dop[21] = -BIG_FLOAT;
//     dop[22] = BIG_FLOAT;
//     dop[23] = -BIG_FLOAT;
//     dop[24] = BIG_FLOAT;
//     dop[25] = -BIG_FLOAT;
//     return dop;
// }

//f32[26] dop26Dummy()
//{
//    f32 dop[26];
//    // result of dopFit(vec3(-BIG_FLOAT), vec3(BIG_FLOAT))
//    dop[0] = -1e30f;
//    dop[1] = 1e30f;
//    dop[2] = -1e30f;
//    dop[3] = 1e30f;
//    dop[4] = -1e30f;
//    dop[5] = 1e30f;
//    dop[6] = -1.73205e30f;
//    dop[7] = 1.73205e30f;
//    dop[8] = -5.7735e29f;
//    dop[9] = 5.7735e29f;
//    dop[10] = -5.7735e29f;
//    dop[11] = 5.7735e29f;
//    dop[12] = -5.7735e29f;
//    dop[13] = 5.7735e29f;
//
//    return dop;
//}

// void bvFit(inout f32 dop[26], in vec3 v)
// {
//     f32 d;
//     d = dot_dop26_n0(v);
//     dop[0] = min(dop[0], d);
//     dop[1] = max(dop[1], d);
//     d = dot_dop26_n1(v);
//     dop[2] = min(dop[2], d);
//     dop[3] = max(dop[3], d);
//     d = dot_dop26_n2(v);
//     dop[4] = min(dop[4], d);
//     dop[5] = max(dop[5], d);
//     d = dot_dop26_n3(v);
//     dop[6] = min(dop[6], d);
//     dop[7] = max(dop[7], d);
//     d = dot_dop26_n4(v);
//     dop[8] = min(dop[8], d);
//     dop[9] = max(dop[9], d);
//     d = dot_dop26_n5(v);
//     dop[10] = min(dop[10], d);
//     dop[11] = max(dop[11], d);
//     d = dot_dop26_n6(v);
//     dop[12] = min(dop[12], d);
//     dop[13] = max(dop[13], d);
//     d = dot_dop26_n7(v);
//     dop[14] = min(dop[14], d);
//     dop[15] = max(dop[15], d);
//     d = dot_dop26_n8(v);
//     dop[16] = min(dop[16], d);
//     dop[17] = max(dop[17], d);
//     d = dot_dop26_n9(v);
//     dop[18] = min(dop[18], d);
//     dop[19] = max(dop[19], d);
//     d = dot_dop26_n10(v);
//     dop[20] = min(dop[20], d);
//     dop[21] = max(dop[21], d);
//     d = dot_dop26_n11(v);
//     dop[22] = min(dop[22], d);
//     dop[23] = max(dop[23], d);
//     d = dot_dop26_n12(v);
//     dop[24] = min(dop[24], d);
//     dop[25] = max(dop[25], d);
// }

//f32[26] dop26Init(in vec3 v0, in vec3 v1, in vec3 v2)
//{
//    f32[26] dop;
//    dop[0] = min(dot_dop26_n0(v0), min(dot_dop26_n0(v1), dot_dop26_n0(v2)));
//    dop[1] = max(dot_dop26_n0(v0), max(dot_dop26_n0(v1), dot_dop26_n0(v2)));
//    dop[2] = min(dot_dop26_n1(v0), min(dot_dop26_n1(v1), dot_dop26_n1(v2)));
//    dop[3] = max(dot_dop26_n1(v0), max(dot_dop26_n1(v1), dot_dop26_n1(v2)));
//    dop[4] = min(dot_dop26_n2(v0), min(dot_dop26_n2(v1), dot_dop26_n2(v2)));
//    dop[5] = max(dot_dop26_n2(v0), max(dot_dop26_n2(v1), dot_dop26_n2(v2)));
//    dop[6] = min(dot_dop26_n3(v0), min(dot_dop26_n3(v1), dot_dop26_n3(v2)));
//    dop[7] = max(dot_dop26_n3(v0), max(dot_dop26_n3(v1), dot_dop26_n3(v2)));
//    dop[8] = min(dot_dop26_n4(v0), min(dot_dop26_n4(v1), dot_dop26_n4(v2)));
//    dop[9] = max(dot_dop26_n4(v0), max(dot_dop26_n4(v1), dot_dop26_n4(v2)));
//    dop[10] = min(dot_dop26_n5(v0), min(dot_dop26_n5(v1), dot_dop26_n5(v2)));
//    dop[11] = max(dot_dop26_n5(v0), max(dot_dop26_n5(v1), dot_dop26_n5(v2)));
//    dop[12] = min(dot_dop26_n6(v0), min(dot_dop26_n6(v1), dot_dop26_n6(v2)));
//    dop[13] = max(dot_dop26_n6(v0), max(dot_dop26_n6(v1), dot_dop26_n6(v2)));
//    return dop;
//}
//
// void bvFit(inout f32 dop[26], in f32 dopToFit[26])
// {
//     dop[0] = min(dop[0], dopToFit[0]);
//     dop[1] = max(dop[1], dopToFit[1]);
//     dop[2] = min(dop[2], dopToFit[2]);
//     dop[3] = max(dop[3], dopToFit[3]);
//     dop[4] = min(dop[4], dopToFit[4]);
//     dop[5] = max(dop[5], dopToFit[5]);
//     dop[6] = min(dop[6], dopToFit[6]);
//     dop[7] = max(dop[7], dopToFit[7]);
//     dop[8] = min(dop[8], dopToFit[8]);
//     dop[9] = max(dop[9], dopToFit[9]);
//     dop[10] = min(dop[10], dopToFit[10]);
//     dop[11] = max(dop[11], dopToFit[11]);
//     dop[12] = min(dop[12], dopToFit[12]);
//     dop[13] = max(dop[13], dopToFit[13]);
//     dop[14] = min(dop[14], dopToFit[14]);
//     dop[15] = max(dop[15], dopToFit[15]);
//     dop[16] = min(dop[16], dopToFit[16]);
//     dop[17] = max(dop[17], dopToFit[17]);
//     dop[18] = min(dop[18], dopToFit[18]);
//     dop[19] = max(dop[19], dopToFit[19]);
//     dop[20] = min(dop[20], dopToFit[20]);
//     dop[21] = max(dop[21], dopToFit[21]);
//     dop[22] = min(dop[22], dopToFit[22]);
//     dop[23] = max(dop[23], dopToFit[23]);
//     dop[24] = min(dop[24], dopToFit[24]);
//     dop[25] = max(dop[25], dopToFit[25]);
// }

// f32[26] overlapDop(in f32 dop[26], in f32 dop1[26])
// {
//     dop[0] = max(dop[0], dop1[0]);
//     dop[2] = max(dop[2], dop1[2]);
//     dop[4] = max(dop[4], dop1[4]);
//     dop[6] = max(dop[6], dop1[6]);
//     dop[8] = max(dop[8], dop1[8]);
//     dop[10] = max(dop[10], dop1[10]);
//     dop[12] = max(dop[12], dop1[12]);
//     dop[1] = min(dop[1], dop1[1]);
//     dop[3] = min(dop[3], dop1[3]);
//     dop[5] = min(dop[5], dop1[5]);
//     dop[7] = min(dop[7], dop1[7]);
//     dop[9] = min(dop[9], dop1[9]);
//     dop[11] = min(dop[11], dop1[11]);
//     dop[13] = min(dop[13], dop1[13]);
//
//     if (dop[0] > dop[1] || dop[2] > dop[3] || dop[4] > dop[5] || dop[6] > dop[7] || dop[8] > dop[9] || dop[10] > dop[11] || dop[12] > dop[13])
//         dop = f32[26](0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
//     return dop;
// }

// vec3 aabbCentroid(in f32 dop[26])
// {
//     return .5f * vec3((dop[0] + dop[1]), (dop[2] + dop[3]), (dop[4] + dop[5]));
// }
//
// f32 aabbSurfaceArea(in f32 dop[26])
// {
//     vec3 d = vec3((dop[1] - dop[0]), (dop[3] - dop[2]), (dop[5] - dop[4]));
//     return 2.f * (d.x * d.y + d.x * d.z + d.z * d.y);
// }
//
// f32 FitSOBB3(in f32[26] dop, out i32 besti, out i32 bestj, out i32 bestk)
// {
//     f32 d[13];
//     besti = 0;
//     for (i32 i = 0; i < 13; i++) {
//         d[i] = dop[2 * i + 1] - dop[2 * i + 0];
//         if (d[i] < d[besti])
//             besti = i;
//     }
//     f32 best = BIG_FLOAT;
//     for (i32 j = 0; j < 13; j++)
//         if (j != besti) {
//             f32 dd = d[j] / (1.0f - (sqrt(dot(DOP26_NORMAL[j], DOP26_NORMAL[besti]))));
//             // f32 dd = d[j] / (1.0f - (abs(dot(DOP26_NORMAL[j], DOP26_NORMAL[besti]))));
//             if (dd < best) {
//                 best = dd;
//                 bestj = j;
//             }
//         }
//
//     f32 bestArea = BIG_FLOAT;
//     f32 a1 = d[besti] * d[bestj];
//     f32 a2 = d[besti] + d[bestj];
//     for (i32 k = 0; k < 13; k++)
//         if (k != besti && k != bestj) {
//             const vec3 n1 = DOP26_NORMAL[besti];
//             const vec3 n2 = DOP26_NORMAL[bestj];
//             const vec3 n3 = DOP26_NORMAL[k];
//             f32 det = abs(-n1.z * n2.y * n3.x + n1.y * n2.z * n3.x + n1.z * n2.x * n3.y - n1.x * n2.z * n3.y - n1.y * n2.x * n3.z + n1.x * n2.y * n3.z);
//             f32 area = a1 + d[k] * a2;
//             area = 2 * abs(area) / det;
//             if (area < bestArea) {
//                 bestArea = area;
//                 bestk = k;
//             }
//         }
//     return bestArea;
// }
//
// f32 FitSOBB_FULL(in f32[26] dop, out i32 besti, out i32 bestj, out i32 bestk)
// {
//     f32 bestArea = BIG_FLOAT;
//     i32 tests = 0;
//     for (i32 i = 0; i < 13; i++)
//         for (i32 j = i + 1; j < 13; j++)
//             for (i32 k = j + 1; k < 13; k++)
//             //                        if (i!=j && i!=k && j!=k) {
//             {
//                 //tests ++;
//                 const vec3 n1 = DOP26_NORMAL[i];
//                 const vec3 n2 = DOP26_NORMAL[j];
//                 const vec3 n3 = DOP26_NORMAL[k];
//                 f32 det = abs(-n1.z * n2.y * n3.x + n1.y * n2.z * n3.x + n1.z * n2.x * n3.y - n1.x * n2.z * n3.y - n1.y * n2.x * n3.z + n1.x * n2.y * n3.z);
//                 f32 area = (dop[2 * i + 1] - dop[2 * i + 0]) * (dop[2 * j + 1] - dop[2 * j + 0]) + (dop[2 * i + 1] - dop[2 * i + 0]) * (dop[2 * k + 1] - dop[2 * k + 0]) + (dop[2 * k + 1] - dop[2 * k + 0]) * (dop[2 * j + 1] - dop[2 * j + 0]);
//                 area = 2 * abs(area) / det;
//                 if (area < bestArea) {
//                     bestArea = area;
//                     besti = i;
//                     bestj = j;
//                     bestk = k;
//                 }
//             }
//     //cout<<tests<<" ";
//     return bestArea;
// }

#endif
