#include "game.hh"

//
// Naive routine to build interface.
// Our interface system should be able to support prototyping now,
// and there is no point in creating system that dynamically determines where to place 
// widgets and does this for different sizes.
// Also it is very naive in its algotirithms - nothing should overlap (when it should???),
// and system for gathering inputs is uncertain. (there are buttons with listeners, checkboxes with pointers
// - no consistency)
// Text rendering system for sure needs more work
// More work need dynamic widgets that could appear if, for example, player selects some entity
//
static void build_interface_for_window_size(Game *game) {
    //
    // This should be loaded from file for current interface mode - we want to support both light and dark
    // also displayed text needs to be loaded from some localization file
    // 
    vec2 text_box_size = Vec2(100, 50);
    f32 text_box_x = (game->renderer_settings.display_size.x - text_box_size.x) * 0.5f;
    vec4 text_box_background = Vec4(0.4, 0.4, 0.4, 1.0);
    vec4 button_color0 = Vec4(0.6, 0.6, 0.6, 1.0);
    vec4 button_color1 = Vec4(0.8, 0.8, 0.8, 1.0);
    vec4 button_color2 = Vec4(0.8, 0.8, 0.0, 1.0);
    vec4 title_color = Vec4(0.8, 0.2, 0.3, 1.0);
    
    arena_clear(&game->interface_arena);
    game->main_menu_interface = 0;
    game->settings_interface = 0;
    game->pause_interface = 0;
    game->game_interface = 0;
    //
    // Main menu
    //
    create_ui_label(&game->interface_arena, &game->main_menu_interface, Rect(text_box_x, 150, 100, 50), title_color, "GOSUDAR");
    UIElement *start_game_button = create_ui_button_background(&game->interface_arena, &game->main_menu_interface,
                                                               Rect(Vec2(text_box_x, 300), text_box_size), button_color0, button_color1, "Start game", text_box_background);
    game->main_menu_start_game_button = get_listener(start_game_button);
    UIElement *settings_button = create_ui_button_background(&game->interface_arena, &game->main_menu_interface,
                                                             Rect(Vec2(text_box_x, 400), text_box_size), button_color0, button_color1, "Settings", text_box_background);
    game->main_menu_settings_button = get_listener(settings_button);
    UIElement *exit_button = create_ui_button_background(&game->interface_arena, &game->main_menu_interface,
                                                         Rect(Vec2(text_box_x, 550), text_box_size), button_color0, button_color1, "Exit game", text_box_background);
    game->main_menu_exit_button = get_listener(exit_button);
    //
    // Settings
    //
    create_ui_label(&game->interface_arena, &game->settings_interface, Rect(Vec2(text_box_x, 50), text_box_size), title_color, "Settings");
    UIElement *texture_filtering = create_ui_checkbox(&game->interface_arena, &game->settings_interface,
                                                      Rect(Vec2(text_box_x, 200), text_box_size), button_color0, button_color2, "Bilinear filtering", 
                                                      &game->renderer_settings.filtered);
    game->settings_texture_filtering = get_listener(texture_filtering);
    UIElement *mipmapping = create_ui_checkbox(&game->interface_arena, &game->settings_interface,
                                               Rect(Vec2(text_box_x, 300), text_box_size), button_color0, button_color2, "Mipmaps", 
                                               &game->renderer_settings.mipmapping);
    game->settings_texture_mipmapping = get_listener(mipmapping);
    UIElement *vsync = create_ui_checkbox(&game->interface_arena, &game->settings_interface,
                                          Rect(Vec2(text_box_x, 400), text_box_size), button_color0, button_color2, "Vsync", 
                                          &game->renderer_settings.vsync);
    game->settings_vsync = get_listener(vsync);
    UIElement *settings_back = create_ui_button_background(&game->interface_arena, &game->settings_interface,
                                                           Rect(Vec2(text_box_x, 550), text_box_size), button_color0, button_color1, "Back", text_box_background);
    game->settings_back = get_listener(settings_back);
    //
    // Pause
    //
    create_ui_label(&game->interface_arena, &game->pause_interface, Rect(Vec2(text_box_x, 150), text_box_size), title_color, "Pause");
    UIElement *pause_continue = create_ui_button_background(&game->interface_arena, &game->pause_interface,
                                                            Rect(Vec2(text_box_x, 300), text_box_size), button_color0, button_color1, "Continue", text_box_background);
    game->pause_continue = get_listener(pause_continue);
    UIElement *pause_main_menu = create_ui_button_background(&game->interface_arena, &game->pause_interface,
                                                             Rect(Vec2(text_box_x, 400), text_box_size), button_color0, button_color1, "Main menu", text_box_background);
    game->pause_main_menu = get_listener(pause_main_menu);
    UIElement *pause_settings = create_ui_button_background(&game->interface_arena, &game->pause_interface,
                                                            Rect(Vec2(text_box_x, 500), text_box_size), button_color0, button_color1, "Settings", text_box_background);
    game->pause_settings = get_listener(pause_settings);
    UIElement *pause_exit = create_ui_button_background(&game->interface_arena, &game->pause_interface,
                                                        Rect(Vec2(text_box_x, 600), text_box_size), button_color0, button_color1, "Exit", text_box_background);
    game->pause_exit = get_listener(pause_exit);
    
    //
    // Game interface
    //
    f32 down_menu_height = 150.0f;
    Rect down_menu_rect = Rect(0, game->renderer_settings.display_size.y - down_menu_height,
                               game->renderer_settings.display_size.x, down_menu_height);
    UIElement *mine_button = create_ui_button_background(&game->interface_arena, &game->game_interface, 
                                                         Rect(down_menu_rect.p + Vec2(20), Vec2(100, 30)),
                                                         button_color0, button_color1, "Mine", text_box_background);
    game->game_interface_button_mine_resource = get_listener(mine_button);
    UIElement *ground_interact = create_ui_button_background(&game->interface_arena, &game->game_interface,
                                                             Rect(down_menu_rect.p + Vec2(20, 80), Vec2(100, 30)),
                                                             button_color0, button_color1, "Ground", text_box_background);
    game->game_interface_button_ground_interact = get_listener(ground_interact);
    UIElement *building1 = create_ui_button_background(&game->interface_arena, &game->game_interface, 
                                                       Rect(down_menu_rect.p + Vec2(170, 20), Vec2(100, 30)),
                                                       button_color0, button_color1, "Building1", text_box_background);
    game->game_interface_button_building1 = get_listener(building1);
    UIElement *building2 = create_ui_button_background(&game->interface_arena, &game->game_interface,
                                                       Rect(down_menu_rect.p + Vec2(170, 80), Vec2(100, 30)),
                                                       button_color0, button_color1, "Building2", text_box_background);
    game->game_interface_button_building2 = get_listener(building2);
    create_ui_block(&game->interface_arena, &game->game_interface, down_menu_rect, Vec4(0.2, 0.2, 0.2, 1.0));
}

