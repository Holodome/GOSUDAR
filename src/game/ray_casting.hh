#if !defined(RAY_CASTING_HH)

#include "lib/lib.hh"

inline bool ray_intersect_plane(Vec3 plane_normal, f32 plane_d, Vec3 o, Vec3 d, f32 *t_out) {
    f32 denom = Math::dot(plane_normal, d);
    if (fabs(denom) > 0.001f) {
        f32 t = (-plane_d - Math::dot(plane_normal, o)) / denom;
        *t_out = t;
        return true;
    }
    return false;
}

#define RAY_CASTING_HH 1
#endif
