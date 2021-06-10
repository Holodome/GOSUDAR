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
    
    bool mleak = false;
    if (Mem::times_alloced) {
        mleak = true;
        logprintln("Mem", "Memory leak detected: free not called %llu times", Mem::times_alloced);
    }
    if (Mem::currently_allocated) {
        mleak = true;
        logprintln("Mem", "Memory leak detected: %llu bytes are not deallocated", Mem::currently_allocated);
    }
    
    if (!mleak) {
        logprintln("Mem", "No memory leaks detected");
    }
    
    logprintln("App", "end of main");
    return 0;
}