void game_init(Game *game) {
    game->is_running = true;   
#define GAME_ARENA_SIZE MEGABYTES(256)
    arena_init(&game->arena, os_alloc(GAME_ARENA_SIZE), GAME_ARENA_SIZE);
#define INTERFACE_ARENA_SIZE MEGABYTES(8)
    arena_init(&game->interface_arena, os_alloc(INTERFACE_ARENA_SIZE), INTERFACE_ARENA_SIZE);
#define FRAME_ARENA_SIZE MEGABYTES(256)
    arena_init(&game->frame_arena, os_alloc(FRAME_ARENA_SIZE), FRAME_ARENA_SIZE);
    game->debug_state = DEBUG_init();
    game->renderer_settings.filtered = true;
    game->renderer_settings.mipmapping = true;
    game->renderer_settings.vsync = true;
    game->renderer_settings.sample_count = 4;
    game->os = os_init(&game->renderer_settings.display_size);
    game->renderer = renderer_init(game->renderer_settings);
    game->assets = assets_init(game->renderer, &game->frame_arena);
    init_audio_system(&game->audio);
    
    world_state_init(&game->world_state, &game->arena, &game->frame_arena);
    game->state = STATE_MAIN_MENU;
    build_interface_for_window_size(game);
}


static void update_game_state(Game *game, GameLinks links) {
    // @TODO think more about separated rendering - here we render interface during it, 
    // should that be the norm?
    begin_separated_rendering(links.commands);
    update_and_render_world_state(&game->world_state, links);
    
    if (game->game_state == GAME_STATE_PAUSED) {
        if (is_key_pressed(&game->input, KEY_ESCAPE)) {
            game->game_state = GAME_STATE_PLAYING;
        }
        
        do_blur(links.commands);
        update_and_render_interface(game->pause_interface, links);
        if (game->pause_continue->is_pressed) {
            game->game_state = GAME_STATE_PLAYING;
        } 
        if (game->pause_main_menu->is_pressed) {
            game->game_state = GAME_STATE_PLAYING;
            game->state = STATE_MAIN_MENU;
        }
        if (game->pause_exit->is_pressed) {
            game->is_running = false;
        }
        if (game->pause_settings->is_pressed) {
            game->game_state = GAME_STATE_SETTINGS;
        }
    } else if (game->game_state == GAME_STATE_SETTINGS) {
        if (is_key_pressed(&game->input, KEY_ESCAPE)) {
            game->game_state = GAME_STATE_PAUSED;
        }
        
        do_blur(links.commands);
        update_and_render_interface(game->settings_interface, links);
        if (game->settings_back->is_pressed) {
            game->game_state = GAME_STATE_PAUSED;
        }
    } else {
        if (is_key_pressed(&game->input, KEY_ESCAPE)) {
            game->game_state = GAME_STATE_PAUSED;
        }
        
        update_and_render_interface(game->game_interface, links);
    }
    end_separated_rendering(links.commands);
}

