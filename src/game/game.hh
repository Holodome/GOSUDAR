#if !defined(GAME_H)

#include "lib/lib.hh"

#include "framework/os.hh"
#include "framework/renderer.hh"
#include "framework/assets.hh"
#include "framework/devui.hh"

#include "game/world.hh"

enum DevMode {
    DevMode_None           = 0x0,
    DevMode_DevUI          = 0x1,
    DevMode_DevUIFocused   = 0x2,
    DevMode_FreeCamera     = 0x4,
    DevMode_StopSimulation = 0x8
};  

// Game is a object that decribes program as one element 
// It contains several elements that are all used in game state
// Game state is the logic of the game
// It decides how to use all data recived from input, what to render when to close etc.
struct Game {
    OS os;
    Renderer renderer;
    Input input;
    Assets assets;
    
    bool is_running;
    
    bool draw_sprite_frames;
    bool fullscreen;
    u32 dev_mode; // DevMode
    World world;
    
    MemoryArena frame_arena;
};

void game_init(Game *game);
void game_cleanup(Game *game);
void game_update_and_render(Game *game);

#define GAME_H 1
#endif
