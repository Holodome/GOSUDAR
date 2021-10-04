/*
Author: Holodome
Date: 03.10.2021
File: src/math/mat.hhh
Version: 0
*/
#pragma once 
#include "lib/general.hh"

#include "math/vec.hhh"

struct mat4x4 {
    union {
        f32  e[4][4];
        vec4 v[4];
        f32  i[16];
        struct {
            f32 m00; f32 m01; f32 m02; f32 m03;
            f32 m10; f32 m11; f32 m12; f32 m13;
            f32 m20; f32 m21; f32 m22; f32 m23;
            f32 m30; f32 m31; f32 m32; f32 m33;
        };
    };
};

inline f32 *ValuePtr(mat4x4 mat) {
    return &mat.m00;
}

inline mat4x4 Mat4x4(vec4 v0, vec4 v1, vec4 v2, vec4 v3) {
    mat4x4 result;
    result.v[0] = v0;
    result.v[1] = v1;
    result.v[2] = v2;
    result.v[3] = v3;
    return result;
}

inline mat4x4 Identity() {
    mat4x4 result = {};
    result.m00 = result.m11 = result.m22 = result.m33 = 1.0f;
    return result;
}       

static mat4x4 Translate(vec3 t) {
    mat4x4 result = Identity();
    result.e[3][0] = t.x;
    result.e[3][1] = t.y;
    result.e[3][2] = t.z;
    return result;
}

inline mat4x4 Scale(vec3 s) {
    mat4x4 result = Mat4x4(Vec4(s.x,   0,   0,  0),
                           Vec4(0,   s.y,   0,  0),
                           Vec4(0,     0, s.z,  0),
                           Vec4(0,     0,   0,  1));
    return result;
}

inline mat4x4 XRotation(f32 angle) {
    const f32 c = Cos(angle);
    const f32 s = Sin(angle);
    mat4x4 r = Mat4x4(Vec4(1, 0, 0, 0),
                      Vec4(0, c,-s, 0),
                      Vec4(0, s, c, 0),
                      Vec4(0, 0, 0, 1));
    return(r);
}

inline mat4x4 YRotation(f32 angle) {
    const f32 c = Cos(angle);
    const f32 s = Sin(angle);
    mat4x4 r = Mat4x4(Vec4( c, 0, s, 0),
                      Vec4( 0, 1, 0, 0),
                      Vec4(-s, 0, c, 0),
                      Vec4( 0, 0, 0, 1));
    return(r);
}

inline mat4x4 ZRotation(f32 angle) {
    const f32 c = Cos(angle);
    const f32 s = Sin(angle);
    mat4x4 r = Mat4x4(Vec4(c,-s, 0, 0),
                      Vec4(s, c, 0, 0),
                      Vec4(0, 0, 1, 0),
                      Vec4(0, 0, 0, 1));
    return(r);
}

inline mat4x4 Rotation(f32 angle, vec3 a) {
    const f32 c = Cos(angle);
    const f32 s = Sin(angle);
    a = Normalize(a);
    
    const f32 tx = (1.0f - c) * a.x;
    const f32 ty = (1.0f - c) * a.y;
    const f32 tz = (1.0f - c) * a.z;
    
    mat4x4 r = Mat4x4(Vec4(c + tx * a.x, 		     tx * a.y - s * a.z, tx * a.z - s * a.y, 0),
                      Vec4(    ty * a.x - a.z * s, ty * a.y + c,       ty * a.z + s * a.x, 0),
                      Vec4(    tz * a.x + s * a.y, tz * a.y - s * a.x, tz * a.z + c,       0),
                      Vec4(0, 0, 0, 1));
    return(r);
}

inline mat4x4 Ortographic2d(f32 l, f32 r, f32 b, f32 t) {
    mat4x4 result = Mat4x4(Vec4(2.0f / (r - l),    0,                   0, 0),
                           Vec4(0,                 2.0f / (t - b),      0, 0),
                           Vec4(0,                 0,                  -1, 0),
                           Vec4(-(r + l) / (r - l), -(t + b) / (t - b), 0, 1));
    return result;
}

