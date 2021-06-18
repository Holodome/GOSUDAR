#if !defined(CAMERA_HH)

#include "lib/lib.hh"
#include "game/ray_casting.hh"

struct Camera {
    f32 fov;
    f32 near_plane;
    f32 far_plane;
    
    Vec3 center_pos; 
    Vec3 pos;
    f32 pitch;
    f32 yaw;
    f32 distance_from_player;
    Mat4x4 view;
    Mat4x4 projection;
    Mat4x4 mvp;
    
    void init();
    void update(Input *input);    
    void recalculate_matrices(Vec2 winsize);
    
    Vec3 screen_to_world(Vec2 winsize, Vec2 screen);
    Vec3 uv_to_world(Vec2 uv);
};  

#define CAMERA_HH 1
#endif
