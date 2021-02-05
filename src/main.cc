#include "game/game.hh"

int 
main(int argc, char **argv) {
    Game game = Game();
    while (game.is_running) {
        game.update();
    }
    
    return 0;
}