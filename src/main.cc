#include "game/game.hh"

int main(int argc, char **argv) {
    logger_init();
    logprintln("App", "start");
    {
        Game game;
        memset(&game, 0, sizeof(game));
        game_init(&game);
        while (game.is_running) {
            game_update_and_render(&game);
        }
        
        game_cleanup(&game);
    }
    
    bool mleak = false;
    if (Mem::times_alloced) {
        mleak = true;
        logprintln("Mem", "Memory leak detected: free not called %llu times", Mem::times_alloced);
    }
    
    if (!mleak) {
        logprintln("Mem", "No memory leaks detected");
    }
    logprintln("Mem", "Alloc called %llu times", Mem::alloc_count);
    logprintln("App", "end of main");
    logger_cleanup();
    return 0;
}
