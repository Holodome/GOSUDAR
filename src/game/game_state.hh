#if !defined(GAME_STATE_HH)

#include "lib/lib.hh"

struct GameState {
    // DevUI ui;
    Vec3 camera_pos;
    Vec3 camera_rot;
    
    GameState();
    void init();
    void update();
};

#define GAME_STATE_HH 1
#endif
