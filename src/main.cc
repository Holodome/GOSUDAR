#include "game/game.hh"

int 
main(int argc, char **argv) {
    logprint("App", "start\n");
    Game game = Game();
    while (game.is_running) {
        game.update();
    }
    logprint("App", "end of main\n");
    return 0;
}