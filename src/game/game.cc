#include "game/game.hh"

void game_init(Game *game) {
    logprintln("Game", "Init start");
    game->is_running = true;   
    game->debug_state = DEBUG_init();
    game->os = os_init();
    init_renderer_backend(game->os);
    renderer_init(&game->renderer);
    
    size_t assets_arena_size = MEGABYTES(256);
    arena_init(&game->assets.arena, os_alloc(assets_arena_size), assets_arena_size);
    game->assets.renderer = &game->renderer;
    game->assets.init();
    game_state_init(&game->game_state);
}

void game_cleanup(Game *game) {
    logprintln("Game", "Cleanup");
    game->assets.cleanup();
}

void game_update_and_render(Game *game) {
    FRAME_MARKER();
    DEBUG_begin_frame(game->debug_state);
    
    DEBUG_SWITCH(&game->game_state.show_grid, "Show grid");
    DEBUG_BEGIN_VALUE_BLOCK("Debug");
    DEBUG_VALUE(game->debug_state->value_blocks_allocated, "Value blocks allocated");
    DEBUG_VALUE(game->debug_state->debug_open_blocks_allocated, "Blocks allocated");
    DEBUG_VALUE(game->debug_state->debug_values_allocated, "Values allocated");
    DEBUG_END_VALUE_BLOCK();
    DEBUG_BEGIN_VALUE_BLOCK("Memory");
    DEBUG_VALUE(game->debug_state->arena.peak_size >> 20, "Debug arena size");
    DEBUG_VALUE(game->game_state.frame_arena.peak_size >> 20, "Frame arena size");
    DEBUG_VALUE(game->game_state.arena.peak_size >> 20, "Game arena size");
    DEBUG_VALUE(game->renderer.arena.peak_size >> 20, "Renderer arena size");
    DEBUG_VALUE(game->assets.arena.peak_size >> 20, "Assets arena size");
    DEBUG_END_VALUE_BLOCK();
    
    Input *os_input = update_input(game->os);
    InputManager input_ = create_input_manager(os_input);
    InputManager *input = &input_;
    if (is_key_pressed(input, KEY_ESCAPE, INPUT_ACCESS_TOKEN_ALL) || os_input->is_quit_requested) {
        game->is_running = false;
    }    
    
    RendererCommands *commands = renderer_begin_frame(&game->renderer, window_size(input), Vec4(0.2));
    DEBUG_update(game->debug_state, input, commands, &game->assets);
    update_and_render(&game->game_state, input, commands, &game->assets);
    renderer_end_frame(&game->renderer);
    update_window(game->os);
    DEBUG_frame_end(game->debug_state);
}
