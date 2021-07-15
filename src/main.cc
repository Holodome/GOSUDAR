#include "game.hh"

int main() {
    Game game = {};
    game_init(&game);
    while (game.is_running) {
        game_update_and_render(&game);
    }
    ExitProcess(0);
    return 0;
}
