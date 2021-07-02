#include "game/game.hh"

int main() {
    logger_init();
    logprintln("App", "start");
    Game game = {};
    game_init(&game);
    while (game.is_running) {
        game_update_and_render(&game);
    }
    
    game_cleanup(&game);
    
    logprintln("App", "end of main");
    logger_cleanup();
    return 0;
}