inline mat4x4 Ortographic3d(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
    mat4x4 result = Mat4x4(Vec4(2.0f / (r - l),    0,                   0,                 0),
                           Vec4(0,                 2.0f / (t - b),      0,                 0),
                           Vec4(0,                 0,                  -2.0f / (f - n),    0),
                           Vec4(-(r + l) / (r - l), -(t + b) / (t - b),-(f + n) / (f - n), 1)
                           );
    return result;
}

inline mat4x4 Perspective(f32 fov, f32 aspect, f32 n, f32 f) {
    const f32 toHf = tanf(fov * 0.5f);
    
    mat4x4 r = Mat4x4(Vec4(1.0f / (aspect * toHf), 0,           0,                         0),
                      Vec4(0,                      1.0f / toHf, 0,                         0),
                      Vec4(0,                      0,          -       (f + n) / (f - n), -1),
                      Vec4(0,                      0,          -2.0f * (f * n) / (f - n),  0));
    return(r);
}

inline mat4x4 Inverse(mat4x4 m) {
    f32 coef00 = m.e[2][2] * m.e[3][3] - m.e[3][2] * m.e[2][3];
    f32 coef02 = m.e[1][2] * m.e[3][3] - m.e[3][2] * m.e[1][3];
    f32 coef03 = m.e[1][2] * m.e[2][3] - m.e[2][2] * m.e[1][3];
    f32 coef04 = m.e[2][1] * m.e[3][3] - m.e[3][1] * m.e[2][3];
    f32 coef06 = m.e[1][1] * m.e[3][3] - m.e[3][1] * m.e[1][3];
    f32 coef07 = m.e[1][1] * m.e[2][3] - m.e[2][1] * m.e[1][3];
    f32 coef08 = m.e[2][1] * m.e[3][2] - m.e[3][1] * m.e[2][2];
    f32 coef10 = m.e[1][1] * m.e[3][2] - m.e[3][1] * m.e[1][2];
    f32 coef11 = m.e[1][1] * m.e[2][2] - m.e[2][1] * m.e[1][2];
    f32 coef12 = m.e[2][0] * m.e[3][3] - m.e[3][0] * m.e[2][3];
    f32 coef14 = m.e[1][0] * m.e[3][3] - m.e[3][0] * m.e[1][3];
    f32 coef15 = m.e[1][0] * m.e[2][3] - m.e[2][0] * m.e[1][3];
    f32 coef16 = m.e[2][0] * m.e[3][2] - m.e[3][0] * m.e[2][2];
    f32 coef18 = m.e[1][0] * m.e[3][2] - m.e[3][0] * m.e[1][2];
    f32 coef19 = m.e[1][0] * m.e[2][2] - m.e[2][0] * m.e[1][2];
    f32 coef20 = m.e[2][0] * m.e[3][1] - m.e[3][0] * m.e[2][1];
    f32 coef22 = m.e[1][0] * m.e[3][1] - m.e[3][0] * m.e[1][1];
    f32 coef23 = m.e[1][0] * m.e[2][1] - m.e[2][0] * m.e[1][1];
    
    vec4 fac0 = Vec4(coef00, coef00, coef02, coef03);
    vec4 fac1 = Vec4(coef04, coef04, coef06, coef07);
    vec4 fac2 = Vec4(coef08, coef08, coef10, coef11);
    vec4 fac3 = Vec4(coef12, coef12, coef14, coef15);
    vec4 fac4 = Vec4(coef16, coef16, coef18, coef19);
    vec4 fac5 = Vec4(coef20, coef20, coef22, coef23);
    
    vec4 vec0 = Vec4(m.e[1][0], m.e[0][0], m.e[0][0], m.e[0][0]);
    vec4 vec1 = Vec4(m.e[1][1], m.e[0][1], m.e[0][1], m.e[0][1]);
    vec4 vec2 = Vec4(m.e[1][2], m.e[0][2], m.e[0][2], m.e[0][2]);
    vec4 vec3 = Vec4(m.e[1][3], m.e[0][3], m.e[0][3], m.e[0][3]);
    
    vec4 inv0 = (vec1 * fac0) - (vec2 * fac1) + (vec3 * fac2);
    vec4 inv1 = (vec0 * fac0) - (vec2 * fac3) + (vec3 * fac4);
    vec4 inv2 = (vec0 * fac1) - (vec1 * fac3) + (vec3 * fac5);
    vec4 inv3 = (vec0 * fac2) - (vec1 * fac4) + (vec2 * fac5);
    
    const vec4 sign_a = Vec4(1, -1,  1, -1);
    const vec4 sign_b = Vec4(-1,  1, -1,  1);
    
    mat4x4 inverse;
    for(u32 i = 0; 
        i < 4;
        ++i) {
        inverse.e[0][i] = inv0.e[i] * sign_a.e[i];
        inverse.e[1][i] = inv1.e[i] * sign_b.e[i];
        inverse.e[2][i] = inv2.e[i] * sign_a.e[i];
        inverse.e[3][i] = inv3.e[i] * sign_b.e[i];
    }
    
    vec4 row0 = Vec4(inverse.e[0][0], inverse.e[1][0], inverse.e[2][0], inverse.e[3][0]);
    vec4 m0   = Vec4(m.e[0][0],       m.e[0][1],       m.e[0][2],       m.e[0][3]      );
    vec4 dot0 = m0 * row0;
    f32 dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);
    
    f32 one_over_det = 1.0f / dot1;
    
    for (u32 i = 0; i < 16; ++i) {
        inverse.i[i] *= one_over_det;
    }
    return inverse;
}

