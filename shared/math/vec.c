#include "math/vec.h"

#include "math/math.h"

Vec2 
v2(f32 x, f32 y) {
    Vec2 result;
    result.x = x;
    result.y = y;
    return result;
}

Vec2 
v2s(f32 s) {
    Vec2 result;
    result.x = s;
    result.y = s;
    return result;
}

Vec2 
v2neg(Vec2 a) {
    Vec2 result;
    result.x = -a.x;
    result.y = -a.y;
    return result;
}

Vec2 
v2add(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

Vec2
v2add3(Vec2 a, Vec2 b, Vec2 c) {
    return v2add(a, v2add(b, c));
}

Vec2 
v2sub(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

Vec2 
v2div(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    return result;
}

Vec2 
v2mul(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

Vec2 
v2divs(Vec2 a, f32 b) {
    Vec2 result;
    result.x = a.x / b;
    result.y = a.y / b;
    return result;
}

Vec2 
v2muls(Vec2 a, f32 b) {
    Vec2 result;
    result.x = a.x * b;
    result.y = a.y * b;
    return result;
}


Vec3 
v3(f32 x, f32 y, f32 z) {
    Vec3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

Vec3 
v3s(f32 s) {
    Vec3 result;
    result.x = s;
    result.y = s;
    result.z = s;
    return result;
}

Vec3 
v3neg(Vec3 a) {
    Vec3 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    return result;
}

Vec3 
v3add(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

Vec3
v3add3(Vec3 a, Vec3 b, Vec3 c) {
    return v3add(a, v3add(b, c));
}

Vec3 
v3sub(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

Vec3 
v3div(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    return result;
}

Vec3 
v3mul(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}

Vec3 
v3divs(Vec3 a, f32 b) {
    Vec3 result;
    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;
    return result;
}

Vec3 
v3muls(Vec3 a, f32 b) {
    Vec3 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    return result;
}

f32 
dot(Vec3 a, Vec3 b) {
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

Vec3 
cross(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

f32 
length_sq(Vec3 a) {
    f32 result = dot(a, a);
    return result;
}

f32 
length(Vec3 a) {
    f32 result = sqrt(length_sq(a));
    return result;
}

Vec3 
normalize(Vec3 a)
{
    Vec3 result = v3muls(a, rsqrt(length_sq(a)));
    return result;
}

Vec3 
v3lerp(Vec3 a, Vec3 b, f32 t) {
    Vec3 result;
    result.x = lerp(a.x, b.x, t);
    result.y = lerp(a.y, b.y, t);
    result.z = lerp(a.z, b.z, t);
    return result;
}

Vec4 
v4neg(Vec4 a) {
    Vec4 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = -a.w;
    return result;
}

Vec4 
v4add(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

Vec4 
v4sub(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}

Vec4 
v4div(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    result.w = a.w / b.w;
    return result;
}

Vec4 
v4mul(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    return result;
}

Vec4
v4divs(Vec4 a, f32 b) {
    Vec4 result;
    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;
    result.w = a.w / b;
    return result;
}

Vec4
v4muls(Vec4 a, f32 b) {
    Vec4 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    result.w = a.w * b;
    return result;
}

Vec4 
v4(f32 x, f32 y, f32 z, f32 w) {
    Vec4 result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return result;
}

Vec4 
v4s(f32 s) {
    Vec4 result;
    result.x = s;
    result.y = s;
    result.z = s;
    result.w = s;
    return result;
}

f32 
v4dot(Vec4 a, Vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

Vec4
v4normalize(Vec4 a) {
    Vec4 result = v4muls(a, rsqrt(v4dot(a, a)));
    return result;
} 