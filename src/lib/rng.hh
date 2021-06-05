#if !defined(RNG_HH)

#include "lib/general.hh"
#include "lib/math.hh"

struct RNG {
    u32 state = 32897342509;
    u32 xorshift32() {
        u32 x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }
    
    void seed(u32 s) {
        state = s;
    }
    
    RNG(u32 s) {
        seed(s);
    }
    
    // Returns random value in range [0, 1)
    f32 random() {
        return (f32)xorshift32() / ((f32)UINT32_MAX + 1);
    }
    
    // Returns random value in range [-1, 1)
    f32 bilaterial() {
        return random() * 2.0f - 1.0f;
    }
    
    // Returns random value in range [low, high)
    f32 uniform(f32 low, f32 high) {
        return Math::lerp(low, high, random());
    }
    
    u64 rint(u64 modulo) {
        assert(modulo);
        return xorshift32() % modulo;
    }
    
    u64 rint_range(u64 low, u64 high) {
        return low + rint(high - low);
    }
    
    Vec3 unit_vector() {
        return Vec3(random(), random(), random());
    }
    
    Vec3 unit_sphere() {
        Vec3 result;
        do {
            result = unit_vector();
        } while(length_sq(result) >= 1.0f);
        return result;
    }
    
    Vec3 unit_disk() {
        Vec3 result;
        do {
            result = Vec3(bilaterial(), bilaterial(), 0);
        } while(length_sq(result) >= 1.0f);
        return result;
    }
    
    Vec3 hemisphere(Vec3 n) {
        Vec3 result = unit_sphere();
        if (dot(n, result) > 0.0f) {
            
        } else {
            result = -result;
        }
        return result;
    }
    
    Vec3 vector(f32 low, f32 high) {
        return Vec3(uniform(low, high), uniform(low, high), uniform(low, high));
    }
};

static RNG rng(100);

#define RNG_HH 1
#endif
