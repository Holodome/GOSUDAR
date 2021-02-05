#include "game/game.hh"

Game *game;

char *skip_to_next_line(char *cursor) {
    while (*cursor && *cursor != '\n') {
        ++cursor;
    }
    
    if (*cursor == '\n') {
        ++cursor;
    }
    return cursor;
}

Game::Game() {
    game = this;
    
    is_running = true;   
    
    os.init();
    os.init_renderer_backend();
    renderer.init();
    
    os.prepare_to_start();
    game_state.init();
}

void Game::update() {
    os.update_input(&input);
    input.update();
    
    renderer.begin_frame(input.winsize);
    
    if (input.is_quit_requested) {
        is_running = false;
    }
    
    game_state.update();
    
    // renderer.render();
    os.update_window();
}
