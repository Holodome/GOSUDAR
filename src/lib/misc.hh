#if !defined(MISC_HH)

#include "lib/general.hh"
#include "lib/math.hh"

inline u32 
pack_rgba_4x8(u32 r, u32 g, u32 b, u32 a) {
    assert(r <= 0xFF && b <= 0xFF && b <= 0xFF && a <= 0xFF);
    return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

inline u32
pack_rgba_4x8_linear1(f32 r, f32 b, f32 g, f32 a) {
    u32 ru = roundf(clamp(r, 0, 1 - FLT_EPSILON) * 255);
    u32 gu = roundf(clamp(g, 0, 1 - FLT_EPSILON) * 255);
    u32 bu = roundf(clamp(b, 0, 1 - FLT_EPSILON) * 255);
    u32 au = roundf(clamp(a, 0, 1 - FLT_EPSILON) * 255);
    return pack_rgba_4x8(ru, gu, bu, au);
}

inline u32 
pack_rgba_4x8_linear1(Vec4 rgba) {
    return pack_rgba_4x8_linear1(rgba.r, rgba.g, rgba.b, rgba.a);
}


#define MISC_HH 1
#endif
