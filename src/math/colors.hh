/*
Author: Holodome
Date: 03.10.2021
File: src/math/colors.hhh
Version: 0
*/
#pragma once 
#include "lib/general.hh"

namespace math
{
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
} // namespace math
