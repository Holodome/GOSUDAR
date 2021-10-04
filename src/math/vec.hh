/*
Author: Holodome
Date: 03.10.2021
File: src/math/vec.hhh
Version: 0
*/
#pragma once 
#include "lib/general.hh"

#include "math/math.hh"

template <typename T>
struct vec2G {
    union {
        struct {
            T x, y;
        };
        T e[2];
    };
    
    vec2G<T> operator-() const {
        return {-x, -y};
    } 
    vec2G<T> operator+() const {
        return *this;
    }
    
    vec2G<T> operator+(vec2G<T> v) const {
        return {x + v.x, y + v.y};
    }
    vec2G<T> operator-(vec2G<T> v) const {
        return { x - v.x, y - v.y };
    }
    vec2G<T> operator*(vec2G<T> v) const {
        return { x * v.x, y * v.y };
    }
    vec2G<T> operator/(vec2G<T> v) const {
        return { x / v.x, y / v.y };
    }
    vec2G<T> operator*(T s) const {
        return { x * s, y * s };
    }
    vec2G<T> operator/(T s) const {
        return { x / s, y / s };
    }
    
    vec2G<T> &operator+=(vec2G<T> v) {
        return (*this = *this + v);
    }
    vec2G<T> &operator-=(vec2G<T> v) {
        return (*this = *this - v);
    }
    vec2G<T> &operator*=(vec2G<T> v) {
        return (*this = *this * v);
    }
    vec2G<T> &operator/=(vec2G<T> v) {
        return (*this = *this / v);
    }
    vec2G<T> &operator*=(T s) {
        return (*this = *this * s);
    }
    vec2G<T> &operator/=(T s) {
        return (*this = *this / s);
    }
};

using vec2 = vec2G<f32>;
inline vec2 Vec2(f32 s) {
    return { s, s };
}
inline vec2 Vec2(f32 x, f32 y) {
    return { x, y };
}

template <typename T>
struct vec3G {
    union {
        struct {
            T x, y, z;
        };
        struct {
            T r, g, b;
        };
        vec2G<T> xy;
        T e[3];
    };
    
    vec3G<T> operator-() {
        return { -x, -y, -z };
    } 
    vec3G<T> operator+() {
        return *this;
    }
    
    vec3G<T> operator+(vec3G<T> v) {
        return { x + v.x, y + v.y, z + v.z };
    }
    vec3G<T> operator-(vec3G<T> v) {
        return { x - v.x, y - v.y, z - v.z };
    }
    vec3G<T> operator*(vec3G<T> v) {
        return { x * v.x, y * v.y, z * v.z };
    }
    vec3G<T> operator/(vec3G<T> v) {
        return { x / v.x, y / v.y, z / v.z };
    }
    vec3G<T> operator*(T s) {
        return { x * s, y * s, z * s };
    }
    vec3G<T> operator/(T s) {
        return { x / s, y / s, z / s };
    }
    
    vec3G<T> &operator+=(vec3G<T> v) {
        return (*this = *this + v);
    }
    vec3G<T> &operator-=(vec3G<T> v) {
        return (*this = *this - v);
    }
    vec3G<T> &operator*=(vec3G<T> v) {
        return (*this = *this * v);
    }
    vec3G<T> &operator/=(vec3G<T> v) {
        return (*this = *this / v);
    }
    vec3G<T> &operator*=(T s) {
        return (*this = *this * s);
    }
    vec3G<T> &operator/=(T s) {
        return (*this = *this / s);
    }
};

using vec3 = vec3G<f32>;
inline vec3 Vec3(f32 s) {
    return { s, s, s };
}
inline vec3 Vec3(f32 x, f32 y, f32 z) {
    return { x, y, z};
}
inline vec3 Vec3(vec2 xy, f32 z = 0.0f) {
    return { xy.x, xy.y, z };
}

template <typename T>
struct vec4G {
    union {
        struct {
            T x, y, z, w;
        };
        struct {
            T r, g, b, a;
        };
        struct {
            vec3G<T> xyz;
        };
        T e[4];
    };
    
    vec4G<T> operator-() {
        return vec4G(-x, -y, -z, -w);
    } 
    vec4G<T> operator+() {
        return *this;
    }
    
    vec4G<T> operator+(vec4G<T> v) {
        return {x + v.x, y + v.y, z + v.z, w + v.w};
    }
    vec4G<T> operator-(vec4G<T> v) {
        return {x - v.x, y - v.y, z - v.z, w - v.w};
    }
    vec4G<T> operator*(vec4G<T> v) {
        return {x * v.x, y * v.y, z * v.z, w * v.w};
    }
    vec4G<T> operator/(vec4G<T> v) {
        return {x / v.x, y / v.y, z / v.z, w / v.w};
    }
    vec4G<T> operator*(T s) {
        return {x * s, y * s, z * s, w * s};
    }
    vec4G<T> operator/(T s) {
        return {x / s, y / s, z / s, w / s};
    }
    
    vec4G<T> &operator+=(vec4G<T> v) {
        return (*this = *this + v);
    }
    vec4G<T> &operator-=(vec4G<T> v) {
        return (*this = *this - v);
    }
    vec4G<T> &operator*=(vec4G<T> v) {
        return (*this = *this * v);
    }
    vec4G<T> &operator/=(vec4G<T> v) {
        return (*this = *this / v);
    }
    vec4G<T> &operator*=(T s) {
        return (*this = *this * s);
    }
    vec4G<T> &operator/=(T s) {
        return (*this = *this / s);
    }
};

using vec4 = vec4G<f32>;
inline vec4 Vec4(f32 s) {
    return { s, s, s, s };
}
inline vec4 Vec4(f32 x, f32 y, f32 z, f32 w) {
    return { x, y, z, w };
}
inline vec4 Vec4(vec3 xyz, f32 w = 1.0f) {
    return { xyz.x, xyz.y, xyz.z, w };
}
inline vec4 Vec4(vec2 xy, f32 z = 0.0f, f32 w = 1.0f) {
    return { xy.x, xy.y, z, w };
}

namespace math
{
    template <typename T>
    inline T dot(vec2G<T> a, vec2G<T> b) {
        T result = a.x * b.x + a.y * b.y;
        return result;
    }

    template <typename T>
    inline T dot(vec3G<T> a, vec3G<T> b) {
        T result = a.x * b.x + a.y * b.y + a.z * b.z;
        return result;
    }

    template <typename T>
    inline T dot(vec4G<T> a, vec4G<T> b) {
        T result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        return result;
    }

    inline vec3 cross(vec3 a, vec3 b) {
        vec3 result;
        result.x = a.y * b.z - a.z * b.y;
        result.y = a.z * b.x - a.x * b.z;
        result.z = a.x * b.y - a.y * b.x;
        return result;
    }

    template <typename T>
    inline f32 length_sq(T a) {
        f32 result = dot(a, a);
        return result;
    }

    template <typename T>
    inline f32 length(T a) {
        f32 result = sqrt(length_sq(a));
        return result;
    }

    template <typename T>
    inline T normalize(T a) {
        T result = a * rsqrt(length_sq(a));
        return result;
    }

    inline vec3 xz(vec2 xz, f32 y = 0.0f) {
        return Vec3(xz.x, y, xz.y);
    }

    inline vec2 floor(vec2 v) {
        return Vec2(floor(v.x), floor(v.y));
    }
    
    inline vec4 sqrt(vec4 v) {
        vec4 result;
        result.x = sqrt(v.x);
        result.y = sqrt(v.y);
        result.z = sqrt(v.z);
        result.w = sqrt(v.w);
        return result;
    }
} // namespace math
