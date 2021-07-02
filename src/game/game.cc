#include "game/game.hh"

void game_init(Game *game) {
    logprintln("Game", "Init start");
    game->is_running = true;   

    game->debug_state = DEBUG_create();
    
    game->os = os_init();
    init_renderer_backend(game->os);
    
    size_t renderer_arena_size = MEGABYTES(256);
    arena_init(&game->renderer.arena, os_alloc(renderer_arena_size), renderer_arena_size);
    renderer_init(&game->renderer);
    
    size_t assets_arena_size = MEGABYTES(256);
    arena_init(&game->assets.arena, os_alloc(assets_arena_size), assets_arena_size);
    game->assets.renderer = &game->renderer;
    game->assets.init();
    
    DEBUG_init(game->debug_state, &game->assets);
    
    size_t frame_arena_size = MEGABYTES(256);
    arena_init(&game->game_state.frame_arena, os_alloc(frame_arena_size), frame_arena_size);
    size_t world_arena_size = MEGABYTES(512);
    arena_init(&game->game_state.arena, os_alloc(world_arena_size), world_arena_size);
    game_state_init(&game->game_state);
}

void game_cleanup(Game *game) {
    logprintln("Game", "Cleanup");
    os_free(game->game_state.frame_arena.data);
    os_free(game->game_state.arena.data);
    game->assets.cleanup();
    os_free(game->assets.arena.data);
    renderer_cleanup(&game->renderer);
    os_free(game->renderer.arena.data);
}

void game_update_and_render(Game *game) {
    FRAME_MARKER();
    DEBUG_begin_frame(game->debug_state);
    Input *os_input = update_input(game->os);
    InputManager input_ = create_input_manager(os_input);
    InputManager *input = &input_;
    if (is_key_pressed(input, KEY_ESCAPE, INPUT_ACCESS_TOKEN_ALL) || os_input->is_quit_requested) {
        game->is_running = false;
    }    
    
    RendererCommands *commands = renderer_begin_frame(&game->renderer, window_size(input), Vec4(0.2));
    update_and_render(&game->game_state, input, commands, &game->assets);
    DEBUG_update(game->debug_state, &game->game_state, input, commands);
    renderer_end_frame(&game->renderer);
    update_window(game->os);
    DEBUG_frame_end(game->debug_state);
}
