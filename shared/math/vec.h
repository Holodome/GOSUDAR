/*
Author: Holodome
Date: 04.10.2021
File: shared/math/vec.h
Version: 0
*/
#pragma once 
#include "general.h"

typedef union {
    struct {
        f32 x, y;
    };
    f32 e[2];
} Vec2;

Vec2 v2(f32 x, f32 y);
Vec2 v2s(f32 s);
Vec2 v2neg(Vec2 a);
Vec2 v2add(Vec2 a, Vec2 b);
Vec2 v2add3(Vec2 a, Vec2 b, Vec2 c);
Vec2 v2sub(Vec2 a, Vec2 b);
Vec2 v2div(Vec2 a, Vec2 b);
Vec2 v2mul(Vec2 a, Vec2 b);
Vec2 v2divs(Vec2 a, f32 b);
Vec2 v2muls(Vec2 a, f32 b);

typedef union {
    struct {
        f32 x, y, z;  
    };
    struct {
        f32 r, g, b;  
    };
    f32 e[3];
} Vec3;

Vec3 v3(f32 x, f32 y, f32 z);
Vec3 v3s(f32 s);
Vec3 v3neg(Vec3 a);
Vec3 v3add(Vec3 a, Vec3 b);
Vec3 v3add3(Vec3 a, Vec3 b, Vec3 c);
Vec3 v3sub(Vec3 a, Vec3 b);
Vec3 v3div(Vec3 a, Vec3 b);
Vec3 v3mul(Vec3 a, Vec3 b);
Vec3 v3divs(Vec3 a, f32 b);
Vec3 v3muls(Vec3 a, f32 b);
f32 dot(Vec3 a, Vec3 b);
Vec3 cross(Vec3 a, Vec3 b);
f32 length_sq(Vec3 a);
f32 length(Vec3 a);
Vec3 normalize(Vec3 a);
Vec3 v3lerp(Vec3 a, Vec3 b, f32 t);

typedef union {
    struct {
        f32 x, y, z, w;
    };
    f32 e[4];
} Vec4;

Vec4 v4neg(Vec4 a);
Vec4 v4add(Vec4 a, Vec4 b);
Vec4 v4sub(Vec4 a, Vec4 b);
Vec4 v4div(Vec4 a, Vec4 b);
Vec4 v4mul(Vec4 a, Vec4 b);
Vec4 v4divs(Vec4 a, f32 b);
Vec4 v4muls(Vec4 a, f32 b);
Vec4 v4(f32 x, f32 y, f32 z, f32 w);
Vec4 v4s(f32 s); 
f32 v4dot(Vec4 a, Vec4 b);
Vec4 v4normalize(Vec4 a);