/*
Author: Holodome
Date: 04.10.2021
File: shared/math/math.h
Version: 0
*/
#pragma once

#include "lib/general.h"

#define PI      3.14159265359f
#define TAU     6.28318530718f
#define HALF_PI 1.57079632679f
#define INV_PI  0.31830988618f
#define MIN(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#define MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#define ABS(_a) ((_a) < 0 ? -(_a) : (_a))

// for sinf, cosf, sqrtf etc.
#include <math.h>

f32 rsqrt(f32 a);
f32 sq(f32 a);
f32 rad(f32 deg);
f32 lerp(f32 a, f32 b, f32 t);
f32 clamp(f32 x, f32 low, f32 high);
f32 saturate(f32 x);