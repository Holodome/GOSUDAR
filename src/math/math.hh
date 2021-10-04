/*
Author: Holodome
Date: 03.10.2021
File: src/math/math.hhh
Version: 0
*/
#pragma once 
#include "lib/general.hh"

namespace math
{
    i32 truncate_i32(f32 value);
    f32 truncate(f32 value);
    i32 floor_i32(f32 value);
    f32 floor(f32 value);
    i32 round_i32(f32 value);
    f32 round(f32 value);
    i32 ceil_i32(f32 value);
    f32 ceil(f32 value);
    f32 sqrt(f32 value);
    f32 rsqrt(f32 value);
    f32 abs(f32 value);
    i32 abs(i32 value);
    f32 max(f32 a, f32 b);
    f32 min(f32 a, f32 b);
    i64 maxi(i64 a, i64 b);
    i64 mini(i64 a, i64 b);
    f32 clamp(f32 value, f32 low, f32 high);
    i64 clampi(i64 value, i64 low, i64 high);
    f32 lerp(f32 a, f32 b, f32 t);
} // namespace math
