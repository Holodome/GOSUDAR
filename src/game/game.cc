#include "game/game.hh"

Game *game;

void Game::init() {
    logprint("Game", "Init start\n");
    is_running = true;   
    
    os.init();
    os.init_renderer_backend();
    renderer.init();
    
    os.prepare_to_start();
    dev_ui.begin_frame();
    dev_ui.font = new Font("c:\\windows\\fonts\\arial.ttf", 32);
    game_state.init();
    
    logprint("Game", "Init end\n");
}

void Game::cleanup() {
    logprintln("Game", "Cleanup");
    game_state.cleanup();
    delete dev_ui.font;
    renderer.cleanup();
    os.cleanup();
}

void Game::update() {
    os.update_input(&input);
    input.update();
    
    if (input.is_quit_requested) {
        is_running = false;
    }
    
    dev_ui.begin_frame();
    game_state.update();
    dev_ui.end_frame();
    
    // renderer.render();
    os.update_window();
}
