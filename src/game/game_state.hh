#if !defined(GAME_STATE_HH)

#include "lib/lib.hh"

#include "game/camera.hh"

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
    void update_input();
    void update_logic();
    void render();
};

#define GAME_STATE_HH 1
#endif
