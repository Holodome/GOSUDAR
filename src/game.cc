#include "game.hh"

Game *game_init() {
    DebugState *debug_state = DEBUG_init();
    Game *game = bootstrap_alloc_struct(Game, arena);
    DEBUG_ARENA_NAME(&game->arena, "Game");
    game->debug_state = debug_state;
    
    game->is_running = true;   
    
#define COMMAND_MEMORY_SIZE MEGABYTES(4)
#define MAX_VERTEX_COUNT (1 << 16)
#define MAX_INDEX_COUNT (MAX_VERTEX_COUNT / 2 * 3)
    game->renderer_settings.filtered = true;
    game->renderer_settings.mipmapping = true;
    game->renderer_settings.vsync = true;
    game->renderer_settings.sample_count = 4;
    game->renderer_settings.max_vertex_count = MAX_VERTEX_COUNT;
    game->renderer_settings.max_index_count = MAX_INDEX_COUNT;
    game->os = os_init(&game->renderer_settings.display_size);
    game->renderer = renderer_init(game->renderer_settings);
    game->assets = assets_init(game->renderer, &game->frame_arena);
    init_audio_system(&game->audio);
    game->ui = ui_init();
    
    game->commands.command_memory_size = COMMAND_MEMORY_SIZE;
    game->commands.command_memory = (u8 *) alloc(&game->arena, COMMAND_MEMORY_SIZE);
    game->commands.max_vertex_count = MAX_VERTEX_COUNT;
    game->commands.vertices = alloc_arr(&game->arena, MAX_VERTEX_COUNT, Vertex);
    game->commands.max_index_count = MAX_INDEX_COUNT;
    game->commands.indices = alloc_arr(&game->arena, MAX_INDEX_COUNT, RENDERER_INDEX_TYPE);
    
    game->play_state.world_state = world_state_init();
    game->state = STATE_PLAY;
    return game;
}

static void update_game_state(PlayState *play_state, GameLinks links) {
    begin_separated_rendering(links.commands);
    update_and_render_world_state(play_state->world_state, links);
    
    bool blur_game = false;
    if (blur_game) {
        do_blur(links.commands);
    }
    end_separated_rendering(links.commands);
}

static void update_main_menu_state(MainMenuState *main_menu_state, GameLinks links) {
    
}

void game_update_and_render(Game *game) {
    FRAME_MARKER();
    arena_clear(&game->frame_arena);
    // @TODO this is stupid
    alloc(&game->frame_arena, 1);
    DEBUG_ARENA_NAME(&game->frame_arena, "FrameArena");
    DEBUG_begin_frame(game->debug_state);
    
    DEBUG_SWITCH(&game->renderer_settings.mipmapping, "M");
    DEBUG_SWITCH(&game->renderer_settings.filtered, "F");
    
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
        assets_purge_textures(game->assets);
        init_renderer_for_settings(game->renderer, game->renderer_settings);
    }
    game->commands.white_texture = get_white_texture(game->renderer);
    game->commands.command_memory_used = 0;
    game->commands.vertex_count = 0;
    game->commands.index_count = 0;
    game->commands.last_header = 0;
    game->commands.last_setup = 0;
    
    GameLinks links;
    links.assets = game->assets;
    links.platform = platform;
    links.input = &game->input;
    links.audio = &game->audio;
    links.commands = &game->commands;
    links.frame_arena = &game->frame_arena;
    links.ui = game->ui;
    
    switch (game->state) {
        case STATE_MAIN_MENU: {
            update_main_menu_state(&game->main_menu_state, links);
        } break;
        case STATE_PLAY: {
            update_game_state(&game->play_state, links);
        } break;
    }
    DEBUG_update(game->debug_state, links);
    renderer_end_frame(game->renderer, &game->commands);
    
    platform->vsync = game->renderer_settings.vsync;
    update_audio(&game->audio, game->assets, platform);
    os_end_frame(game->os);
    DEBUG_frame_end(game->debug_state);
}
