#ifndef BV_DOP14_AREA_GLSL
#define BV_DOP14_AREA_GLSL

// surface area of a DOP14 by corner cutting
//   to improve numeric stability for certain scenes with small coordinates,
//   we scale the DOP14 by 1e3 and return the result scaled by 1e-6
f32 bvArea(in f32[14] dop)
{
    for (i32 i = 0; i < 14; i++)
        dop[i] *= 1e3;

    vec3 diag = vec3((dop[1] - dop[0]), (dop[3] - dop[2]), (dop[5] - dop[4]));
    f32 result = 2.f * (diag.x * diag.y + diag.x * diag.z + diag.z * diag.y);

    // fast path for dummy dop (max area)
    if (dop[0] <= -1e30f && dop[1] >= 1e30f)
        return result;

    f32 d[8];
    d[0] = dop[6] - dot_dop14_n3(vec3(dop[0], dop[2], dop[4]));
    d[1] = dot_dop14_n3(vec3(dop[1], dop[3], dop[5])) - dop[7];
    d[2] = dop[8] - dot_dop14_n4(vec3(dop[0], dop[2], dop[5]));
    d[3] = dot_dop14_n4(vec3(dop[1], dop[3], dop[4])) - dop[9];
    d[4] = dop[10] - dot_dop14_n5(vec3(dop[0], dop[3], dop[4]));
    d[5] = dot_dop14_n5(vec3(dop[1], dop[2], dop[5])) - dop[11];
    d[6] = dop[12] - dot_dop14_n6(vec3(dop[0], dop[3], dop[5]));
    d[7] = dot_dop14_n6(vec3(dop[1], dop[2], dop[4])) - dop[13];

    // dop normals are not normalized, so we need to multiply by 1/sqrt(3)
    f32 accToSubtract = d[0] * d[0] + d[1] * d[1] + d[2] * d[2] + d[3] * d[3] + d[4] * d[4] + d[5] * d[5] + d[6] * d[6] + d[7] * d[7];
    accToSubtract *= .6339745962155614f;

    f32 s[12];
    // X pairs: 0:7, 1:6, 2:5, 3:4
    s[0] = max(0.f, d[0] + d[7] - diag.x);
    s[1] = max(0.f, d[1] + d[6] - diag.x);
    s[2] = max(0.f, d[2] + d[5] - diag.x);
    s[3] = max(0.f, d[3] + d[4] - diag.x);
    // Y pairs: 0:4, 1:5, 2:6, 3:7
    s[4] = max(0.f, d[0] + d[4] - diag.y);
    s[5] = max(0.f, d[1] + d[5] - diag.y);
    s[6] = max(0.f, d[2] + d[6] - diag.y);
    s[7] = max(0.f, d[3] + d[7] - diag.y);
    // Z pairs: 0:2, 1:3, 4:6, 5:7
    s[8] = max(0.f, d[0] + d[2] - diag.z);
    s[9] = max(0.f, d[1] + d[3] - diag.z);
    s[10] = max(0.f, d[4] + d[6] - diag.z);
    s[11] = max(0.f, d[5] + d[7] - diag.z);

    f32 accToAdd = s[0] * s[0] + s[1] * s[1] + s[2] * s[2] + s[3] * s[3]
            + s[4] * s[4] + s[5] * s[5] + s[6] * s[6] + s[7] * s[7]
            + s[8] * s[8] + s[9] * s[9] + s[10] * s[10] + s[11] * s[11];
    accToAdd *= .06698729810778067f;

    return (result - accToSubtract + accToAdd) * 1e-6;
}

#endif
