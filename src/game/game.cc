#include "game/game.hh"

Game *game;

void Game::init() {
    logprintln("Game", "Init start");
    is_running = true;   
    
    os.init();
    os.init_renderer_backend();
    f32 init_start = game->os.get_time();
    renderer.init();
    // Init assets after initing renderer
    this->tex_lib.init();
    // This is kinda circly- but renderer params have to be set after texture
    
    game_state.init();
    f32 init_end = game->os.get_time();
    // Effectively it is not whole init time, but time of game initialization-related routines,
    // cause there is little point in recording time spend on os-related stuff. It should be profiled separately
    // and is (probably) inconsistent due to tf os does 
    logprintln("Game", "Init took %llums", (u64)((init_end - init_start) * 1000));
}

void Game::cleanup() {
    logprintln("Game", "Cleanup");
    game_state.cleanup();
    renderer.cleanup();
    os.cleanup();
}

void Game::update() {
    os.update_input(&input);
    input.update();
    
    if (input.is_quit_requested) {
        is_running = false;
    }
    
    renderer.begin_frame();
    game_state.update();
    
    // renderer.render();
    os.update_window();
}
