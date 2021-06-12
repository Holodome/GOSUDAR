#if !defined(RAY_CASTING_HH)

#include "lib/lib.hh"

struct Ray {
    Vec3 orig;
    Vec3 dir;  
    
    Ray(Vec3 orig, Vec3 dir) {
        this->orig = orig;
        this->dir = dir;
    }
};

bool ray_intersect_plane(Vec3 plane_normal, f32 plane_d, const Ray &ray, f32 *t_out) {
    f32 denom = Math::dot(plane_normal, ray.dir);
    if (fabs(denom) > 0.001f) {
        f32 t = (-plane_d - Math::dot(plane_normal, ray.orig)) / denom;
        *t_out = t;
        return true;
    }
    return false;
}

#define RAY_CASTING_HH 1
#endif
