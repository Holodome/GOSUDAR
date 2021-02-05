#if !defined(RNG_HH)

#include "general.hh"
#include "math.hh"

struct RNG {
    u32 a = 0, b = 0, c = 0, d = 0, e = 0, count = 0;
   
    void seed(u32 s) {
        memset(this, 0, sizeof(*this));
        e = s;
    } 
    
    RNG() = default;
    RNG(u32 s) {
        seed(s);
    }  
    
    u32 xorwow32() {
        u32 t = e;
        u32 s = a;
        e = d;
        d = c;
        c = b;
        b = s;
        t ^= t >> 2;
        t ^= t << 1;
        t ^= s ^ (s << 4);
        a = t;
        count += 0x587C5;
        return t + count;
    }
    
    f32 random() {
        return (f32)xorwow32() / ((f32)U32_MAX + 1);
    }
    
    f32 bilaterial() {
        return random() * 2.0f - 1.0f;
    }
    
    f32 uniform(f32 low, f32 high) {
        return lerp(low, high, random());
    }
    
    u64 rint(u64 modulo) {
        assert(modulo);
        return xorwow32() % modulo;
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


#define RNG_HH 1
#endif
