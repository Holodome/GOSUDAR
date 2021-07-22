#if !defined(MATH_HH)

#include "general.hh"

//
// Bit manipulation stuff
//

#define PACK_4U8_TO_U32_(_a, _b, _c, _d) (((_a) << 0) | ((_b) << 8) | ((_c) << 16) | ((_d) << 24))
#define PACK_4U8_TO_U32(_a, _b, _c, _d) PACK_4U8_TO_U32_((u32)(_a), (u32)(_b), (u32)(_c), (u32)(_d))
#define IS_POW2(_x) (!(_x & (_x - 1)))

inline b32 is_pow2(u64 x) {
    return !(x & (x - 1));
}

inline u32 align_forward_pow2(u32 v) {
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    return v;
}

inline u64 align_forward(u64 value, u64 align) {
    u64 result = (value + (align - 1)) / align * align;
    return result;
}

// 
// General purpose stuff
// 

#include <intrin.h>

//
// We use SSE4 here for different round functions
// SSE4 support on modern gaming machines is close to 100% (check steam hardware survey),
// but nethertheless, this is restriciton that we better have some option to disable
//

inline i32 Truncate_i32(f32 value) {
    return _mm_cvtt_ss2si(_mm_set_ss(value));
}

inline f32 Truncate(f32 value) {
    return (f32)Truncate_i32(value);
}

inline i32 Floor_i32(f32 value) {
    return _mm_cvt_ss2si(_mm_set_ss(value + value - 0.5f)) >> 1;
}

inline f32 Floor(f32 value) {
#if 0
    return (f32)Floor_i32(value);
#else 
    return _mm_cvtss_f32(_mm_floor_ps(_mm_set_ss(value)));
#endif 
}

inline i32 Round_i32(f32 value) {
    return _mm_cvt_ss2si(_mm_set_ss(value + value + 0.5f)) >> 1;
}

inline f32 Round(f32 value) {
    return (f32)Round_i32(value);
    // roundps has no rounding to nearest
}

inline i32 Ceil_i32(f32 value) {
    return -(_mm_cvt_ss2si(_mm_set_ss(-0.5f - (value + value))) >> 1);
}

inline f32 Ceil(f32 value) {
    return (f32)Ceil_i32(value);
}

inline f32 Sqrt(f32 value) {
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(value)));
}

inline f32 Rsqrt(f32 value) {
    return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(value)));
}

inline f32 Abs(f32 value) {
    u32 bits = *(u32 *)&value;
    bits &= 0x7FFFFFFF;
    return *(f32 *)&bits;
}

inline i32 Abs(i32 value) {
    u32 mask = value >> 31;
    return ((value ^ mask) - mask);
}

