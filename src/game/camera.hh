#if !defined(CAMERA_HH)

#include "lib/lib.hh"
#include "game/ray_casting.hh"

struct Camera {
    f32 fov = Math::rad(60.0f);
    f32 near_plane = 0.1f;
    f32 far_plane = 100.0f;
    
    Vec3 center_pos = Vec3(0); 
    Vec3 pos = Vec3(0);
    f32 pitch = 0.0f;
    f32 yaw = 0.0f;
    f32 distance_from_player = 5.0f;
    Mat4x4 view = Mat4x4::identity();
    Mat4x4 projection = Mat4x4::identity();
    Mat4x4 mvp = Mat4x4::identity();
    
    void init();
    void update();    
    void recalculate_matrices();
    
    Vec3 screen_to_world(Vec2 screen);
    Vec3 uv_to_world(Vec2 uv);
};  

#define CAMERA_HH 1
#endif
