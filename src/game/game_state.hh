#if !defined(GAME_STATE_HH)

#include "lib/lib.hh"

struct Camera {
    Vec3 pos = Vec3(0); 
    Vec3 rot = Vec3(0);
    Mat4x4 view = Mat4x4::identity();
    Mat4x4 projection = Mat4x4::identity();
    
    // UV is in [-1; 1]
    Vec3 screen_to_world(Vec2 uv) {
        Vec3 ray_dc = Vec3(uv.x, uv.y, 1.0f);
        Vec4 ray_clip = Vec4(ray_dc.xy, -1.0f, 1.0f);
        Vec4 ray_eye = Mat4x4::inverse(this->projection) * ray_clip;
        ray_eye.z = -1.0f;
        ray_eye.w = 0.0f;
        Vec3 ray_world = Math::normalize((Mat4x4::inverse(this->view) * ray_eye).xyz);
        return ray_world;
    }
};  

struct Settings {
    bool fullscreen = false;
    bool enable_devui = false; // Press f3 to enable, f8 to focus mouse 
    bool focus_devui = true;
};  

struct GameState {
    DevUI dev_ui = {};
    
    Settings settings = {};
    Camera camera = {};

    Mesh *cube = 0;
    Mesh *rect = 0;
    
    Vec3 point_on_plane;
    
    void init();
    void cleanup();
    
    void update();
    // private
    void update_camera();
    void update_input();
    void update_logic();
    void render();
};

#define GAME_STATE_HH 1
#endif
