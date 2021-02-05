#if !defined(GAME_H)

#include "lib/lib.hh"

#include "os/os.hh"
#include "renderer/assets.hh"
#include "renderer/renderer.hh"
// #include "game/devui.hh"
#include "game/game_state.hh"

struct Game {
    bool is_running;
    OS os;
    Renderer renderer;
    Input input;
    
    GameState game_state;
    
    Game();
    void update();  
};

extern Game *game;

#define GAME_H 1
#endif
