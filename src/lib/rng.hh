#if !defined(RNG_HH)

#include "lib/general.hh"
#include "lib/math.hh"

struct Entropy {
    u32 state;
};  

inline u32 xorshift32(u32 *state) {
    u32 x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

f32 random(Entropy *entropy) {
    return (f32)xorshift32(&entropy->state) / ((f32)UINT32_MAX + 1);
}

f32 random_bilateral(Entropy *entropy) {
    return random(entropy) * 2.0f - 1.0f;
}

u64 random_int(Entropy *entropy, u64 modulo) {
    return xorshift32(&entropy->state) % modulo;
}

#define RNG_HH 1
#endif
