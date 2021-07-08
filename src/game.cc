#include "game.hh"

void game_init(Game *game) {
    game->is_running = true;   
#define GAME_ARENA_SIZE MEGABYTES(256)
    arena_init(&game->arena, os_alloc(GAME_ARENA_SIZE), GAME_ARENA_SIZE);
#define FRAME_ARENA_SIZE MEGABYTES(256)
    arena_init(&game->frame_arena, os_alloc(FRAME_ARENA_SIZE), FRAME_ARENA_SIZE);
    game->debug_state = DEBUG_init();
    game->renderer_settings.filtered = true;
    game->renderer_settings.mipmapping = true;
    game->renderer_settings.vsync = true;
    game->os = os_init(&game->renderer_settings.display_size);
    renderer_init(&game->renderer, game->renderer_settings);
    game->assets = assets_init(&game->renderer, &game->frame_arena);
    
    game->game_state = GAME_STATE_MAIN_MENU;
    // Main menu
    game->main_menu_start_game_button = create_ui_button(&game->arena, &game->main_menu_interface,
        Rect(550, 100, 100, 50), Vec4(0.4, 0.4, 0.4, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Start game");
    game->main_menu_exit_button = create_ui_button(&game->arena, &game->main_menu_interface,
        Rect(550, 200, 100, 50), Vec4(0.4, 0.4, 0.4, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Exit game");
    // Game state
    game->pause_continue = create_ui_button(&game->arena, &game->pause_interface,
        Rect(550, 100, 100, 50), Vec4(0.4, 0.4, 0.4, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Continue");
    game->pause_main_menu = create_ui_button(&game->arena, &game->pause_interface,
        Rect(550, 200, 100, 50), Vec4(0.4, 0.4, 0.4, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Main menu");
    game->pause_exit = create_ui_button(&game->arena, &game->pause_interface,
        Rect(550, 300, 100, 50), Vec4(0.4, 0.4, 0.4, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Exit");
}

static void update_game_state(Game *game, RendererCommands *commands) {
    if (is_key_pressed(&game->input, KEY_ESCAPE)) {
        game->is_paused = !game->is_paused;
    }
    
    RenderGroup test_render_group = render_group_begin(commands, game->assets, setup_2d(RENDERER_FRAMEBUFFER_GAME_WORLD, 
        Mat4x4::ortographic_2d(0, game->input.platform->display_size.x, game->input.platform->display_size.y, 0)));
    push_rect(&test_render_group, Rect(Vec2(0), game->input.platform->display_size), Vec4(1), Rect(0, 0, 1, 1), 
        assets_get_first_of_type(game->assets, ASSET_TYPE_WORLD_OBJECT));
    push_rect(&test_render_group, Rect(200, 200, 200, 200), Vec4(0, 1, 0, 1));
    push_rect(&test_render_group, Rect(250, 250, 200, 200), Vec4(0, 0.6, 0, 1));
    render_group_end(&test_render_group);
    
    if (game->is_paused) {
        commands->perform_blur = true;
        update_interface(game->pause_interface, &game->input, commands, game->assets);
        if (game->pause_continue->button.is_pressed) {
            game->is_paused = false;
        } 
        if (game->pause_main_menu->button.is_pressed) {
            game->is_paused = false;
            game->game_state = GAME_STATE_MAIN_MENU;
        }
        if (game->pause_exit->button.is_pressed) {
            game->is_running = false;
        }
    }
}

static void update_main_menu_state(Game *game, RendererCommands *commands) {
    update_interface(game->main_menu_interface, &game->input, commands, game->assets);
    if (game->main_menu_start_game_button->button.is_pressed) {
        game->game_state = GAME_STATE_PLAY;
    } 
    if (game->main_menu_exit_button->button.is_pressed) {
        game->is_running = false;
    }
}

void game_update_and_render(Game *game) {
    FRAME_MARKER();
    arena_clear(&game->frame_arena);
    DEBUG_begin_frame(game->debug_state);
    
    {DEBUG_VALUE_BLOCK("Memory")
        DEBUG_VALUE(game->frame_arena.peak_size >> 10, "Frame arena size");
        DEBUG_VALUE(game->debug_state->arena.peak_size >> 10, "Debug arena size");
        DEBUG_VALUE(game->arena.peak_size >> 10, "Game arena size");
        DEBUG_VALUE(game->renderer.arena.peak_size >> 10, "Renderer arena size");
        DEBUG_VALUE(game->assets->arena.peak_size >> 10, "Assets arena size");
    }
    
    Platform *platform = os_begin_frame(game->os);
    game->input = create_input_manager(platform);
    if (platform->is_quit_requested) {
        game->is_running = false;
    }    
    if (is_key_pressed(&game->input, KEY_F11)) {
        platform->fullscreen = !platform->fullscreen;
    }
    if (is_key_pressed(&game->input, KEY_F10)) {
        game->renderer_settings.vsync = !game->renderer_settings.vsync;
    }
    
    game->renderer_settings.display_size = platform->display_size;
    if (memcmp(&game->renderer.settings, &game->renderer_settings, sizeof(RendererSettings)) != 0) {
        assets_purge_textures(game->assets);
        init_renderer_for_settings(&game->renderer, game->renderer_settings);
    }
    
    RendererCommands *commands = renderer_begin_frame(&game->renderer);
    switch (game->game_state) {
        case GAME_STATE_MAIN_MENU: {
            update_main_menu_state(game, commands);
        } break;
        case GAME_STATE_PLAY: {
            update_game_state(game, commands);
        } break;
    }
    DEBUG_update(game->debug_state, &game->input, commands, game->assets);
    renderer_end_frame(&game->renderer);
    
    platform->vsync = game->renderer_settings.vsync;
    os_end_frame(game->os);
    DEBUG_frame_end(game->debug_state);
}
