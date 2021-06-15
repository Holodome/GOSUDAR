#if !defined(WORLD_HH)

#include "lib/lib.hh"

#include "game/camera.hh"

struct World {
    Camera camera = {};

    Vec2i map_size = Vec2i(10, 10);
    Vec3 player_pos = Vec3(0, 0, 0);
    Vec3 point_on_plane;

    void init();
    void cleanup();
    
    void update();
    void render();
    
    void get_billboard_positions(Vec3 mid_bottom, f32 width, f32 height, Vec3 out[4]);
};  

#define WORLD_HH 1
#endif
