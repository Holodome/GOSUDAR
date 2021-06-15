#if !defined(MATH_HH)

#include "general.hh"

#include "math.h"

#ifdef max
#undef max
#endif 
#ifdef min
#undef min
#endif 

namespace Math {
    const f32 HALF_PI = 1.57079632679f;
    const f32 PI = 3.14159265359f;
    const f32 TWO_PI = 6.28318530718f;

    inline bool is_power_of_two(u64 x) {
        return !(x & (x - 1));
    }

    inline f32 
    rsqrt(f32 a) {
        return 1.0f / sqrtf(a);
    }

    inline f32 
    rad(f32 deg) {
        return deg * PI / 180.0f;
    }

    inline f32
    rsqrtf(f32 a) {
        return 1.0f / sqrtf(a);
    }

    inline f32 
    clamp(f32 a, f32 low, f32 high) {
        return (a < low ? low : a > high ? high : a);
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

    inline f32  
    saturate(f32 a) {
        return clamp(a, 0, 1);
    }

    template <typename T>
    inline T min(T a, T b) {
        return (a < b ? a : b);
    }

    template <typename T>
    inline T max(T a, T b) {
        return (a > b ? a : b);
    }

    template <typename T>
    inline T lerp(T a, T b, f32 t) {
        return a * (1.0f - t) + b * t;
    }

    template <typename T>
    inline T bilerp(T v00, T v01, T v10, T v11, f32 tx, f32 ty) {
        return lerp(lerp(v00, v10, tx), lerp(v01, v11, tx), ty);
    }

    template <typename T>
    void swap(T &a, T &b) {
        T tmp = a;
        a = b;
        b = tmp;
    }
    
    template <typename T>
    struct Vec2G {
        union {
            struct {
                T x, y;
            };
            T e[2];
        };
        
        Vec2G() : x(0), y(0) {}; 
        Vec2G(T x, T y) : x(x), y(y) {}
        explicit Vec2G(T s) : x(s), y(s) {}
        template <typename S>
        explicit Vec2G(Vec2G<S> v) : x(v.x), y(v.y) {}
        
        Vec2G<T> operator-() const {
            return Vec2G<T>(-x, -y);
        } 
        Vec2G<T> operator+() const {
            return *this;
        }
        
        Vec2G<T> operator+(Vec2G<T> v) const {
            return Vec2G<T>(x + v.x, y + v.y);
        }
        Vec2G<T> operator-(Vec2G<T> v) const {
            return Vec2G<T>(x - v.x, y - v.y);
        }
        Vec2G<T> operator*(Vec2G<T> v) const {
            return Vec2G<T>(x * v.x, y * v.y);
        }
        Vec2G<T> operator/(Vec2G<T> v) const {
            return Vec2G<T>(x / v.x, y / v.y);
        }
        Vec2G<T> operator*(T s) const {
            return Vec2G<T>(x * s, y * s);
        }
        Vec2G<T> operator/(T s) const {
            return Vec2G<T>(x / s, y / s);
        }
        
        Vec2G<T> &operator+=(Vec2G<T> v) {
            return (*this = *this + v);
        }
        Vec2G<T> &operator-=(Vec2G<T> v) {
            return (*this = *this - v);
        }
        Vec2G<T> &operator*=(Vec2G<T> v) {
            return (*this = *this * v);
        }
        Vec2G<T> &operator/=(Vec2G<T> v) {
            return (*this = *this / v);
        }
        Vec2G<T> &operator*=(T s) {
            return (*this = *this * v);
        }
        Vec2G<T> &operator/=(T s) {
            return (*this = *this / v);
        }
        
        f32 aspect_ratio() const {
            return (f32)x / (f32)y;
        }
        
        T product() const {
            return x * y;
        }
    };

    using Vec2 = Vec2G<f32>;
    using Vec2i = Vec2G<i32>;

    template <typename T>
    struct Vec3G {
        union {
            struct {
                T x, y, z;
            };
            struct {
                T r, g, b;
            };
            Vec2G<T> xy;
            T e[3];
        };
        
        Vec3G() : x(0), y(0), z(0) {};
        Vec3G(T x, T y, T z) : x(x), y(y), z(z) {}
        explicit Vec3G(T s) : x(s), y(s), z(s) {}
        explicit Vec3G(Vec2G<T> xy, T z = 0) : x(xy.x), y(xy.y), z(z) {}
        
        Vec3G<T> operator-() {
            return Vec3G(-x, -y, -z);
        } 
        Vec3G<T> operator+() {
            return *this;
        }
        
        template <typename S>
        Vec3G<T> operator+(Vec3G<S> v) {
            return Vec3G<T>(x + v.x, y + v.y, z + v.z);
        }
        template <typename S>
        Vec3G<T> operator-(Vec3G<S> v) {
            return Vec3G<T>(x - v.x, y - v.y, z - v.z);
        }
        template <typename S>
        Vec3G<T> operator*(Vec3G<S> v) {
            return Vec3G<T>(x * v.x, y * v.y, z * v.z);
        }
        template <typename S>
        Vec3G<T> operator/(Vec3G<S> v) {
            return Vec3G<T>(x / v.x, y / v.y, z / v.z);
        }
        Vec3G<T> operator*(T s) {
            return Vec3G<T>(x * s, y * s, z * s);
        }
        Vec3G<T> operator/(T s) {
            return Vec3G<T>(x / s, y / s, z / s);
        }
        
        template <typename S>
        Vec3G<T> &operator+=(Vec3G<S> v) {
            return (*this = *this + v);
        }
        template <typename S>
        Vec3G<T> &operator-=(Vec3G<S> v) {
            return (*this = *this - v);
        }
        template <typename S>
        Vec3G<T> &operator*=(Vec3G<S> v) {
            return (*this = *this * v);
        }
        template <typename S>
        Vec3G<T> &operator/=(Vec3G<S> v) {
            return (*this = *this / v);
        }
        Vec3G<T> &operator*=(T s) {
            return (*this = *this * s);
        }
        Vec3G<T> &operator/=(T s) {
            return (*this = *this / s);
        }
    };

    using Vec3 = Vec3G<f32>;
    using Vec3i = Vec3G<i32>;

    template <typename T>
    struct Vec4G {
        union {
            struct {
                T x, y, z, w;
            };
            struct {
                T r, g, b, a;
            };
            struct {
                Vec3G<T> xyz;
            };
            T e[4];
        };
        
        Vec4G() : x(0), y(0), z(0), w(0) {};
        Vec4G(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
        explicit Vec4G(T s) : x(s), y(s), z(s), w(s) {}
        explicit Vec4G(Vec2G<T> xy, T z = 0, T w = 1) : x(xy.x), y(xy.y), z(z), w(w) {}
        explicit Vec4G(Vec3G<T> xyz, T w = 1) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
        
        Vec4G<T> operator-() {
            return Vec4G(-x, -y, -z, -w);
        } 
        Vec4G<T> operator+() {
            return *this;
        }
        
        template <typename S>
        Vec4G<T> operator+(Vec4G<S> v) {
            return Vec4G<T>(x + v.x, y + v.y, z + v.z, w + v.w);
        }
        template <typename S>
        Vec4G<T> operator-(Vec4G<S> v) {
            return Vec4G<T>(x - v.x, y - v.y, z - v.z, w - v.w);
        }
        template <typename S>
        Vec4G<T> operator*(Vec4G<S> v) {
            return Vec4G<T>(x * v.x, y * v.y, z * v.z, w * v.w);
        }
        template <typename S>
        Vec4G<T> operator/(Vec4G<S> v) {
            return Vec4G<T>(x / v.x, y / v.y, z / v.z, w / v.w);
        }
        Vec4G<T> operator*(T s) {
            return Vec4G<T>(x * s, y * s, z * s, w * s);
        }
        Vec4G<T> operator/(T s) {
            return Vec4G<T>(x / s, y / s, z / s, w / s);
        }
        
        template <typename S>
        Vec4G<T> &operator+=(Vec4G<S> v) {
            return (*this = *this + v);
        }
        template <typename S>
        Vec4G<T> &operator-=(Vec4G<S> v) {
            return (*this = *this - v);
        }
        template <typename S>
        Vec4G<T> &operator*=(Vec4G<S> v) {
            return (*this = *this * v);
        }
        template <typename S>
        Vec4G<T> &operator/=(Vec4G<S> v) {
            return (*this = *this / v);
        }
        Vec4G<T> &operator*=(T s) {
            return (*this = *this * v);
        }
        Vec4G<T> &operator/=(T s) {
            return (*this = *this / v);
        }
    };

    using Vec4 = Vec4G<f32>;
    using Vec4i = Vec4G<i32>;

    // Design decision for vectors was that functions related to them should not be members of structs
    // because there are integer vectors and there is no point in adding stuff like cross product into them

    inline f32 dot(Vec3 a, Vec3 b) {
        f32 result = a.x * b.x + a.y * b.y + a.z * b.z;
        return result;
    }

    inline f32 dot(Vec4 a, Vec4 b) {
        f32 result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        return result;
    }

    inline Vec3 cross(Vec3 a, Vec3 b) {
        Vec3 result;
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

    struct Mat4x4 {
        union {
            f32  e[4][4];
            Vec4 v[4];
            f32  i[16];
            struct {
                f32 m00; f32 m01; f32 m02; f32 m03;
                f32 m10; f32 m11; f32 m12; f32 m13;
                f32 m20; f32 m21; f32 m22; f32 m23;
                f32 m30; f32 m31; f32 m32; f32 m33;
            };
        };
        
        const f32 *value_ptr() const {
            return &m00;
        }
        
        Mat4x4()  {};
        explicit Mat4x4(Vec4 v0, Vec4 v1, Vec4 v2, Vec4 v3) {
            v[0] = v0;
            v[1] = v1;
            v[2] = v2;
            v[3] = v3;
        }
        
        static Mat4x4 identity() {
            Mat4x4 result = {};
            memset(&result, 0, sizeof(result));
            result.m00 = result.m11 = result.m22 = result.m33 = 1.0f;
            return result;
        }       
        
        static Mat4x4 translate(Vec3 t) {
            Mat4x4 result = Mat4x4::identity();
            result.e[3][0] = t.x;
            result.e[3][1] = t.y;
            result.e[3][2] = t.z;
            return result;
        }

        static Mat4x4 scale(Vec3 s) {
            Mat4x4 result = Mat4x4(
                Vec4(s.x,   0,   0,  0),
                Vec4(0,   s.y,   0,  0),
                Vec4(0,     0, s.z,  0),
                Vec4(0,     0,   0,  1)
            );
            return result;
        }

        static Mat4x4 rotation_x(f32 angle) {
            const f32 c = cosf(angle);
            const f32 s = sinf(angle);
            Mat4x4 r = Mat4x4(
                Vec4(1, 0, 0, 0),
                Vec4(0, c,-s, 0),
                Vec4(0, s, c, 0),
                Vec4(0, 0, 0, 1)
            );
            return(r);
        }

        static Mat4x4 rotation_y(f32 angle) {
            const f32 c = cosf(angle);
            const f32 s = sinf(angle);
            Mat4x4 r = Mat4x4(
                Vec4( c, 0, s, 0),
                Vec4( 0, 1, 0, 0),
                Vec4(-s, 0, c, 0),
                Vec4( 0, 0, 0, 1)
            );
            return(r);
        }

        static Mat4x4 rotation_z(f32 angle) {
            const f32 c = cosf(angle);
            const f32 s = sinf(angle);
            Mat4x4 r = Mat4x4(
                Vec4(c,-s, 0, 0),
                Vec4(s, c, 0, 0),
                Vec4(0, 0, 1, 0),
                Vec4(0, 0, 0, 1)
            );
            return(r);
        }

        static Mat4x4 rotation(f32 angle, Vec3 a) {
            const f32 c = cosf(angle);
            const f32 s = sinf(angle);
            a = normalize(a);

            const f32 tx = (1.0f - c) * a.x;
            const f32 ty = (1.0f - c) * a.y;
            const f32 tz = (1.0f - c) * a.z;

            Mat4x4 r = Mat4x4(
                Vec4(c + tx * a.x, 		     tx * a.y - s * a.z, tx * a.z - s * a.y, 0),
                Vec4(    ty * a.x - a.z * s, ty * a.y + c,       ty * a.z + s * a.x, 0),
                Vec4(    tz * a.x + s * a.y, tz * a.y - s * a.x, tz * a.z + c,       0),
                Vec4(0, 0, 0, 1)
            );
            return(r);
        }

        static Mat4x4 ortographic_2d(f32 l, f32 r, f32 b, f32 t) {
            Mat4x4 result =	Mat4x4(
                Vec4(2.0f / (r - l),    0,                   0, 0),
                Vec4(0,                 2.0f / (t - b),      0, 0),
                Vec4(0,                 0,                  -1, 0),
                Vec4(-(r + l) / (r - l), -(t + b) / (t - b), 0, 1)
            );
            return result;
        }

        static Mat4x4 ortographic_3d(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
            Mat4x4 result =	Mat4x4(
                Vec4(2.0f / (r - l),    0,                   0,                 0),
                Vec4(0,                 2.0f / (t - b),      0,                 0),
                Vec4(0,                 0,                  -2.0f / (f - n),    0),
                Vec4(-(r + l) / (r - l), -(t + b) / (t - b),-(f + n) / (f - n), 1)
            );
            return result;
        }

        static Mat4x4 perspective(f32 fov, f32 aspect, f32 n, f32 f) {
            const f32 toHf = tanf(fov * 0.5f);

            Mat4x4 r = Mat4x4(
                Vec4(1.0f / (aspect * toHf), 0,           0,                         0),
                Vec4(0,                      1.0f / toHf, 0,                         0),
                Vec4(0,                      0,          -       (f + n) / (f - n), -1),
                Vec4(0,                      0,          -2.0f * (f * n) / (f - n),  0)
            );
            return(r);
        }
        
        static Mat4x4 inverse(Mat4x4 m) {
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
            
            Vec4 fac0 = Vec4(coef00, coef00, coef02, coef03);
            Vec4 fac1 = Vec4(coef04, coef04, coef06, coef07);
            Vec4 fac2 = Vec4(coef08, coef08, coef10, coef11);
            Vec4 fac3 = Vec4(coef12, coef12, coef14, coef15);
            Vec4 fac4 = Vec4(coef16, coef16, coef18, coef19);
            Vec4 fac5 = Vec4(coef20, coef20, coef22, coef23);
            
            Vec4 vec0 = Vec4(m.e[1][0], m.e[0][0], m.e[0][0], m.e[0][0]);
            Vec4 vec1 = Vec4(m.e[1][1], m.e[0][1], m.e[0][1], m.e[0][1]);
            Vec4 vec2 = Vec4(m.e[1][2], m.e[0][2], m.e[0][2], m.e[0][2]);
            Vec4 vec3 = Vec4(m.e[1][3], m.e[0][3], m.e[0][3], m.e[0][3]);
            
            Vec4 inv0 = (vec1 * fac0) - (vec2 * fac1) + (vec3 * fac2);
            Vec4 inv1 = (vec0 * fac0) - (vec2 * fac3) + (vec3 * fac4);
            Vec4 inv2 = (vec0 * fac1) - (vec1 * fac3) + (vec3 * fac5);
            Vec4 inv3 = (vec0 * fac2) - (vec1 * fac4) + (vec2 * fac5);
            
            const Vec4 sign_a = Vec4(1, -1,  1, -1);
            const Vec4 sign_b = Vec4(-1,  1, -1,  1);
            
            Mat4x4 inverse;
            for(u32 i = 0; 
                i < 4;
                ++i) {
                inverse.e[0][i] = inv0.e[i] * sign_a.e[i];
                inverse.e[1][i] = inv1.e[i] * sign_b.e[i];
                inverse.e[2][i] = inv2.e[i] * sign_a.e[i];
                inverse.e[3][i] = inv3.e[i] * sign_b.e[i];
            }
            
            Vec4 row0 = Vec4(inverse.e[0][0], inverse.e[1][0], inverse.e[2][0], inverse.e[3][0]);
            Vec4 m0   = Vec4(m.e[0][0],       m.e[0][1],       m.e[0][2],       m.e[0][3]      );
            Vec4 dot0 = m0 * row0;
            f32 dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);
            
            f32 one_over_det = 1.0f / dot1;
            
            for (u32 i = 0; i < 16; ++i) {
                inverse.i[i] *= one_over_det;
            }
            return inverse;
        }
        
        friend Mat4x4 operator*(Mat4x4 a, Mat4x4 b) {
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

        friend Vec4 operator*(Mat4x4 a, Vec4 v) {
            Vec4 result;
            result.x = a.e[0][0] * v.x + a.e[1][0] * v.y + a.e[2][0] * v.z + a.e[3][0] * v.w;
            result.y = a.e[0][1] * v.x + a.e[1][1] * v.y + a.e[2][1] * v.z + a.e[3][1] * v.w;
            result.z = a.e[0][2] * v.x + a.e[1][2] * v.y + a.e[2][2] * v.z + a.e[3][2] * v.w;
            result.w = a.e[0][3] * v.x + a.e[1][3] * v.y + a.e[2][3] * v.z + a.e[3][3] * v.w;
            return result;
        }
        
        Mat4x4 &operator*=(Mat4x4 b) {
            return (*this = *this * b);
        }
        
        Vec3 get_x() {
            return Vec3(e[0][0], e[1][0], e[2][0]);
        }

        Vec3 get_y() {
            return Vec3(e[0][1], e[1][1], e[2][1]);
        }

        Vec3 get_z() {
            return Vec3(e[0][2], e[1][2], e[2][2]);
        }
    };

    struct Rect {
        union {
            struct {
                Vec2 p, s;
            };
            struct {
                f32 x, y, w, h;  
            };
        };
        
        Rect() {}
        explicit Rect(f32 x, f32 y, f32 w, f32 h)
            : x(x), y(y), w(w), h(h) {}
        explicit Rect(Vec2 p, Vec2 size)
            : p(p), s(size) {}
        static Rect minmax(Vec2 min, Vec2 max) {
            return Rect(min, max - min);
        }
        static Rect empty() {
            return Rect(0, 0, 0, 0);
        }
        
        f32 right() {
            return x + w;
        }
        f32 bottom() {
            return y + h;
        }
        
        Vec2 size() const {
            return Vec2(w, h);
        }
        
        Vec2 middle() const {
            return p + s * 0.5f;
        }
        
        f32 middle_x() const {
            return x + w * 0.5f;
        }
        
        f32 middle_y() const {
            return y + h * 0.5f;
        }
        
        Vec2 top_left() const {
            return p;
        }
        Vec2 bottom_left() const {
            return p + Vec2(0, h);
        }
        Vec2 top_right() const {
            return p + Vec2(w, 0);
        }
        Vec2 bottom_right() const {
            return p + s;
        }
        
        void normalize() {
            if (w < 0) {
                x += w;
                w = -w;
            }
            if (h < 0) {
                y += h;
                h = -h;
            }
        }
        
        void store_points(Vec2 *pts) {
            pts[0] = top_left();
            pts[1] = bottom_left();
            pts[2] = top_right();
            pts[3] = bottom_right();
        }
        void store_points(Vec3 *pts) {
            pts[0].xy = top_left();
            pts[1].xy = bottom_left();
            pts[2].xy = top_right();
            pts[3].xy = bottom_right();
            pts[0].z = 0;
            pts[1].z = 0;
            pts[2].z = 0;
            pts[3].z = 0;
        }
        
        static Rect move(Rect r, Vec2 d) {
            return Rect::minmax(r.top_left() + d, r.bottom_right() + d);
        }
        
        bool collide(Vec2 p) {
            return x < p.x && y < p.y && p.x < x + w && p.y < y + h;
        }
        
        bool collide(Rect rect) {
            bool result = true;
            if ((x + w < rect.x || x > rect.x + rect.w) ||
                (y + h < rect.y | y > rect.y + rect.h)) {
                result = false;
            }

            return result;
        }
        
        bool contains(Rect child) {
            return collide(child.top_left()) && collide(child.top_right()) &&
                   collide(child.bottom_left()) && collide(child.bottom_right());
        }
        
        Rect clip(Rect child) {
            Vec2 rmin = Vec2(Math::max(x, child.x), Math::max(y, child.y));
            Vec2 rmax = Vec2(Math::min(x + w, child.x + child.w), Math::min(y + h, child.y + child.h));
            return Rect::minmax(rmin, rmax);
        }
        
        Rect join(Rect other) {
            Vec2 rmin = Vec2(Math::min(x, other.x), Math::min(y, other.y));
            Vec2 rmax = Vec2(Math::max(x + w, other.x + other.w), Math::max(y + h, other.y + other.h));
            return Rect::minmax(rmin, rmax);
        }
    };

    struct Bounds {
        Vec3 min, max;
        
        explicit Bounds()
            : min(Vec3(INFINITY)), max(Vec3(-INFINITY)) {}
        explicit Bounds(Vec3 min, Vec3 max) 
            : min(min), max(max) {}
        explicit Bounds(Vec3 a)
            : min(a), max(a) {}
        
        f32 surface_area() {
            Vec3 d = max - min;
            return 2 * (d.x * d.y + d.y * d.z + d.z * d.z);
        }
        
        u32 longest_axis() {
            Vec3 d = max - min;
            return (d.x > d.y && d.x > d.z) ? 0 : (d.y > d.x && d.y > d.z) ? 1 : 2;
        }
        
        static Bounds join(Bounds a, Bounds b) {
            Bounds result;
            result.min.x = fminf(a.min.x, b.min.x);
            result.min.y = fminf(a.min.y, b.min.y);
            result.min.z = fminf(a.min.z, b.min.z);
            result.max.x = fmaxf(a.max.x, b.max.x);
            result.max.y = fmaxf(a.max.y, b.max.y);
            result.max.z = fmaxf(a.max.z, b.max.z);
            return result;   
        }
        
        static Bounds extend(Bounds a, Vec3 p) {
            Bounds result;
            result.min.x = fminf(a.min.x, p.x);
            result.min.y = fminf(a.min.y, p.y);
            result.min.z = fminf(a.min.z, p.z);
            result.max.x = fmaxf(a.max.x, p.x);
            result.max.y = fmaxf(a.max.y, p.y);
            result.max.z = fmaxf(a.max.z, p.z);
            return result;
        }
    };

    struct Quat4 {
        union {
            struct {
                f32 x, y, z, w;
            };
            Vec4 v;
            f32 e[4];
        };
        
        Quat4() {}
        Quat4(f32 x, f32 y, f32 z, f32 w) {
            this->x = x;
            this->y = y;
            this->z = z;
            this->w = w;
        }
        
        static Quat4 identity() {
            return Quat4(0, 0, 0, 1);
        }
        
        static Quat4 euler(f32 roll, f32 pitch, f32 yaw) {
            f32 cy = cosf(yaw * 0.5f);
            f32 sy = sinf(yaw * 0.5f);
            f32 cp = cosf(pitch * 0.5f);
            f32 sp = sinf(pitch * 0.5f);
            f32 cr = cosf(roll * 0.5f);
            f32 sr = sinf(roll * 0.5f);

            return Quat4(sr * cp * cy - cr * sp * sy,
                        cr * sp * cy + sr * cp * sy,
                        cr * cp * sy - sr * sp * cy,
                        cr * cp * cy + sr * sp * sy);
        }
        
        friend Quat4 operator+(Quat4 a, Quat4 b) { 
            Quat4 result; 
            result.v = a.v + b.v; 
            return result; 
        }

        friend Quat4 operator-(Quat4 a, Quat4 b) { 
            Quat4 result; 
            result.v = a.v - b.v; 
            return result; 
        }

        friend Quat4 operator/(Quat4 q, f32 s) { 
            Quat4 result; 
            result.v = q.v / s; 
            return result; 
        }

        friend Quat4 operator*(Quat4 q, f32 s) { 
            Quat4 result; 
            result.v = q.v * s; 
            return result; 
        }
        
    static Mat4x4 to_mat4x4(Quat4 q) {
            f32 xx = q.x * q.x;    
            f32 yy = q.y * q.y;    
            f32 zz = q.z * q.z;
            f32 xy = q.x * q.y;    
            f32 xz = q.x * q.z;    
            f32 yz = q.y * q.z;    
            f32 wx = q.w * q.x;    
            f32 wy = q.w * q.y;    
            f32 wz = q.w * q.z;
            
            Mat4x4 result = Mat4x4::identity();
            result.e[0][0] = 1 - 2 * (yy + zz);
            result.e[0][1] = 2 * (xy + wz);
            result.e[0][2] = 2 * (xz - wy);
            result.e[1][0] = 2 * (xy - wz);
            result.e[1][1] = 1 - 2 * (xx + zz);
            result.e[1][2] = 2 * (yz + wx);
            result.e[2][0] = 2 * (xz + wy);
            result.e[2][1] = 2 * (yz - wx);
            result.e[2][2] = 1 - 2 * (xx + yy);  
            return result;    
        }
    };

    inline f32 dot(Quat4 a, Quat4 b) {
        return dot(a.v, b.v);
    }

    inline Quat4 normalize(Quat4 q) {
        Quat4 result;
        result.v = normalize(q.v);
        return result;
    } 

    inline Quat4 lerp(Quat4 a, Quat4 b, f32 t) {
        Quat4 result;
        
        f32 cos_theta = dot(a, b);
        if (cos_theta > 0.9995f) {
            result = normalize(a * (1 - t) + b * t);
        } else {
            f32 theta = acosf(clamp(cos_theta, -1, 1));
            f32 thetap = theta * t;
            Quat4 qperp = normalize(b - a * cos_theta);
            result = a * cosf(thetap) + qperp * sinf(thetap);
        }
        
        return result;
    }

    inline Vec3 triangle_normal(Vec3 a, Vec3 b, Vec3 c) {
        Vec3 ab = b - a;
        Vec3 ac = c - a;
        return cross(ab, ac);
    }
}

typedef Math::Vec2 Vec2;
typedef Math::Vec3 Vec3;
typedef Math::Vec4 Vec4;
typedef Math::Vec2i Vec2i;
typedef Math::Vec3i Vec3i;
typedef Math::Vec4i Vec4i;
typedef Math::Mat4x4 Mat4x4;
typedef Math::Quat4 Quat4;
typedef Math::Rect Rect;

namespace Colors {
    const static Vec4 white = Vec4(1.0f);
    const static Vec4 black = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    const static Vec4 red = Vec4(1.0f, 0.0f, 0.0f, 1.0f);
    const static Vec4 green = Vec4(0.0f, 1.0f, 0.0f, 1.0f);
    const static Vec4 blue = Vec4(0.0f, 0.0f, 1.0f, 1.0f);
    const static Vec4 pink = Vec4(1.0f, 0.0f, 1.0f, 1.0f);
}

#define MATH_HH 1
#endif
