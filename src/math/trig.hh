/*
Author: Holodome
Date: 03.10.2021
File: src/math/trig.hhh
Version: 0
*/
#pragma once 
#include "lib/general.hh"

#define HALF_PI 1.57079632679f
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f

namespace math {
    f32 sin(f32 a);
    f32 cos(f32 a); 
    f32 atan2(f32 y, f32 x);
    f32 rad(f32 deg);
    f32 unwind_rad(f32 a);    
} // namespace mat

