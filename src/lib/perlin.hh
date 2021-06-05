#if !defined(PERLIN_HH)

#include "lib/general.hh"
#include "lib/array.hh"
#include "lib/rng.hh"

struct Perlin {
    u32 seed;
    f32 roughness;
    f32 amplitude;
    u32 octaves;
    
    Perlin(f32 roughness, f32 amplitude, u32 octaves, u32 seed = 0) 
        : roughness(roughness), amplitude(amplitude), octaves(octaves), seed(seed) {
    }
    
    f32 get_perlin_noise(i32 x, i32 y) {
        f32 result = 0;
        f32 d = 2 << (octaves - 1);
        for (u32 i = 0; i < octaves; ++i) {
            f32 freq = (2 << i) / d;
            f32 amp = powf(roughness, i) * amplitude;
            result += get_cell_smooth_noise(x * freq, y * freq) * amp;    
        }
        return result;
    }
    
    f32 get_cell_smooth_noise(f32 x, f32 y) {
        i32 xi = (i32)x;
        i32 yi = (i32)y;
        f32 xf = x - xi;
        f32 yf = y - yi;
        
        f32 v00 = get_smooth_noise(xi, yi);
        f32 v01 = get_smooth_noise(xi, yi + 1);
        f32 v10 = get_smooth_noise(xi + 1, yi);
        f32 v11 = get_smooth_noise(xi + 1, yi + 1);
        f32 xc = (1.0f - cosf(xf * Math::PI)) * 0.5f;
        f32 yc = (1.0f - cosf(yf * Math::PI)) * 0.5f;
        return Math::bilerp(v00, v01, v10, v11, xc, yc);       
    }
    
    f32 get_smooth_noise(i32 x, i32 y) {
        f32 corners = (get_noise(x - 1, y - 1) + get_noise(x + 1, y - 1) + 
                       get_noise(x - 1, y + 1) + get_noise(x + 1, y + 1)) / 16.0f;
        f32 sides = (get_noise(x - 1, y) + get_noise(x + 1, y) + 
                     get_noise(x, y - 1) + get_noise(x, y + 1)) / 8.0f;
        f32 center = get_noise(x, y) / 4.0f;
        return corners + sides + center;
    }
    
    f32 get_noise(i32 x, i32 y) {
        return RNG(x * 496332 + y * 325176 + seed).bilaterial();
    }
};

#define PERLIN_HH 1
#endif
