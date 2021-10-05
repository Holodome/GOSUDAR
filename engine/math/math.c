#include "math/math.h"

f32 
rsqrt(f32 a) {
    return 1.0f / sqrtf(a);
}

f32 
sq(f32 a) {
    return a * a;
}

f32 
rad(f32 deg) {
    return deg * PI / 180.0f;
}

f32 
lerp(f32 a, f32 b, f32 t) {
    return (1.0f - t) * a + t * b;
}

f32 
clamp(f32 x, f32 low, f32 high) {
    return MAX(MIN(high, x), low);
}

f32 
saturate(f32 x) {
    return clamp(x, 0, 1);
}