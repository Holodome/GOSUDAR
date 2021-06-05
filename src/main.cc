#include "game/game.hh"

int main(int argc, char **argv) {
    logprintln("App", "start");
    
    {
        Game local_game = Game();
        game = &local_game;
        game->init();
        while (game->is_running) {
            game->update();
        }
        
        game->cleanup();
    }
    
    if (Mem::times_alloced) {
        logprintln("Mem", "Memory leak detected: free not called %llu times", Mem::times_alloced);
    } else {
        logprintln("Mem", "No memory leaks detected");
    }
    
    logprintln("App", "end of main");
    return 0;
}
