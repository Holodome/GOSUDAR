#include "math/math.hh"

#include <math.h>

namespace math
{
    
i32 truncate_i32(f32 value) {
    return (i32)value;
}

f32 truncate(f32 value) {
    return (f32)truncate_i32(value);
}

i32 floor_i32(f32 value) {
    return (i32)floorf(value);
}

f32 floor(f32 value) {
    return floorf(value);
}

i32 round_i32(f32 value) {
    return (i32)roundf(value);
}

f32 round(f32 value) {
    return (f32)round_i32(value);
}

i32 ceil_i32(f32 value) {
    return ceilf(value);
}

f32 ceil(f32 value) {
    return (f32)ceil_i32(value);
}

f32 sqrt(f32 value) {
    return sqrtf(value);
}

f32 rsqrt(f32 value) {
    return 1.0f / sqrt(value);
}

f32 abs(f32 value) {
    u32 bits = *(u32 *)&value;
    bits &= 0x7FFFFFFF;
    return *(f32 *)&bits;
}

i32 abs(i32 value) {
    u32 mask = value >> 31;
    return ((value ^ mask) - mask);
}

f32 max(f32 a, f32 b) {
    return fmaxf(a, b);
}

f32 min(f32 a, f32 b) {
    return fminf(a, b);
}

i64 max(i64 a, i64 b) {
    return a > b ? a : b;
}

i64 min(i64 a, i64 b) {
    return a < b ? a : b;
}

f32 clamp(f32 value, f32 low, f32 high) {
    return max(low, min(value, high));
}

i64 clamp(i64 value, i64 low, i64 high) {
    return (value < low ? low : value > high ? high : value);
}

f32 lerp(f32 a, f32 b, f32 t) {
    return a * (1.0f - t) + b * t;
}
} // namespace math
