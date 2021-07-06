#if !defined(GAME_H)

#include "lib.hh"

#include "debug.hh"
#include "os.hh"
#include "renderer.hh"
#include "assets.hh"
#include "render_group.hh"

#include "game_state.hh"

// Game is a object that decribes program as one element 
// It contains several elements that are all used in game state
// Game state is the logic of the game
// It decides how to use all data recived from input, what to render when to close etc.
struct Game {
    // Global temporary arena - easy way to allocate some stuff that doesn't need to exist for long
    // Just dump any temporary data here - during development and maybe even after we can afford not to 
    // worry about a couple of megabytes
    MemoryArena frame_arena;
    
    OS *os;
    Renderer renderer;
    Assets *assets;
    DebugState *debug_state;
    
    bool is_running;
    
    GameState game_state;
};

void game_init(Game *game);
void game_cleanup(Game *game);
void game_update_and_render(Game *game);

#define GAME_H 1
#endif
