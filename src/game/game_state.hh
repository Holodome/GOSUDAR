#if !defined(GAME_STATE_HH)

#include "lib/lib.hh"

struct GameState {
    // DevUI ui;
    
    GameState();
    void init();
    void update();
};

#define GAME_STATE_HH 1
#endif
