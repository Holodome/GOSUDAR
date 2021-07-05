#include "game.hh"

void game_init(Game *game) {
    game->is_running = true;   
    game->debug_state = DEBUG_init();
    Vec2 win_size;
    game->os = os_init(&win_size);
    renderer_init(&game->renderer, win_size);

    game->assets = assets_init(&game->renderer);
    game_state_init(&game->game_state);
}

void game_cleanup(Game *game) {
}

void game_update_and_render(Game *game) {
    FRAME_MARKER();
    DEBUG_begin_frame(game->debug_state);
    
    Platform *platform = os_begin_frame(game->os);
    InputManager input_ = create_input_manager(platform);
    InputManager *input = &input_;
    if (is_key_pressed(input, KEY_ESCAPE, INPUT_ACCESS_TOKEN_ALL) || platform->is_quit_requested) {
        game->is_running = false;
    }    
    if (is_key_pressed(input, KEY_F11, INPUT_ACCESS_TOKEN_ALL)) {
        platform->fullscreen = !platform->fullscreen;
    }
    if (is_key_pressed(input, KEY_F10, INPUT_ACCESS_TOKEN_ALL)) {
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
        DEBUG_VALUE(game->debug_state->arena.peak_size >> 10, "Debug arena size");
        DEBUG_VALUE(game->game_state.frame_arena.peak_size >> 10, "Frame arena size");
        DEBUG_VALUE(game->game_state.arena.peak_size >> 10, "Game arena size");
        DEBUG_VALUE(game->renderer.arena.peak_size >> 10, "Renderer arena size");
        DEBUG_VALUE(game->assets->arena.peak_size >> 10, "Assets arena size");
    }
    
    RendererSettings renderer_settings = {};
    renderer_settings.display_size = window_size(input);
    RendererCommands *commands = renderer_begin_frame(&game->renderer, renderer_settings, Vec4(0.2));
    update_and_render(&game->game_state, input, commands, game->assets);
    DEBUG_update(game->debug_state, input, commands, game->assets);
    renderer_end_frame(&game->renderer);
    os_end_frame(game->os);
    DEBUG_frame_end(game->debug_state);
}
