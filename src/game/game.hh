#if !defined(GAME_H)

#include "lib/lib.hh"

#include "framework/os.hh"
#include "framework/renderer.hh"
#include "framework/devui.hh"
#include "framework/texture_library.hh"
#include "game/game_state.hh"

// Game is a object that decribes program as one element 
// It contains several elements that are all used in game state
// Game state is the logic of the game
// It decides how to use all data recived from input, what to render when to close etc.
struct Game {
    bool is_running = false;
    OS os = OS();
    Renderer renderer = Renderer();
    Input input = Input();
    // @TODO Assets...    
    TextureLibrary tex_lib = TextureLibrary();
    
    GameState game_state = GameState();
    
    void init();
    void cleanup();
    
    void update();  
};

extern Game *game;

#define GAME_H 1
#endif
