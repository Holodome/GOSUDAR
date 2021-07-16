#if !defined(SIMD_MATH_HH)

#include "general.hh"
#include <intrin.h>

union f32_4x {
    __m128 p;
    f32 e[4];
};

inline f32_4x zero_F32_4x() {
    f32_4x result;
    result.p = _mm_setzero_ps();
    return result;
}

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

#define SIMD_MATH_HH 1
#endif 