inline mat4x4 LookAt(vec3 from, vec3 to) {
    vec3 z = Normalize(to - from);
    vec3 x = Normalize(Cross(Vec3(0, 1, 0), z));
    vec3 y = Cross(z, x);
    mat4x4 result = Identity();
    result.e[0][0] = x.x;
    result.e[1][0] = x.y;
    result.e[2][0] = x.z;
    result.e[3][0] = -Dot(x, from);
    result.e[0][1] = y.x;
    result.e[1][1] = y.y;
    result.e[2][1] = y.z;
    result.e[3][1] = -Dot(y, from);
    result.e[0][2] = z.x;
    result.e[1][2] = z.y;
    result.e[2][2] = z.z;
    result.e[3][2] = -Dot(z, from);
    return result;
}

inline mat4x4 operator*(mat4x4 a, mat4x4 b) {
    mat4x4 result;
    for(int r = 0; r < 4; ++r) {
        for(int c = 0; c < 4; ++c) {
            result.e[r][c] = a.e[0][c] * b.e[r][0]
                + a.e[1][c] * b.e[r][1]
                + a.e[2][c] * b.e[r][2]
                + a.e[3][c] * b.e[r][3];
        }
    }
    return result;
}

inline vec4 operator*(mat4x4 a, vec4 v) {
    vec4 result;
    result.x = a.e[0][0] * v.x + a.e[1][0] * v.y + a.e[2][0] * v.z + a.e[3][0] * v.w;
    result.y = a.e[0][1] * v.x + a.e[1][1] * v.y + a.e[2][1] * v.z + a.e[3][1] * v.w;
    result.z = a.e[0][2] * v.x + a.e[1][2] * v.y + a.e[2][2] * v.z + a.e[3][2] * v.w;
    result.w = a.e[0][3] * v.x + a.e[1][3] * v.y + a.e[2][3] * v.z + a.e[3][3] * v.w;
    return result;
}

inline mat4x4 &operator*=(mat4x4 &a, mat4x4 b) {
    return (a = a * b);
}

inline vec3 GetX(mat4x4 mat) {
    return Vec3(mat.e[0][0], mat.e[1][0], mat.e[2][0]);
}

inline vec3 GetY(mat4x4 mat) {
    return Vec3(mat.e[0][1], mat.e[1][1], mat.e[2][1]);
}

inline vec3 GetZ(mat4x4 mat) {
    return Vec3(mat.e[0][2], mat.e[1][2], mat.e[2][2]);
}
