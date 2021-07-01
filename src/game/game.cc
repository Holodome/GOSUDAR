#include "game/game.hh"

void game_init(Game *game) {
    logprintln("Game", "Init start");
    game->is_running = true;   

    game->debug_state = DEBUG_create();
    
    game->os.init();
    game->os.init_renderer_backend();
    f32 init_start = game->os.get_time();
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
    
    game->input.input = &game->input_;
    game->input.access_token = INPUT_ACCESS_TOKEN_NO_LOCK;
    
    f32 init_end = game->os.get_time();
    
    logprintln("Game", "Init took %llums", (u64)((init_end - init_start) * 1000));
}

void game_cleanup(Game *game) {
    logprintln("Game", "Cleanup");
    os_free(game->game_state.frame_arena.data);
    os_free(game->game_state.arena.data);
    game->assets.cleanup();
    os_free(game->assets.arena.data);
    renderer_cleanup(&game->renderer);
    os_free(game->renderer.arena.data);
    game->os.cleanup();
}

void game_update_and_render(Game *game) {
    FRAME_MARKER();
    DEBUG_begin_frame(game->debug_state);
    game->os.update_input(&game->input_);
#define MIN_DT 0.001f
#define MAX_DT 0.1f
    game->input_.dt = Math::clamp(game->input_.dt, MIN_DT, MAX_DT);
    
    if (is_key_pressed(&game->input, Key::Escape, INPUT_ACCESS_TOKEN_ALL) || game->input_.is_quit_requested) {
        game->is_running = false;
    }    
    RendererCommands *commands = renderer_begin_frame(&game->renderer, window_size(&game->input), Vec4(0.2));
    update_and_render(&game->game_state, &game->input, commands, &game->assets);
    DEBUG_update(game->debug_state, &game->game_state, &game->input, commands);
    renderer_end_frame(&game->renderer);
    game->os.update_window();
    DEBUG_frame_end(game->debug_state);
}
