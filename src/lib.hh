#if !defined(LIB_HH)

#include "general.hh"

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
    u32 mask = value >> (SIZE_OF(i32) * 8 - 1);
    return ((value ^ mask) - mask);
}

inline f32 Max(f32 a, f32 b) {
    return _mm_cvtss_f32(_mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
}

inline f32 Min(f32 a, f32 b) {
    return _mm_cvtss_f32(_mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
}

inline i32 Max_i32(i32 a, i32 b) {
    return _mm_cvt_ss2si(_mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
}

inline i32 Min_i32(i32 a, i32 b) {
    return _mm_cvt_ss2si(_mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
}

inline f32 Clamp(f32 value, f32 low, f32 high) {
    return _mm_cvtss_f32(_mm_min_ss(_mm_set_ss(high), _mm_max_ss(_mm_set_ss(value), _mm_set_ss(low))));
}

inline f32 Clamp_i32(f32 value, f32 low, f32 high) {
    return _mm_cvt_ss2si(_mm_min_ss(_mm_set_ss(high), _mm_max_ss(_mm_set_ss(value), _mm_set_ss(low))));
}

inline f32 Sin(f32 a) {
    return sinf(a);
}

inline f32 Cos(f32 a) {
    return cosf(a);
}

// @CLEANUP
void *os_alloc(size_t size);
void os_free(void *ptr);
struct OSMemoryBlock *os_alloc_block(size_t size);

struct Entropy {
    u32 state;
};  

inline u32 xorshift32(u32 *state) {
    u32 x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

f32 random(Entropy *entropy) {
    return (f32)xorshift32(&entropy->state) / ((f32)UINT32_MAX + 1);
}

f32 random_bilateral(Entropy *entropy) {
    return random(entropy) * 2.0f - 1.0f;
}

u64 random_int(Entropy *entropy, u64 modulo) {
    return xorshift32(&entropy->state) % modulo;
}


#define DEFAULT_ALIGNMENT (2 * sizeof(void *))

inline uintptr_t align_forward(uintptr_t ptr, size_t align) {
    assert(!(align & (align - 1)));
    
    uintptr_t p = ptr;
    uintptr_t a = align;
    uintptr_t modulo = p & (a - 1);
    
    if (modulo) {
        p += a - modulo;
    }
    return p;
}

#if 0
struct MemoryArena {
    u8 *data;
    size_t last_data_size;
    size_t data_size;
    size_t data_capacity;
    
    size_t peak_size;
    u32 temp_count;
};
#else 

struct MemoryArena {
    OSMemoryBlock *current_block;
    u32 minimum_block_size;
    u32 temp_count;
};

#endif 

#define alloc_struct(_arena, _type, ...) (_type *)alloc(_arena, sizeof(_type), ##__VA_ARGS__)
#define alloc_arr(_arena, _count, _type, ...) (_type *)alloc(_arena, _count * sizeof(_type), ##__VA_ARGS__)
#define alloc_string(_arena, _string, ...) (const char *)alloc_copy(_arena, _string, strlen(_string) + 1, ##__VA_ARGS__)


void *alloc(MemoryArena *arena, size_t size);
void arena_init(MemoryArena *arena, size_t minimum_block_size = 0) {
    arena->minimum_block_size = minimum_block_size;
    
}
void arena_clear(MemoryArena *arena);

#define bootstrap_alloc_struct(_type, _field, ...) (_type *)bootstrap_alloc_size(SIZE_OF(_type), STRUCT_OFFSET(_type, _field), __VA_ARGS__)
inline void *bootstrap_alloc_size(size_t size, size_t arena_offset, size_t minimal_block_size = MEGABYTES(4)) {
    MemoryArena bootstrap = {};
    arena_init(&bootstrap, minimal_block_size);
    void *struct_ptr = alloc(&bootstrap, size);
    *(MemoryArena *)((u8 *)struct_ptr + arena_offset) = bootstrap;
    return struct_ptr;
}


struct TempMemory {
    MemoryArena *arena;
    OSMemoryBlock *block;
    u64 block_used;
};

inline TempMemory begin_temp_memory(MemoryArena *arena);
inline void end_temp_memory(TempMemory mem);

static u32 crc32_lookup_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

inline u32 crc32(const void *data_init, size_t data_size, u32 seed = 0) {
    u32 crc = ~seed;
    u8  *data = (u8 *)data_init;
    u32 *crc32_lut = crc32_lookup_table;
    while (data_size--) {
        crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ *data++];
    }
    return ~crc;
}

inline u32 crc32_cstr(const char *cstr, u32 seed = 0) {
    u32 crc = ~seed;
    u8  *data = (u8 *)cstr;
    u32 *crc32_lut = crc32_lookup_table;
    while (*cstr) {
        crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ *cstr++];
    }
    return ~crc;
}

#define IS_POW2(_x) (!(_x & (_x - 1)))

const f32 HALF_PI = 1.57079632679f;
const f32 PI = 3.14159265359f;
const f32 TWO_PI = 6.28318530718f;

inline bool is_power_of_two(u64 x) {
    return !(x & (x - 1));
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

inline f32 saturate(f32 a) {
    return Clamp(a, 0, 1);
}

inline f32 lerp(f32 a, f32 b, f32 t) {
    return a * (1.0f - t) + b * t;
}

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
    
    bool operator==(vec2G<T> other) {
        return this->x == other.x && this->y == other.y;
    }
    bool operator!=(vec2G<T> other) {
        return !(*this == other);
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
    
    template <typename S>
        vec4G<T> operator+(vec4G<S> v) {
        return {x + v.x, y + v.y, z + v.z, w + v.w};
    }
    template <typename S>
        vec4G<T> operator-(vec4G<S> v) {
        return {x - v.x, y - v.y, z - v.z, w - v.w};
    }
    template <typename S>
        vec4G<T> operator*(vec4G<S> v) {
        return {x * v.x, y * v.y, z * v.z, w * v.w};
    }
    template <typename S>
        vec4G<T> operator/(vec4G<S> v) {
        return {x / v.x, y / v.y, z / v.z, w / v.w};
    }
    vec4G<T> operator*(T s) {
        return {x * s, y * s, z * s, w * s};
    }
    vec4G<T> operator/(T s) {
        return {x / s, y / s, z / s, w / s};
    }
    
    template <typename S>
        vec4G<T> &operator+=(vec4G<S> v) {
        return (*this = *this + v);
    }
    template <typename S>
        vec4G<T> &operator-=(vec4G<S> v) {
        return (*this = *this - v);
    }
    template <typename S>
        vec4G<T> &operator*=(vec4G<S> v) {
        return (*this = *this * v);
    }
    template <typename S>
        vec4G<T> &operator/=(vec4G<S> v) {
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

// Design decision for vectors was that functions related to them should not be members of structs
// because there are integer vectors and there is no point in adding stuff like cross product into them

inline f32 dot(vec2 a, vec2 b) {
    f32 result = a.x * b.x + a.y * b.y;
    return result;
}

inline f32 dot(vec3 a, vec3 b) {
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

inline f32 dot(vec4 a, vec4 b) {
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
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
    T result = a * Rsqrt(length_sq(a));
    return result;
}

struct Mat4x4 {
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
    
    const f32 *value_ptr() const {
        return &m00;
    }
    
    Mat4x4()  {};
    explicit Mat4x4(vec4 v0, vec4 v1, vec4 v2, vec4 v3) {
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
    
    static Mat4x4 translate(vec3 t) {
        Mat4x4 result = Mat4x4::identity();
        result.e[3][0] = t.x;
        result.e[3][1] = t.y;
        result.e[3][2] = t.z;
        return result;
    }
    
    static Mat4x4 scale(vec3 s) {
        Mat4x4 result = Mat4x4(Vec4(s.x,   0,   0,  0),
                               Vec4(0,   s.y,   0,  0),
                               Vec4(0,     0, s.z,  0),
                               Vec4(0,     0,   0,  1)
                               );
        return result;
    }
    
    static Mat4x4 rotation_x(f32 angle) {
        const f32 c = Cos(angle);
        const f32 s = Sin(angle);
        Mat4x4 r = Mat4x4(Vec4(1, 0, 0, 0),
                          Vec4(0, c,-s, 0),
                          Vec4(0, s, c, 0),
                          Vec4(0, 0, 0, 1));
        return(r);
    }
    
    static Mat4x4 rotation_y(f32 angle) {
        const f32 c = Cos(angle);
        const f32 s = Sin(angle);
        Mat4x4 r = Mat4x4(Vec4( c, 0, s, 0),
                          Vec4( 0, 1, 0, 0),
                          Vec4(-s, 0, c, 0),
                          Vec4( 0, 0, 0, 1));
        return(r);
    }
    
    static Mat4x4 rotation_z(f32 angle) {
        const f32 c = Cos(angle);
        const f32 s = Sin(angle);
        Mat4x4 r = Mat4x4(Vec4(c,-s, 0, 0),
                          Vec4(s, c, 0, 0),
                          Vec4(0, 0, 1, 0),
                          Vec4(0, 0, 0, 1));
        return(r);
    }
    
    static Mat4x4 rotation(f32 angle, vec3 a) {
        const f32 c = Cos(angle);
        const f32 s = Sin(angle);
        a = normalize(a);
        
        const f32 tx = (1.0f - c) * a.x;
        const f32 ty = (1.0f - c) * a.y;
        const f32 tz = (1.0f - c) * a.z;
        
        Mat4x4 r = Mat4x4(Vec4(c + tx * a.x, 		     tx * a.y - s * a.z, tx * a.z - s * a.y, 0),
                          Vec4(    ty * a.x - a.z * s, ty * a.y + c,       ty * a.z + s * a.x, 0),
                          Vec4(    tz * a.x + s * a.y, tz * a.y - s * a.x, tz * a.z + c,       0),
                          Vec4(0, 0, 0, 1));
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
        
        Mat4x4 inverse;
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
    
    static Mat4x4 look_at(vec3 from, vec3 to) {
        vec3 z = normalize(to - from);
        vec3 x = normalize(cross(Vec3(0, 1, 0), z));
        vec3 y = cross(z, x);
        Mat4x4 result = identity();
        result.e[0][0] = x.x;
        result.e[1][0] = x.y;
        result.e[2][0] = x.z;
        result.e[3][0] = -dot(x, from);
        result.e[0][1] = y.x;
        result.e[1][1] = y.y;
        result.e[2][1] = y.z;
        result.e[3][1] = -dot(y, from);
        result.e[0][2] = z.x;
        result.e[1][2] = z.y;
        result.e[2][2] = z.z;
        result.e[3][2] = -dot(z, from);
        return result;
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
    
    friend vec4 operator*(Mat4x4 a, vec4 v) {
        vec4 result;
        result.x = a.e[0][0] * v.x + a.e[1][0] * v.y + a.e[2][0] * v.z + a.e[3][0] * v.w;
        result.y = a.e[0][1] * v.x + a.e[1][1] * v.y + a.e[2][1] * v.z + a.e[3][1] * v.w;
        result.z = a.e[0][2] * v.x + a.e[1][2] * v.y + a.e[2][2] * v.z + a.e[3][2] * v.w;
        result.w = a.e[0][3] * v.x + a.e[1][3] * v.y + a.e[2][3] * v.z + a.e[3][3] * v.w;
        return result;
    }
    
    Mat4x4 &operator*=(Mat4x4 b) {
        return (*this = *this * b);
    }
    
    vec3 get_x() {
        return Vec3(e[0][0], e[1][0], e[2][0]);
    }
    
    vec3 get_y() {
        return Vec3(e[0][1], e[1][1], e[2][1]);
    }
    
    vec3 get_z() {
        return Vec3(e[0][2], e[1][2], e[2][2]);
    }
};

struct Rect {
    union {
        struct {
            vec2 p, s;
        };
        struct {
            f32 x, y, w, h;  
        };
    };
    
    Rect() {}
    explicit Rect(f32 x, f32 y, f32 w, f32 h)
        : x(x), y(y), w(w), h(h) {}
    explicit Rect(vec2 p, vec2 size)
        : p(p), s(size) {}
    static Rect minmax(vec2 min, vec2 max) {
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
    
    vec2 size() const {
        return Vec2(w, h);
    }
    
    vec2 middle() const {
        return p + s * 0.5f;
    }
    
    f32 middle_x() const {
        return x + w * 0.5f;
    }
    
    f32 middle_y() const {
        return y + h * 0.5f;
    }
    
    vec2 top_left() const {
        return p;
    }
    vec2 bottom_left() const {
        return p + Vec2(0, h);
    }
    vec2 top_right() const {
        return p + Vec2(w, 0);
    }
    vec2 bottom_right() const {
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
    
    void store_points(vec2 *pts) {
        pts[0] = top_left();
        pts[1] = bottom_left();
        pts[2] = top_right();
        pts[3] = bottom_right();
    }
    void store_points(vec3 *pts) {
        pts[0].xy = top_left();
        pts[1].xy = bottom_left();
        pts[2].xy = top_right();
        pts[3].xy = bottom_right();
        pts[0].z = 0;
        pts[1].z = 0;
        pts[2].z = 0;
        pts[3].z = 0;
    }
    
    static Rect move(Rect r, vec2 d) {
        return Rect::minmax(r.top_left() + d, r.bottom_right() + d);
    }
    
    bool collide(vec2 other) {
        return x < other.x && y < other.y && other.x < x + w && other.y < y + h;
    }
    
    bool collide(Rect rect) {
        bool result = true;
        if ((x + w < rect.x || x > rect.x + rect.w) ||
            (y + h < rect.y || y > rect.y + rect.h)) {
            result = false;
        }
        
        return result;
    }
    
    bool contains(Rect child) {
        return collide(child.top_left()) && collide(child.top_right()) &&
            collide(child.bottom_left()) && collide(child.bottom_right());
    }
    
    Rect clip(Rect child) {
        vec2 rmin = Vec2(Max(x, child.x), Max(y, child.y));
        vec2 rmax = Vec2(Min(x + w, child.x + child.w), Min(y + h, child.y + child.h));
        return Rect::minmax(rmin, rmax);
    }
    
    Rect join(Rect other) {
        vec2 rmin = Vec2(Min(x, other.x), Min(y, other.y));
        vec2 rmax = Vec2(Max(x + w, other.x + other.w), Max(y + h, other.y + other.h));
        return Rect::minmax(rmin, rmax);
    }
};

struct Bounds {
    vec3 min, max;
    
    explicit Bounds()
        : min(Vec3(F32_INFINITY)), max(Vec3(-F32_INFINITY)) {}
    explicit Bounds(vec3 min, vec3 max) 
        : min(min), max(max) {}
    explicit Bounds(vec3 a)
        : min(a), max(a) {}
    
    f32 surface_area() {
        vec3 d = max - min;
        return 2 * (d.x * d.y + d.y * d.z + d.z * d.z);
    }
    
    u32 longest_axis() {
        vec3 d = max - min;
        return (d.x > d.y && d.x > d.z) ? 0 : (d.y > d.x && d.y > d.z) ? 1 : 2;
    }
    
    static Bounds join(Bounds a, Bounds b) {
        Bounds result;
        result.min.x = Min(a.min.x, b.min.x);
        result.min.y = Min(a.min.y, b.min.y);
        result.min.z = Min(a.min.z, b.min.z);
        result.max.x = Max(a.max.x, b.max.x);
        result.max.y = Max(a.max.y, b.max.y);
        result.max.z = Max(a.max.z, b.max.z);
        return result;   
    }
    
    static Bounds extend(Bounds a, vec3 p) {
        Bounds result;
        result.min.x = Min(a.min.x, p.x);
        result.min.y = Min(a.min.y, p.y);
        result.min.z = Min(a.min.z, p.z);
        result.max.x = Max(a.max.x, p.x);
        result.max.y = Max(a.max.y, p.y);
        result.max.z = Max(a.max.z, p.z);
        return result;
    }
};

struct Quat4 {
    union {
        struct {
            f32 x, y, z, w;
        };
        vec4 v;
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
        f32 cy = Cos(yaw * 0.5f);
        f32 sy = Sin(yaw * 0.5f);
        f32 cp = Cos(pitch * 0.5f);
        f32 sp = Sin(pitch * 0.5f);
        f32 cr = Cos(roll * 0.5f);
        f32 sr = Sin(roll * 0.5f);
        
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
        f32 theta = acosf(Clamp(cos_theta, -1, 1));
        f32 thetap = theta * t;
        Quat4 qperp = normalize(b - a * cos_theta);
        result = a * Cos(thetap) + qperp * Sin(thetap);
    }
    
    return result;
}

inline vec3 triangle_normal(vec3 a, vec3 b, vec3 c) {
    vec3 ab = b - a;
    vec3 ac = c - a;
    return cross(ab, ac);
}

#define WHITE Vec4(1.0f)
#define BLACK Vec4(0.0f, 0.0f, 0.0f, 1.0f)
#define RED Vec4(1.0f, 0.0f, 0.0f, 1.0f)
#define GREEN Vec4(0.0f, 1.0f, 0.0f, 1.0f)
#define BLUE Vec4(0.0f, 0.0f, 1.0f, 1.0f)
#define PINK Vec4(1.0f, 0.0f, 1.0f, 1.0f)

inline vec3 xz(vec2 xz, f32 y = 0.0f) {
    return Vec3(xz.x, y, xz.y);
}

inline vec2 Floor(vec2 v) {
    return Vec2(Floor(v.x), Floor(v.y));
}

inline u32 utf8_encode(u32 utf32, u8 *dst) {
    u32 len = 0;
    if (utf32 <= 0x0000007F) {
        *dst = utf32;   
        len = 1;    
    } else if (utf32 <= 0x000007FF) {
        *dst++ = 0xC0 | (utf32 >> 6);
        *dst++ = 0x80 | (utf32 & 0x3F);
        len = 2;
    } else if (utf32 <= 0x0000FFFF) {
        *dst++ = 0xE0 | (utf32 >> 12);
        *dst++ = 0x80 | (utf32 >> 6 & 0x3F);
        *dst++ = 0x80 | (utf32 & 0x3F);
        len = 3;
    } else if (utf32 <= 0x0010FFFF) {
        *dst++ = 0xF0 | (utf32 >> 18);
        *dst++ = 0x80 | (utf32 >> 12 & 0x3F);
        *dst++ = 0x80 | (utf32 >> 6 & 0x3F);
        *dst++ = 0x80 | (utf32 & 0x3F);
        len = 4;
    } 
    return len;
}

inline u32 utf8_decode(const u8 *src, const u8 **new_dst) {
    u32 len = 0;
    u32 utf32 = 0;
    if ((src[0] & 0x80) == 0x00) {
        utf32 = src[0];
        len = 1;
    } else if ((src[0] & 0xE0) == 0xC0) {
        if ((src[1] & 0xC0) == 0x80) {
            utf32 = (src[0] & 0x1F) << 6 | (src[1] & 0x3F);     
            len = 2; 
        }
    } else if ((src[0] & 0xF0) == 0xE0) {
        if ((src[1] & 0xC0) == 0x80 && (src[2] & 0xC0) == 0x80) {
            utf32 = (src[0] & 0x0F) << 12 | (src[1] & 0x3F) << 6 | (src[2] & 0x3F);     
            len = 3;
        }
    } else if ((src[0] & 0xF8) == 0xF0) {
        if ((src[1] & 0xC0) == 0x80 && (src[2] & 0xC0) == 0x80 && (src[3] & 0xC0) == 0x80) {
            utf32 = (src[0] & 0x03) << 18 | (src[1] & 0x3F) << 12 | (src[2] & 0x3F) << 6 
                | (src[3] & 0x3F);     
            len = 4;
        }
    }
    
    *new_dst = src + len;
    return utf32;
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

bool ray_intersect_plane(vec3 plane_normal, f32 plane_d, vec3 o, vec3 d, f32 *t_out) {
    f32 denom = dot(plane_normal, d);
    if (Abs(denom) > 0.001f) {
        f32 t = (-plane_d - dot(plane_normal, o)) / denom;
        *t_out = t;
        return true;
    }
    return false;
}

struct SortEntry {
    f32 sort_key;
    u64 sort_index;
};

// Turn floating-point key to strictly ascending u32 value
inline u32 sort_key_to_u32(f32 sort_key) {
    u32 result = *(u32 *)&sort_key;
    if (result & 0x80000000) {
        result = ~result;
    } else {
        result |= 0x80000000;
    }
    return result;
}

void radix_sort(SortEntry *sort_a, SortEntry *sort_b, size_t count) {
    SortEntry *src = sort_a;
    SortEntry *dst = sort_b;
    for (u32 byte_idx = 0; byte_idx < 32; byte_idx += 8) {
        u32 sort_key_offsets[256] = {};
        for (u32 i = 0; i < count; ++i) {
            u32 radix_value = sort_key_to_u32(src[i].sort_key);
            u32 radix_piece = (radix_value >> byte_idx) & 0xFF;
            ++sort_key_offsets[radix_piece];
        }
        
        u32 total = 0;
        for (u32 sort_key_idx = 0; sort_key_idx < ARRAY_SIZE(sort_key_offsets); ++sort_key_idx) {
            u32 count = sort_key_offsets[sort_key_idx];
            sort_key_offsets[sort_key_idx] = total;
            total += count;
        }
        
        for (u32 i = 0; i < count; ++i) {
            u32 radix_value = sort_key_to_u32(src[i].sort_key);
            u32 radix_piece = (radix_value >> byte_idx) & 0xFF;
            dst[sort_key_offsets[radix_piece]++] = src[i];
        }
        
        SortEntry *temp = dst;
        dst = src;
        src = temp;
    }
}

// Abstracted texture
struct Texture {
    // Index in texture array
    u32 index;
    // Size, needed to recompute uvs from image space to texture array space
    u16 width;
    u16 height;
};  

static Texture INVALID_TEXTURE = { (u32)-1, 0, 0 };

struct AssetID {
    u32 value;
};

#define INVALID_ASSET_ID (AssetID {(u32)-1} )

#define PACK_4U8_TO_U32_(_a, _b, _c, _d) (((_a) << 0) | ((_b) << 8) | ((_c) << 16) | ((_d) << 24))
#define PACK_4U8_TO_U32(_a, _b, _c, _d) PACK_4U8_TO_U32_((u32)(_a), (u32)(_b), (u32)(_c), (u32)(_d))

inline u8 safe_truncate_u32_u8(u32 value) {
    assert(value <= MAX_VALUE(u8));
    return (u8)value;
}

inline u32 next_highest_pow_2(u32 v) {
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    return v;
}

i8 interlocked_increment(volatile i8 *value) {
    i8 result = (i8)_InterlockedExchangeAdd8((volatile char *)value, 1) + 1;
    return result;
}

i16 interlocked_increment(volatile i16 *value) {
    i16 result = (i16)_InterlockedIncrement16((volatile short *)value);
    return result;
}

i32 interlocked_increment(volatile i32 *value) {
    i32 result = (i32)_InterlockedIncrement((volatile long *)value);
    return result;
}

i64 interlocked_increment(volatile i64 *value) {
    i64 result = (i64)_InterlockedIncrement64((volatile long long *)value);
    return result;
}

i8 interlocked_decrement(volatile i8 *value) {
    i8 result = (i8)_InterlockedExchangeAdd8((volatile char *)value, -1) - 1;
    return result;
}

i16 interlocked_decrement(volatile i16 *value) {
    i16 result = (i16)_InterlockedDecrement16((volatile short *)value);
    return result;
}

i32 interlocked_decrement(volatile i32 *value) {
    i32 result = (i32)_InterlockedDecrement((volatile long *)value);
    return result;
}

i64 interlocked_decrement(volatile i64 *value) {
    i64 result = (i64)_InterlockedDecrement64((volatile long long *)value);
    return result;
}

i8 interlocked_add(volatile i8 *value, i8 A) {
    i8 result = (i8)_InterlockedExchangeAdd8((volatile char *)value, A);
    return result;
}

i16 interlocked_add(volatile i16 *value, i16 A) {
    i16 result = (i16)_InterlockedExchangeAdd16((volatile short *)value, A);
    return result;
}

i32 interlocked_add(volatile i32 *value, i32 A) {
    i32 result = (i32)_InterlockedExchangeAdd((volatile long *)value, A);
    return result;
}

i64 interlocked_add(volatile i64 *value, i64 A) {
    i64 result = (i64)_InterlockedExchangeAdd64((volatile long long *)value, A);
    return result;
}

i8 interlocked_exchange(volatile i8 *dest, i8 exchange) {
    i8 result = (i8)_InterlockedExchange8((volatile char *)dest, exchange);
    return result;
}

i16 interlocked_exchange(volatile i16 *dest, i16 exchange) {
    i16 result = (i16)_InterlockedExchange16((volatile short *)dest, exchange);
    return result;
}

i32 interlocked_exchange(volatile i32 *dest, i32 exchange) {
    i32 result = (i32)_InterlockedExchange((volatile long *)dest, exchange);
    return result;
}

i64 interlocked_exchange(volatile i64 *dest, i64 exchange) {
    i64 result = (i64)_InterlockedExchange64((volatile long long *)dest, exchange);
    return result;
}

i8 interlocked_compare_exchange(volatile i8 *dest, i8 exchange, i8 comparand) {
    i8 result = (i8)_InterlockedCompareExchange8((volatile char *)dest, exchange, comparand);
    return result;
}

i16 interlocked_compare_exchange(volatile i16 *dest, i16 exchange, i16 comparand) {
    i16 result = (i16)_InterlockedCompareExchange16((volatile short *)dest, exchange, comparand);
    return result;
}

i32 interlocked_compare_exchange(volatile i32 *dest, i32 exchange, i32 comparand) {
    i32 result = (i32)_InterlockedCompareExchange((volatile long *)dest, exchange, comparand);
    return result;
}

i64 interlocked_compare_exchange(volatile i64 *dest, i64 exchange, i64 comparand) {
    i64 result = (i64)_InterlockedCompareExchange64((volatile long long *)dest, exchange, comparand);
    return result;
}

#include "simd_math.hh"

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

// @TODO put this somewhere more reasonable...
struct GameLinks {
    struct RendererCommands *commands;
    struct Assets *assets;
    struct Platform *platform;
    struct InputManager *input;
    struct AudioSystem *audio;
    struct MemoryArena *frame_arena;
    struct UI *ui;
};

#define LIB_HH 1
#endif