inline f32 Max(f32 a, f32 b) {
    return _mm_cvtss_f32(_mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
}

inline f32 Min(f32 a, f32 b) {
    return _mm_cvtss_f32(_mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
}

inline i64 Maxi(i64 a, i64 b) {
    return a > b ? a : b;
}

inline i64 Mini(i64 a, i64 b) {
    return a < b ? a : b;
}

inline f32 Clamp(f32 value, f32 low, f32 high) {
    return Max(low, Min(value, high));
}

inline i64 Clampi(i64 value, i64 low, i64 high) {
    return (value < low ? low : value > high ? high : value);
}

inline f32 Lerp(f32 a, f32 b, f32 t) {
    return a * (1.0f - t) + b * t;
}

//
// Trigonometric stufff
//

#define HALF_PI 1.57079632679f
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f

inline f32 Sin(f32 a) {
    return sinf(a);
}

inline f32 Cos(f32 a) {
    return cosf(a);
}

inline f32 Atan2(f32 y, f32 x) {
    return atan2f(y, x);
}

inline f32 rad(f32 deg) {
    return deg * PI / 180.0f;
}

inline f32 unwind_rad(f32 a) {
    while (a > TWO_PI) {
        a -= TWO_PI;
    }
    while (a < 0) {
        a += TWO_PI;
    }
    return a;
}

//
// Vector stuff
//


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
        return (*this = *this * v);
    }
    vec2G<T> &operator/=(T s) {
        return (*this = *this / v);
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
        return (*this = *this * v);
    }
    vec4G<T> &operator/=(T s) {
        return (*this = *this / v);
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

template <typename T>
inline T Dot(vec2G<T> a, vec2G<T> b) {
    T result = a.x * b.x + a.y * b.y;
    return result;
}

template <typename T>
inline T Dot(vec3G<T> a, vec3G<T> b) {
    T result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

template <typename T>
inline T Dot(vec4G<T> a, vec4G<T> b) {
    Tresult = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return result;
}

inline vec3 Cross(vec3 a, vec3 b) {
    vec3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

template <typename T>
inline f32 LengthSq(T a) {
    f32 result = Dot(a, a);
    return result;
}

template <typename T>
inline f32 Length(T a) {
    f32 result = Sqrt(LengthSq(a));
    return result;
}

template <typename T>
inline T Normalize(T a) {
    T result = a * Rsqrt(LengthSq(a));
    return result;
}

//
// Mat4
//


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

//
// Rect
//

// Axis-aligned rect
// Although name may seem strange, it seemed better to do so
struct aarect {
    union {
        struct {
            vec2 p, s;
        };
        struct {
            f32 x, y, w, h;  
        };
    };
};

inline aarect AARect(f32 x, f32 y, f32 w, f32 h) {
    aarect result;
    result.x = x;
    result.y = y;
    result.w = w;
    result.h = h;
    return result;
}

inline aarect AARect(vec2 p, vec2 size) {
    return AARect(p.x, p.y, size.x, size.y);
}

inline aarect Minmax(vec2 min, vec2 max) {
    return AARect(min, max - min);
}

inline aarect AARectEmpty() {
    return AARect(0, 0, 0, 0);
}

inline aarect AARectUnit() {
    return AARect(0, 0, 1, 1);
}

inline aarect AARectInfinite() {
    return AARect(Vec2(-INFINITY), Vec2(INFINITY));
}

inline f32 Right(aarect rect) {
    return rect.x + rect.w;
}

inline f32 Bottom(aarect rect) {
    return rect.y + rect.h;
}

inline vec2 Size(aarect rect) {
    return Vec2(rect.w, rect.h);
}

inline vec2 Middle(aarect rect) {
    return rect.p + rect.s * 0.5f;
}

inline f32 MiddleX(aarect rect) {
    return rect.x + rect.w * 0.5f;
}

inline f32 MiddleY(aarect rect) {
    return rect.y + rect.h * 0.5f;
}

inline vec2 TopLeft(aarect rect) {
    return rect.p;
}

inline vec2 BottomLeft(aarect rect) {
    return rect.p + Vec2(0, rect.h);
}

inline vec2 TopRight(aarect rect) {
    return rect.p + Vec2(rect.w, 0);
}

inline vec2 BottomRight(aarect rect) {
    return rect.p + rect.s;
}

inline aarect Move(aarect r, vec2 d) {
    return Minmax(TopLeft(r) + d, BottomRight(r) + d);
}

inline b32 Collide(aarect rect, vec2 other) {
    return rect.x < other.x && rect.y < other.y && other.x < rect.x + rect.w && other.y < rect.y + rect.h;
}

inline b32 Collide(aarect a, aarect b) {
    bool result = true;
    if ((a.x + a.w < b.x || a.x > b.x + b.w) ||
        (a.y + a.h < b.y || a.y > b.y + b.h)) {
        result = false;
    }
    
    return result;
}

inline b32 Contains(aarect rect, aarect child) {
    // @TODO optimize
    return Collide(rect, TopLeft(child)) && Collide(rect, TopRight(child)) &&
        Collide(rect, BottomLeft(child)) && Collide(rect, BottomRight(child));
}

inline aarect Clip(aarect rect, aarect child) {
    vec2 rmin = Vec2(Max(rect.x, child.x), Max(rect.y, child.y));
    vec2 rmax = Vec2(Min(rect.x + rect.w, child.x + child.w), Min(rect.y + rect.h, child.y + child.h));
    return Minmax(rmin, rmax);
}

inline aarect Join(aarect rect, aarect other) {
    vec2 rmin = Vec2(Min(rect.x, other.x), Min(rect.y, other.y));
    vec2 rmax = Vec2(Max(rect.x + rect.w, other.x + other.w), Max(rect.y + rect.h, other.y + other.h));
    return Minmax(rmin, rmax);
}

inline f32 Area(aarect aarect) {
    return aarect.w * aarect.h;
}

inline b32 DoesExist(aarect aarect) {
    return aarect.w > 0 && aarect.h > 0;
}

//
// Simd stuff
//


struct f32_4x {
    union {
        __m128 p;
        f32 e[4];
    };
    
};

inline f32_4x F32_4x(f32 all) {
    f32_4x result;
    result.p =_mm_set1_ps(all);
    return result;
}

inline f32_4x F32_4x(f32 a, f32 b, f32 c, f32 d) {
    f32_4x result;
    result.p = _mm_set_ps(a, b, c, d);
    return result;
}

inline f32_4x F32_4x_zero() {
    f32_4x result;
    result.p = _mm_setzero_ps();
    return result;
}


inline f32_4x operator+(f32_4x a, f32_4x b) {
    f32_4x result;
    result.p = _mm_add_ps(a.p, b.p);
    return result;
}

inline f32_4x operator-(f32_4x a, f32_4x b) {
    f32_4x result;
    result.p = _mm_sub_ps(a.p, b.p);
    return result;
}

inline f32_4x operator*(f32_4x a, f32_4x b) {
    f32_4x result;
    result.p = _mm_mul_ps(a.p, b.p);
    return result;
}

inline f32_4x operator/(f32_4x a, f32_4x b) {
    f32_4x result;
    result.p = _mm_div_ps(a.p, b.p);
    return result;
}

inline f32_4x &operator+=(f32_4x &a, f32_4x b) {
    return (a = (a + b));
}

inline f32_4x &operator-=(f32_4x &a, f32_4x b) {
    return (a = (a - b));
}

inline f32_4x &operator*=(f32_4x &a, f32_4x b) {
    return (a = (a * b));
}

inline f32_4x &operator/=(f32_4x &a, f32_4x b) {
    return (a = (a / b));
}

inline f32_4x operator+(f32_4x a) {
    return a;
}

inline f32_4x operator-(f32_4x a) {
    f32_4x result;
    result.p = _mm_sub_ps(_mm_setzero_ps(), a.p);
    return result;
}

inline f32_4x Abs(f32_4x a) {
    u32 mask_u32 = ~(1 << 31);
    __m128 mask = _mm_set1_ps(*(f32 *)&mask_u32);
    f32_4x result;
    result.p = _mm_and_ps(a.p, mask);
    return result;
}

inline bool all_true(f32_4x a) {
    bool result = _mm_movemask_ps(a.p) == 0xF;
    return result;
}

inline bool any_true(f32_4x a) {
    bool result = _mm_movemask_ps(a.p);
    return result;
}

inline bool all_false(f32_4x a) {
    bool result = _mm_movemask_ps(a.p) == 0;
    return result;
}

inline f32_4x operator<(f32_4x a, f32_4x b) {
    f32_4x result;
    result.p = _mm_cmplt_ps(a.p, b.p);
    return result;
}

inline f32_4x operator>(f32_4x a, f32_4x b) {
    f32_4x result;
    result.p = _mm_cmpgt_ps(a.p, b.p);
    return result;
}

inline f32_4x operator<=(f32_4x a, f32_4x b) {
    f32_4x result;
    result.p = _mm_cmple_ps(a.p, b.p);
    return result;
}

inline f32_4x operator>=(f32_4x a, f32_4x b) {
    f32_4x result;
    result.p = _mm_cmpge_ps(a.p, b.p);
    return result;
}

inline f32_4x operator==(f32_4x a, f32_4x b) {
    f32_4x result;
    result.p = _mm_cmpeq_ps(a.p, b.p);
    return result;
}

inline f32_4x operator!=(f32_4x a, f32_4x b) {
    f32_4x result;
    result.p = _mm_cmpneq_ps(a.p, b.p);
    return result;
}

inline f32_4x operator&(f32_4x a, f32_4x b) {
    f32_4x result;
    result.p = _mm_and_ps(a.p, b.p);
    return result;
}

inline f32_4x operator|(f32_4x a, f32_4x b) {
    f32_4x result;
    result.p = _mm_or_ps(a.p, b.p);
    return result;
}

inline f32_4x operator^(f32_4x a, f32_4x b) {
    f32_4x result;
    result.p = _mm_xor_ps(a.p, b.p);
    return result;
}

typedef vec3G<f32_4x> vec3_4x;
typedef vec4G<f32_4x> vec4_4x;

inline vec3_4x Vec3_4x(f32 s) {
    return { F32_4x(s), F32_4x(s), F32_4x(s) }; 
}
inline vec3_4x Vec3_4x(vec3 v) {
    return { F32_4x(v.x), F32_4x(v.y), F32_4x(v.z) };
}

inline vec3 GetComponent(vec3_4x vec, u32 idx) {
    vec3 result = Vec3(vec.x.e[idx], vec.y.e[idx], vec.z.e[idx]);
    return result;
}


//
// Some misc functions...
//

inline vec3 triangle_normal(vec3 a, vec3 b, vec3 c) {
    vec3 ab = b - a;
    vec3 ac = c - a;
    return Cross(ab, ac);
}

inline vec3 xz(vec2 xz, f32 y = 0.0f) {
    return Vec3(xz.x, y, xz.y);
}

inline vec2 Floor(vec2 v) {
    return Vec2(Floor(v.x), Floor(v.y));
}

inline vec4 Sqrt(vec4 v) {
    vec4 result;
    result.x = Sqrt(v.x);
    result.y = Sqrt(v.y);
    result.z = Sqrt(v.z);
    result.w = Sqrt(v.w);
    return result;
}

inline vec4 rgba_unpack_linear1(u32 rgba) {
    f32 r = (f32)((rgba & 0x000000FF) >> 0 ) * (1.0f / 255.0f);
    f32 g = (f32)((rgba & 0x0000FF00) >> 8 ) * (1.0f / 255.0f);
    f32 b = (f32)((rgba & 0x00FF0000) >> 16) * (1.0f / 255.0f);
    f32 a = (f32)((rgba & 0xFF000000) >> 24) * (1.0f / 255.0f);
    return Vec4(r, g, b, a);
}

inline u32 rgba_pack_linear256(u32 r, u32 g, u32 b, u32 a) {
    // If values passed here are greater that 255 something for sure went wrong
    assert(r <= 0xFF && b <= 0xFF && b <= 0xFF && a <= 0xFF);
    return r << 0 | g << 8 | b << 16 | a << 24;
}

inline u32 rgba_pack_4x8_linear1(vec4 c) {
    u32 ru = (u32)Round(Clamp(c.r, 0, 0.999f) * 255.0f);
    u32 gu = (u32)Round(Clamp(c.g, 0, 0.999f) * 255.0f);
    u32 bu = (u32)Round(Clamp(c.b, 0, 0.999f) * 255.0f);
    u32 au = (u32)Round(Clamp(c.a, 0, 0.999f) * 255.0f);
    return rgba_pack_linear256(ru, gu, bu, au);
}

inline b32 ray_intersect_plane(vec3 plane_normal, f32 plane_d, vec3 o, vec3 d, f32 *t_out) {
    f32 denom = Dot(plane_normal, d);
    if (Abs(denom) > 0.001f) {
        f32 t = (-plane_d - Dot(plane_normal, o)) / denom;
        *t_out = t;
        return true;
    }
    return false;
}

void get_billboard_positions(vec3 mid_bottom, vec3 right, vec3 up, f32 width, f32 height, vec3 out[4]) {
    vec3 v_width = right * width;
    vec3 v_height = up * height;
    vec3 top_left = mid_bottom - v_width * 0.5f + v_height;
    vec3 bottom_left = top_left - v_height;
    vec3 top_right = top_left + v_width;
    vec3 bottom_right = top_right - v_height;
    out[0] = top_left;
    out[1] = bottom_left;
    out[2] = top_right;
    out[3] = bottom_right;
}

inline vec3 uv_to_world(mat4x4 projection, mat4x4 view, vec2 uv) {
    f32 x = uv.x;
    f32 y = uv.y;
    vec3 ray_dc = Vec3(x, y, 1.0f);
    vec4 ray_clip = Vec4(ray_dc.xy, -1.0f, 1.0f);
    vec4 ray_eye = Inverse(projection) * ray_clip;
    ray_eye.z = -1.0f;
    ray_eye.w = 0.0f;
    vec3 ray_world = Normalize((Inverse(view) * ray_eye).xyz);
    return ray_world;
}

inline void store_points(aarect rect, vec2 *pts) {
    pts[0] = TopLeft(rect);
    pts[1] = BottomLeft(rect);
    pts[2] = TopRight(rect);
    pts[3] = BottomRight(rect);
}

inline void store_points(aarect rect, vec3 *pts) {
    pts[0].xy = TopLeft(rect);
    pts[1].xy = BottomLeft(rect);
    pts[2].xy = TopRight(rect);
    pts[3].xy = BottomRight(rect);
    pts[0].z = 0;
    pts[1].z = 0;
    pts[2].z = 0;
    pts[3].z = 0;
}

inline aarect transform_rect_based_on_other(aarect a, aarect b, aarect b_transformed) {
    aarect result;
    result.x = a.x + (b_transformed.x - b.x) / b.w * a.w;
    result.y = a.y + (b_transformed.y - b.y) / b.h * a.h;
    result.w = Right(a) + (Right(b_transformed) - Right(b)) / b.w * a.w - result.x;
    result.h = Bottom(a) + (Bottom(b_transformed) - Bottom(b)) / b.h * a.h - result.y;
    return result;
}

#define MATH_HH 1
#endif 