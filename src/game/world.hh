#if !defined(WORLD_HH)

#include "lib/lib.hh"

#include "game/camera.hh"

struct World {
    Camera camera = {};

    Vec3 player_pos = Vec3(0, 0, 0);
    Vec3 point_on_plane;

    void init();
    void cleanup();
    
    void update();
    void render();
};  

#define WORLD_HH 1
#endif
