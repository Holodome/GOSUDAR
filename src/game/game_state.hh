#if !defined(GAME_STATE_HH)

#include "lib/lib.hh"

#include "game/world.hh"

enum DevMode {
    DevMode_None           = 0x0,
    DevMode_DevUI          = 0x1,
    DevMode_DevUIFocused   = 0x2,
    DevMode_FreeCamera     = 0x4,
    DevMode_StopSimulation = 0x8
};  

struct GameState {
    DevUI local_dev_ui = {};
    
    bool draw_sprite_frames = false;
    bool fullscreen = false;
    u32 dev_mode; // DevMode
    i32 wood_count = 0;
    World world = {};
    
    MemoryArena frame_arena;
    
    void init();
    void cleanup();
    
    void update();
    
    void update_logic();
    void render();
};

#define GAME_STATE_HH 1
#endif