static void update_main_menu_state(Game *game, GameLinks links) {
    if (game->main_menu_state == MAIN_MENU_MAIN_SCREEN) {
        update_and_render_interface(game->main_menu_interface, links);
        if (game->main_menu_start_game_button->is_pressed) {
            game->state = STATE_PLAY;
        } 
        if (game->main_menu_settings_button->is_pressed) {
            game->main_menu_state = MAIN_MENU_SETTINGS;   
        }
        if (game->main_menu_exit_button->is_pressed) {
            game->is_running = false;
        }
    } else if (game->main_menu_state == MAIN_MENU_SETTINGS) {
        if (is_key_pressed(&game->input, KEY_ESCAPE)) {
            game->main_menu_state = MAIN_MENU_MAIN_SCREEN;
        }
        update_and_render_interface(game->settings_interface, links);
        if (game->settings_back->is_pressed) {
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
        // DEBUG_VALUE(game->renderer.arena.peak_size >> 10, "Renderer arena size");
        DEBUG_VALUE(game->assets->arena.peak_size >> 10, "Assets arena size");
    }
    
    Platform *platform = os_begin_frame(game->os);
    game->input = create_input_manager(platform);
    
    if (platform->is_quit_requested || (is_key_held(&game->input, KEY_ALT) && is_key_pressed(&game->input, KEY_F4))) {
        game->is_running = false;
    }     
    if (is_key_pressed(&game->input, KEY_F11)) {
        platform->fullscreen = !platform->fullscreen;
    }
    if (is_key_pressed(&game->input, KEY_F10)) {
        game->renderer_settings.vsync = !game->renderer_settings.vsync;
    }
    
    game->renderer_settings.display_size = platform->display_size;
    if (memcmp(get_current_settings(game->renderer), &game->renderer_settings, sizeof(RendererSettings)) != 0) {
        build_interface_for_window_size(game);
        assets_purge_textures(game->assets);
        init_renderer_for_settings(game->renderer, game->renderer_settings);
    }
    
    RendererCommands *commands = renderer_begin_frame(game->renderer);
    
    GameLinks links;
    links.commands = commands;
    links.assets = game->assets;
    links.platform = platform;
    links.input = &game->input;
    links.audio = &game->audio;
    
    switch (game->state) {
        case STATE_MAIN_MENU: {
            update_main_menu_state(game, links);
        } break;
        case STATE_PLAY: {
            update_game_state(game, links);
        } break;
    }
    DEBUG_update(game->debug_state, &game->input, commands, game->assets);
    renderer_end_frame(game->renderer);
    
    platform->vsync = game->renderer_settings.vsync;
    update_audio(&game->audio, game->assets, platform);
    os_end_frame(game->os);
    DEBUG_frame_end(game->debug_state);
}
