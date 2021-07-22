#include "game.hh"

int main() {
    Game *game = game_init();
    while (game->is_running) {
        game_update_and_render(game);
    }
    return 0;
}
