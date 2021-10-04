/*
Author: Holodome
Date: 04.10.2021
File: shared/math/mat4.h
Version: 0
*/
#pragma once
#include "general.h"


typedef union {
    f32  e[4][4];
    struct {
        f32 m00; f32 m01; f32 m02; f32 m03;
        f32 m10; f32 m11; f32 m12; f32 m13;
        f32 m20; f32 m21; f32 m22; f32 m23;
        f32 m30; f32 m31; f32 m32; f32 m33;
    };
    f32 i[16];
} Mat4x4;

const Mat4x4 MAT4X4_IDENTITY = { 
    .m00 = 1,
    .m11 = 1,
    .m22 = 1,
    .m33 = 1
};

Mat4x4
mat4x4_translate(Vec3 t) {
	Mat4x4 result = MAT4X4_IDENTITY;
    result.e[3][0] = t.x;
    result.e[3][1] = t.y;
    result.e[3][2] = t.z;
	return result;
}

Mat4x4
mat4x4_scale(Vec3 s) {
	Mat4x4 result = {{
        {s.x,   0,   0,  0},
        {0,   s.y,   0,  0},
        {0,     0, s.z,  0},
        {0,     0,   0,  1},
    }};
	return result;
}

Mat4x4
mat4x4_rotation_x(f32 angle) {
	const f32 c = cosf(angle);
	const f32 s = sinf(angle);
	Mat4x4 r = {{
		{1, 0, 0, 0},
		{0, c,-s, 0},
		{0, s, c, 0},
		{0, 0, 0, 1}
	}};
	return(r);
}

Mat4x4
mat4x4_rotation_y(f32 angle) {
	const f32 c = cosf(angle);
	const f32 s = sinf(angle);
	Mat4x4 r = {{
		{ c, 0, s, 0},
		{ 0, 1, 0, 0},
		{-s, 0, c, 0},
		{ 0, 0, 0, 1}
	}};
	return(r);
}

Mat4x4
mat4x4_rotation_z(f32 angle) {
	const f32 c = cosf(angle);
	const f32 s = sinf(angle);
	Mat4x4 r = {{
		{c,-s, 0, 0},
		{s, c, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1}
	}};
	return(r);
}

Mat4x4
mat4x4_rotation(f32 angle, Vec3 a) {
	const f32 c = cosf(angle);
	const f32 s = sinf(angle);
	a = normalize(a);

	const f32 tx = (1.0f - c) * a.x;
	const f32 ty = (1.0f - c) * a.y;
	const f32 tz = (1.0f - c) * a.z;

	Mat4x4 r = {{
		{c + tx * a.x, 		     tx * a.y - s * a.z, tx * a.z - s * a.y, 0},
		{    ty * a.x - a.z * s, ty * a.y + c,       ty * a.z + s * a.x, 0},
		{    tz * a.x + s * a.y, tz * a.y - s * a.x, tz * a.z + c,       0},
		{0, 0, 0, 1}
	}};
	return(r);
}

Mat4x4
mat4x4_ortographic_2d(f32 l, f32 r, f32 b, f32 t) {
	Mat4x4 result =	{{
		{2.0f / (r - l),    0,                   0, 0},
		{0,                 2.0f / (t - b),      0, 0},
		{0,                 0,                  -1, 0},
		{-(r + l) / (r - l), -(t + b) / (t - b), 0, 1}
	}};
	return result;
}

Mat4x4
mat4x4_ortographic_3d(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
	Mat4x4 result =	{{
		{2.0f / (r - l),    0,                   0,                 0},
		{0,                 2.0f / (t - b),      0,                 0},
		{0,                 0,                  -2.0f / (f - n),    0},
		{-(r + l) / (r - l), -(t + b) / (t - b),-(f + n) / (f - n), 1}
	}};
	return result;
}

Mat4x4
mat4x4_perspective(f32 fov, f32 aspect, f32 n, f32 f) {
	const f32 toHf = tanf(fov * 0.5f);

	Mat4x4 r = {{
		{1.0f / (aspect * toHf), 0,           0,                         0},
		{0,                      1.0f / toHf, 0,                         0},
		{0,                      0,          -       (f + n) / (f - n), -1},
		{0,                      0,          -2.0f * (f * n) / (f - n),  0}
	}};
	return(r);
}

