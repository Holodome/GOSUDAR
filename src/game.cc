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
    //
    // Main menu
    //
    create_ui_label(&game->arena, &game->main_menu_interface, Rect(550, 150, 100, 50), Vec4(0.8, 0.2, 0.3, 1.0), "GOSUDAR");
    game->main_menu_start_game_button = create_ui_button_background(&game->arena, &game->main_menu_interface,
        Rect(550, 300, 100, 50), Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Start game", Vec4(0.4, 0.4, 0.4, 1.0));
    game->main_menu_settings_button = create_ui_button_background(&game->arena, &game->main_menu_interface,
        Rect(550, 400, 100, 50), Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Settings", Vec4(0.4, 0.4, 0.4, 1.0));
    game->main_menu_exit_button = create_ui_button_background(&game->arena, &game->main_menu_interface,
        Rect(550, 550, 100, 50), Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Exit game", Vec4(0.4, 0.4, 0.4, 1.0));
    //
    // Settings
    //
    create_ui_label(&game->arena, &game->settings_interface, Rect(550, 50, 100, 50), Vec4(0.8, 0.1, 0.3, 1.0), "Settings");
    game->settings_texture_filtering = create_ui_checkbox(&game->arena, &game->settings_interface,
        Rect(550, 200, 100, 50), Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.0, 1.0), "Bilinear filtering", 
        &game->renderer_settings.filtered);
    game->settings_texture_mipmapping = create_ui_checkbox(&game->arena, &game->settings_interface,
        Rect(550, 300, 100, 50), Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.0, 1.0), "Mipmaps", 
        &game->renderer_settings.mipmapping);
    game->settings_vsync = create_ui_checkbox(&game->arena, &game->settings_interface,
        Rect(550, 400, 100, 50), Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.0, 1.0), "Vsync", 
        &game->renderer_settings.vsync);
    game->settings_back = create_ui_button_background(&game->arena, &game->settings_interface,
        Rect(550, 550, 100, 50), Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Back", Vec4(0.4, 0.4, 0.4, 1.0));
    //
    // Pause
    //
    create_ui_label(&game->arena, &game->pause_interface, Rect(550, 150, 100, 50), Vec4(0.8, 0.2, 0.3, 1.0), "Pause");
    game->pause_continue = create_ui_button_background(&game->arena, &game->pause_interface,
        Rect(550, 300, 100, 50), Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Continue", Vec4(0.4, 0.4, 0.4, 1.0));
    game->pause_main_menu = create_ui_button_background(&game->arena, &game->pause_interface,
        Rect(550, 400, 100, 50), Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Main menu", Vec4(0.4, 0.4, 0.4, 1.0));
    game->pause_exit = create_ui_button_background(&game->arena, &game->pause_interface,
        Rect(550, 500, 100, 50), Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Exit", Vec4(0.4, 0.4, 0.4, 1.0));
    //
    // Game interface
    //
    f32 down_menu_height = 150.0f;
    Rect down_menu_rect = Rect(0, game->renderer_settings.display_size.y - down_menu_height,
        game->renderer_settings.display_size.x, down_menu_height);
    game->game_interface_button_mine_resource = create_ui_button_background(&game->arena, &game->game_interface, 
        Rect(down_menu_rect.p + Vec2(20), Vec2(100, 30)),
        Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Mine", Vec4(0.4, 0.4, 0.4, 1.0));
    game->game_interface_button_ground_interact = create_ui_button_background(&game->arena, &game->game_interface,
         Rect(down_menu_rect.p + Vec2(20, 80), Vec2(100, 30)),
        Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Ground", Vec4(0.4, 0.4, 0.4, 1.0));
    game->game_interface_button_building1 = create_ui_button_background(&game->arena, &game->game_interface, 
        Rect(down_menu_rect.p + Vec2(170, 20), Vec2(100, 30)),
        Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Building1", Vec4(0.4, 0.4, 0.4, 1.0));
    game->game_interface_button_building2 = create_ui_button_background(&game->arena, &game->game_interface,
         Rect(down_menu_rect.p + Vec2(170, 80), Vec2(100, 30)),
        Vec4(0.6, 0.6, 0.6, 1.0), Vec4(0.8, 0.8, 0.8, 1.0), "Building2", Vec4(0.4, 0.4, 0.4, 1.0));
    create_ui_block(&game->arena, &game->game_interface, down_menu_rect, Vec4(0.2, 0.2, 0.2, 1.0));
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
    } else {
        update_interface(game->game_interface, &game->input, commands, game->assets);
    }
}

static void update_main_menu_state(Game *game, RendererCommands *commands) {
    if (game->main_menu_state == MAIN_MENU_MAIN_SCREEN) {
        update_interface(game->main_menu_interface, &game->input, commands, game->assets);
        if (game->main_menu_start_game_button->button.is_pressed) {
            game->game_state = GAME_STATE_PLAY;
        } 
        if (game->main_menu_settings_button->button.is_pressed) {
            game->main_menu_state = MAIN_MENU_SETTINGS;   
        }
        if (game->main_menu_exit_button->button.is_pressed) {
            game->is_running = false;
        }
    } else if (game->main_menu_state == MAIN_MENU_SETTINGS) {
        if (is_key_pressed(&game->input, KEY_ESCAPE)) {
            game->main_menu_state = MAIN_MENU_MAIN_SCREEN;
        }
        update_interface(game->settings_interface, &game->input, commands, game->assets);
        if (game->settings_back->button.is_pressed) {
            game->main_menu_state = MAIN_MENU_MAIN_SCREEN;
        } 
    }
}

void game_update_and_render(Game *game) {
    FRAME_MARKER();
    arena_clear(&game->frame_arena);
    DEBUG_begin_frame(game->debug_state);
    
    DEBUG_SWITCH(&draw_ui_frames, "UI frames");
    {DEBUG_VALUE_BLOCK("Memory")
        DEBUG_VALUE(game->frame_arena.peak_size >> 10, "Frame arena size");
        DEBUG_VALUE(game->debug_state->arena.peak_size >> 10, "Debug arena size");
        DEBUG_VALUE(game->arena.peak_size >> 10, "Game arena size");
        DEBUG_VALUE(game->renderer.arena.peak_size >> 10, "Renderer arena size");
        DEBUG_VALUE(game->assets->arena.peak_size >> 10, "Assets arena size");
    }
    {DEBUG_VALUE_BLOCK("Renderer settings");
        DEBUG_SWITCH(&game->renderer_settings.filtered, "Tex filtration");
        DEBUG_SWITCH(&game->renderer_settings.mipmapping, "Mipmapping");
        DEBUG_SWITCH(&game->renderer_settings.vsync, "Vsync");
        DEBUG_VALUE(game->renderer_settings.display_size, "Display size");
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
