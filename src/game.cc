#include "game.hh"

void game_init(Game *game) {
    game->is_running = true;   
#define FRAME_ARENA_SIZE MEGABYTES(256)
    arena_init(&game->frame_arena, os_alloc(FRAME_ARENA_SIZE), FRAME_ARENA_SIZE);
    game->debug_state = DEBUG_init();
    RendererSettings default_renderer_settings;
    default_renderer_settings.display_size = Vec2(0);
    default_renderer_settings.filtered = true;
    game->os = os_init(&default_renderer_settings.display_size);
    renderer_init(&game->renderer, default_renderer_settings);
    game->assets = assets_init(&game->renderer, &game->frame_arena);
    game_state_init(&game->game_state, &game->frame_arena);
    game->game_state.renderer_settings = default_renderer_settings;
}

void game_cleanup(Game *game) {
}

void game_update_and_render(Game *game) {
    FRAME_MARKER();
    arena_clear(&game->frame_arena);
    DEBUG_begin_frame(game->debug_state);
    
    Platform *platform = os_begin_frame(game->os);
    InputManager input_ = create_input_manager(platform);
    InputManager *input = &input_;
    if (is_key_pressed(input, KEY_ESCAPE) || platform->is_quit_requested) {
        game->is_running = false;
    }    
    if (is_key_pressed(input, KEY_F11)) {
        platform->fullscreen = !platform->fullscreen;
    }
    if (is_key_pressed(input, KEY_F10)) {
        platform->vsync = !platform->vsync;
    }
    
    DEBUG_SWITCH(&platform->vsync, "Vsync");
    DEBUG_SWITCH(&game->game_state.show_grid, "Show grid");
    {DEBUG_VALUE_BLOCK("Debug")
        DEBUG_VALUE(game->debug_state->value_blocks_allocated, "Value blocks allocated");
        DEBUG_VALUE(game->debug_state->debug_open_blocks_allocated, "Blocks allocated");
        DEBUG_VALUE(game->debug_state->debug_values_allocated, "Values allocated");
    }
    {DEBUG_VALUE_BLOCK("Memory")
        DEBUG_VALUE(game->frame_arena.peak_size >> 10, "Frame arena size");
        DEBUG_VALUE(game->debug_state->arena.peak_size >> 10, "Debug arena size");
        DEBUG_VALUE(game->game_state.arena.peak_size >> 10, "Game arena size");
        DEBUG_VALUE(game->renderer.arena.peak_size >> 10, "Renderer arena size");
        DEBUG_VALUE(game->assets->arena.peak_size >> 10, "Assets arena size");
    }
    
    if (memcmp(&game->renderer.settings, &game->game_state.renderer_settings, sizeof(RendererSettings)) != 0) {
        assets_purge_textures(game->assets);
        init_renderer_for_settings(&game->renderer, game->game_state.renderer_settings);
    }
    
    RendererCommands *commands = renderer_begin_frame(&game->renderer, game->game_state.renderer_settings);
    DEBUG_update(game->debug_state, input, commands, game->assets);
    update_and_render(&game->game_state, input, commands, game->assets);
    renderer_end_frame(&game->renderer);
    os_end_frame(game->os);
    DEBUG_frame_end(game->debug_state);
}