Mat4x4
mat4x4_mul(Mat4x4 a, Mat4x4 b) {
	Mat4x4 result;
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

Mat4x4
mat4x4_inverse(Mat4x4 m) {
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
    
    Vec4 fac0 = { .x = coef00, .y = coef00, .z = coef02, .w = coef03 };
    Vec4 fac1 = { .x = coef04, .y = coef04, .z = coef06, .w = coef07 };
    Vec4 fac2 = { .x = coef08, .y = coef08, .z = coef10, .w = coef11 };
    Vec4 fac3 = { .x = coef12, .y = coef12, .z = coef14, .w = coef15 };
    Vec4 fac4 = { .x = coef16, .y = coef16, .z = coef18, .w = coef19 };
    Vec4 fac5 = { .x = coef20, .y = coef20, .z = coef22, .w = coef23 };
    
    Vec4 vec0 = { .x = m.e[1][0], .y = m.e[0][0], .z = m.e[0][0], .w = m.e[0][0] };
    Vec4 vec1 = { .x = m.e[1][1], .y = m.e[0][1], .z = m.e[0][1], .w = m.e[0][1] };
    Vec4 vec2 = { .x = m.e[1][2], .y = m.e[0][2], .z = m.e[0][2], .w = m.e[0][2] };
    Vec4 vec3 = { .x = m.e[1][3], .y = m.e[0][3], .z = m.e[0][3], .w = m.e[0][3] };
    
    Vec4 inv0 = v4add(v4sub(v4mul(vec1, fac0), v4mul(vec2, fac1)), v4mul(vec3, fac2));
    Vec4 inv1 = v4add(v4sub(v4mul(vec0, fac0), v4mul(vec2, fac3)), v4mul(vec3, fac4));
    Vec4 inv2 = v4add(v4sub(v4mul(vec0, fac1), v4mul(vec1, fac3)), v4mul(vec3, fac5));
    Vec4 inv3 = v4add(v4sub(v4mul(vec0, fac2), v4mul(vec1, fac4)), v4mul(vec2, fac5));
    
    const Vec4 sign_a = { .x = 1, .y =-1, .z = 1, .w = -1 };
    const Vec4 sign_b = { .x =-1, .y = 1, .z =-1, .w =  1 };
    
    Mat4x4 inverse;
    for(u32 i = 0; 
        i < 4;
        ++i) {
        inverse.e[0][i] = inv0.e[i] * sign_a.e[i];
        inverse.e[1][i] = inv1.e[i] * sign_b.e[i];
        inverse.e[2][i] = inv2.e[i] * sign_a.e[i];
        inverse.e[3][i] = inv3.e[i] * sign_b.e[i];
    }
    
    Vec4 row0 = { .x = inverse.e[0][0], .y = inverse.e[1][0], .z = inverse.e[2][0], .w = inverse.e[3][0] };
    Vec4 m0   = { .x = m.e[0][0],       .y = m.e[0][1],       .z = m.e[0][2],       .w = m.e[0][3]       };
    Vec4 dot0 = v4mul(m0, row0);
    f32 dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);
    
    f32 one_over_det = 1.0f / dot1;
    
    for (u32 i = 0; i < 16; ++i) {
        inverse.i[i] *= one_over_det;
    }
    return inverse;
}

Vec3
mat4x4_mul_vec3(Mat4x4 m, Vec3 v) {
    f32 a = v.e[0] * m.e[0][0] + v.e[1] * m.e[1][0] + v.e[2] * m.e[2][0] + m.e[3][0]; 
    f32 b = v.e[0] * m.e[0][1] + v.e[1] * m.e[1][1] + v.e[2] * m.e[2][1] + m.e[3][1]; 
    f32 c = v.e[0] * m.e[0][2] + v.e[1] * m.e[1][2] + v.e[2] * m.e[2][2] + m.e[3][2]; 
    f32 w = v.e[0] * m.e[0][3] + v.e[1] * m.e[1][3] + v.e[2] * m.e[2][3] + m.e[3][3]; 
    
    f32 one_over_w = 1.0f / w;
    Vec3 result = v3(a * one_over_w, b * one_over_w, c * one_over_w);
    return result;
}

Vec3   
mat4x4_as_3x3_mul_vec3(Mat4x4 m, Vec3 v) {
    f32 a = v.e[0] * m.e[0][0] + v.e[1] * m.e[1][0] + v.e[2] * m.e[2][0]; 
    f32 b = v.e[0] * m.e[0][1] + v.e[1] * m.e[1][1] + v.e[2] * m.e[2][1]; 
    f32 c = v.e[0] * m.e[0][2] + v.e[1] * m.e[1][2] + v.e[2] * m.e[2][2]; 
    
    Vec3 result = v3(a, b, c);
    return result;    
}
