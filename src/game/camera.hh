#if !defined(CAMERA_HH)

#include "lib/lib.hh"
#include "game/ray_casting.hh"

struct Camera {
    Vec3 pos = Vec3(0); 
    Vec3 rot = Vec3(0);
    Mat4x4 view = Mat4x4::identity();
    Mat4x4 projection = Mat4x4::identity();
        
    void update_input();    
    void recalculate_matrices();
    
    Vec3 Camera::screen_to_world(Vec2 screen);
    Vec3 Camera::uv_to_world(Vec2 uv);
};  

#define CAMERA_HH 1
#endif
