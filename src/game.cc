#include "game.hh"


static bool is_in_same_cell(Vec2 a, Vec2 b) {
    Vec2 a_floor = floor_to_cell(a);
    Vec2 b_floor = floor_to_cell(b);
    return a_floor == b_floor;
}

static bool is_cell_occupied(SimRegion *sim, Vec2 p) {
    TIMED_FUNCTION();
    bool occupied = false;
    
    for (EntityIterator iter = iterate_all_entities(sim);
         is_valid(&iter);
         advance(&iter)) {
        SimEntity *entity = iter.ptr;
        if (entity->kind & ENTITY_KIND_WORLD_OBJECT) {
            if (is_in_same_cell(entity->p, p)) {
                occupied = true;
                break;
            }
        }        
    }
    return occupied;
}

static EntityID add_player(SimRegion *sim) {
    SimEntity *entity = create_entity(sim);
    entity->p = {};
    entity->kind = ENTITY_KIND_PLAYER;
    return entity->id;
}

static EntityID add_tree(SimRegion *sim, Vec2 pos) {
    pos = floor_to_cell(pos);
    
    SimEntity *entity = create_entity(sim);
    entity->p = pos;
    entity->kind = ENTITY_KIND_WORLD_OBJECT;
    entity->world_object_kind = (rand() % 3 + WORLD_OBJECT_KIND_TREE_FOREST);
    entity->resource_interactions_left = 1;
    return entity->id;
}

static EntityID add_gold_vein(SimRegion *sim, Vec2 pos) {
    pos = floor_to_cell(pos);
    
    SimEntity *entity = create_entity(sim);
    entity->p = pos;
    entity->kind = ENTITY_KIND_WORLD_OBJECT;
    entity->world_object_kind = WORLD_OBJECT_KIND_GOLD_DEPOSIT;
    entity->resource_interactions_left = 5;
    return entity->id;
}

static void world_gen(Game *game) {
    Entropy entropy;
    entropy.state = 12345678;
    SimRegion *gen_sim = begin_sim(&game->frame_arena, game->world, Vec2i(0), Vec2i(0));
    // Initialize game_state 
    game->camera_followed_entity = add_player(gen_sim);
    for (size_t i = 0; i < 1000; ++i) {
        do {
            Vec2 p;
            p.x = random_bilateral(&entropy) * CHUNK_SIZE * 5;
            p.y = random_bilateral(&entropy) * CHUNK_SIZE * 5;
            if (!is_cell_occupied(gen_sim, p)) {
                add_tree(gen_sim, p);
                break;
            }
        } while (true);
    }
    
    for (size_t i = 0; i < 50; ++i) {
        do {
            Vec2 p;
            p.x = random_bilateral(&entropy) * CHUNK_SIZE * 5;
            p.y = random_bilateral(&entropy) * CHUNK_SIZE * 5;
            if (!is_cell_occupied(gen_sim, p)) {
                add_gold_vein(gen_sim, p);
                break;
            }
        } while (0);
    }
    end_sim(gen_sim);
}

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
    Vec2 text_box_size = Vec2(100, 50);
    f32 text_box_x = (game->renderer_settings.display_size.x - text_box_size.x) * 0.5f;
    Vec4 text_box_background = Vec4(0.4, 0.4, 0.4, 1.0);
    Vec4 button_color0 = Vec4(0.6, 0.6, 0.6, 1.0);
    Vec4 button_color1 = Vec4(0.8, 0.8, 0.8, 1.0);
    Vec4 button_color2 = Vec4(0.8, 0.8, 0.0, 1.0);
    Vec4 title_color = Vec4(0.8, 0.2, 0.3, 1.0);
    
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
    UIElement *pause_exit = create_ui_button_background(&game->interface_arena, &game->pause_interface,
        Rect(Vec2(text_box_x, 500), text_box_size), button_color0, button_color1, "Exit", text_box_background);
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
    game->os = os_init(&game->renderer_settings.display_size);
    renderer_init(&game->renderer, game->renderer_settings);
    game->assets = assets_init(&game->renderer, &game->frame_arena);
    
    game->world = alloc_struct(&game->arena, World);
    game->world->world_arena = &game->arena;
    // Initialize world
    world_init(game->world);
    world_gen(game);
    game->game_state = GAME_STATE_MAIN_MENU;
    build_interface_for_window_size(game);
}

static void update_game(Game *game, SimRegion *sim) {
    f32 x_view_coef = 1.0f * game->input.platform->frame_dt;
    f32 y_view_coef = 0.6f * game->input.platform->frame_dt;
    f32 x_angle_change = game->input.platform->mdelta.x * x_view_coef;
    f32 y_angle_change = game->input.platform->mdelta.y * y_view_coef;
    game->cam.yaw += x_angle_change;
    game->cam.yaw = unwind_rad(game->cam.yaw);
    game->cam.pitch += y_angle_change;
#define MIN_CAM_PITCH (HALF_PI * 0.1f)
#define MAX_CAM_PITCH (HALF_PI * 0.9f)
    game->cam.pitch = clamp(game->cam.pitch, MIN_CAM_PITCH, MAX_CAM_PITCH);
    f32 delta_zoom = game->input.platform->mwheel;
    // printf("%f\n", delta_zoom);
    game->cam.distance_from_player -= delta_zoom;
    game->cam.distance_from_player = clamp(game->cam.distance_from_player, 0.5f, 10);
    
    SimEntity *camera_followed_entity = get_entity_by_id(sim, game->camera_followed_entity);
    // Calculate player movement
    Vec2 player_delta = Vec2(0);
    f32 move_coef = 4.0f * game->input.platform->frame_dt;
    f32 z_speed = 0;
    if (is_key_held(&game->input, KEY_W)) {
        z_speed = move_coef;
    } else if (is_key_held(&game->input, KEY_S)) {
        z_speed = -move_coef;
    }
    player_delta.x += z_speed *  sinf(game->cam.yaw);
    player_delta.y += z_speed * -cosf(game->cam.yaw);
    
    f32 x_speed = 0;
    if (is_key_held(&game->input, KEY_D)) {
        x_speed = move_coef;
    } else if (is_key_held(&game->input, KEY_A)) {
        x_speed = -move_coef;
    }
    player_delta.x += x_speed * cosf(game->cam.yaw);
    player_delta.y += x_speed * sinf(game->cam.yaw);     
    camera_followed_entity->p += player_delta;
}

static void get_billboard_positions(Vec3 mstorage_index_bottom, Vec3 right, Vec3 up, f32 width, f32 height, Vec3 out[4]) {
    Vec3 top_left = mstorage_index_bottom - right * width * 0.5f + up * height;
    Vec3 bottom_left = top_left - up * height;
    Vec3 top_right = top_left + right * width;
    Vec3 bottom_right = top_right - up * height;
    out[0] = top_left;
    out[1] = bottom_left;
    out[2] = top_right;
    out[3] = bottom_right;
}

static void get_ground_tile_positions(Vec2i tile_pos, Vec3 out[4]) {
    f32 x = (f32)tile_pos.x;
    f32 y = (f32)tile_pos.y;
    out[0] = Vec3(x, 0, y) * TILE_SIZE;
    out[1] = Vec3(x, 0, y + 1) * TILE_SIZE;
    out[2] = Vec3(x + 1, 0, y) * TILE_SIZE;
    out[3] = Vec3(x + 1, 0, y + 1) * TILE_SIZE;
}

static void get_sprite_settings_for_entity(Assets *assets, SimEntity *entity, AssetID *id_dest, Vec2 *size_dest) {
    AssetID texture_id = INVALID_ASSET_ID;
    Vec2 size = Vec2(0, 0);
    switch(entity->kind) {
        case ENTITY_KIND_PLAYER: {
            texture_id = assets_get_first_of_type(assets, ASSET_TYPE_PLAYER);
            size = Vec2(0.5f);
        } break;
        case ENTITY_KIND_WORLD_OBJECT: {
            size = Vec2(0.5f);
            AssetTagList tags = {};
            AssetTagList weights = {};
            tags.tags[ASSET_TAG_WORLD_OBJECT_KIND] = entity->world_object_kind;
            weights.tags[ASSET_TAG_WORLD_OBJECT_KIND] = 1000.0f;
            tags.tags[ASSET_TAG_BUILDING_IS_BUILT] = (bool)(entity->world_object_flags & WORLD_OBJECT_FLAG_IS_BUILT);
            weights.tags[ASSET_TAG_BUILDING_IS_BUILT] = 1.0f;
            texture_id = assets_get_closest_match(assets, ASSET_TYPE_WORLD_OBJECT, &weights, &tags);
        } break;
        INVALID_DEFAULT_CASE;
    }
        
    *id_dest = texture_id;
    *size_dest = size;
}

static void render_game(Game *game, SimRegion *sim, RendererCommands *commands) {
    TIMED_FUNCTION();    
    SimEntity *camera_followed_entity = get_entity_by_id(sim, game->camera_followed_entity);
    Vec3 center_pos = xz(camera_followed_entity->p);
    f32 horiz_distance = game->cam.distance_from_player * cosf(game->cam.pitch);
    f32 vert_distance = game->cam.distance_from_player * sinf(game->cam.pitch);
    f32 offsetx = horiz_distance * sinf(-game->cam.yaw);
    f32 offsetz = horiz_distance * cosf(-game->cam.yaw);
    sim->cam_p.x = offsetx + center_pos.x;
    sim->cam_p.z = offsetz + center_pos.z;
    sim->cam_p.y = vert_distance;
    
#define CAMERA_FOV rad(60)
#define CAMERA_NEAR_PLANE 0.001f
#define CAMERA_FAR_PLANE  100.0f
    game->projection = Mat4x4::perspective(CAMERA_FOV, game->input.platform->display_size.aspect_ratio(), CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);
    game->view = Mat4x4::identity() * Mat4x4::rotation(game->cam.pitch, Vec3(1, 0, 0)) * Mat4x4::rotation(game->cam.yaw, Vec3(0, 1, 0))
        * Mat4x4::translate(-sim->cam_p);
    sim->cam_mvp = game->projection * game->view;
    RenderGroup world_render_group = render_group_begin(commands, game->assets,
        setup_3d(RENDERER_FRAMEBUFFER_GAME_WORLD, game->view, game->projection));
    // Draw ground
    BEGIN_BLOCK("Ground");
    AssetID ground_tex_id = assets_get_first_of_type(game->assets, ASSET_TYPE_GRASS);
    for (i32 chunk_x = sim->min_chunk.x; chunk_x <= sim->max_chunk.x; ++chunk_x) {
        for (i32 chunk_y = sim->min_chunk.y; chunk_y <= sim->max_chunk.y; ++chunk_y) {
            Vec2i chunk_pos_in_tiles = (Vec2i(chunk_x, chunk_y) - sim->min_chunk) * TILES_IN_CHUNK;
            for (i32 tile_x = 0; tile_x < TILES_IN_CHUNK; ++tile_x) {
                for (i32 tile_y = 0; tile_y < TILES_IN_CHUNK; ++tile_y) {
                    Vec2i tile_pos = chunk_pos_in_tiles + Vec2i(tile_x, tile_y);
                    Vec3 tile_v[4];
                    get_ground_tile_positions(tile_pos, tile_v);
                    push_quad(&world_render_group, tile_v, ground_tex_id);
                }
            }
        }
    }
    END_BLOCK();
    // Collecting entities for drawing is better done after updating in case some of them are deleted...
    size_t max_drawable_count = sim->entity_count;
    SortEntry *sort_a = alloc_arr(&game->frame_arena, max_drawable_count, SortEntry);
    SortEntry *sort_b = alloc_arr(&game->frame_arena, max_drawable_count, SortEntry);
    size_t drawable_count = 0;
    for (EntityIterator iter = iterate_all_entities(sim);
         is_valid(&iter);
         advance(&iter), ++drawable_count) {
        SimEntity *ent = iter.ptr;
        sort_a[drawable_count].sort_key = dot(sim->cam_mvp.get_z(), xz(ent->p) - sim->cam_p);
        sort_a[drawable_count].sort_index = iter.idx;
    }
    radix_sort(sort_a, sort_b, drawable_count);
    for (size_t drawable_idx = 0; drawable_idx < drawable_count; ++drawable_idx) {
        SimEntity *entity = &sim->entities[sort_a[drawable_count - drawable_idx - 1].sort_index];
        AssetID texture_id;
        Vec2 size;
        get_sprite_settings_for_entity(game->assets, entity, &texture_id, &size);
        Vec3 billboard[4];
        Vec3 pos = Vec3(entity->p.x, 0, entity->p.y);
        Vec4 color = WHITE;
        if (entity->kind == ENTITY_KIND_WORLD_OBJECT && entity->world_object_flags & WORLD_OBJECT_FLAG_IS_BLUEPRINT) {
            color.a = 0.5f;
        }
        get_billboard_positions(pos, sim->cam_mvp.get_x(), sim->cam_mvp.get_y(), size.x, size.y, billboard);
        push_quad(&world_render_group, billboard[0], billboard[1], billboard[2], billboard[3], color, texture_id);
    }
    
    render_group_end(&world_render_group); 
}

static void update_game_state(Game *game, RendererCommands *commands) {
    if (is_key_pressed(&game->input, KEY_ESCAPE)) {
        game->is_paused = !game->is_paused;
    }
    
    SimRegion *sim = begin_sim(&game->frame_arena, game->world, Vec2i(-5), Vec2i(4));
    if (!game->is_paused) {
        update_game(game, sim);
    }
    render_game(game, sim, commands);
    end_sim(sim);
    
    if (game->is_paused) {
        commands->perform_blur = true;
        update_interface(game->pause_interface, &game->input, commands, game->assets);
        if (game->pause_continue->is_pressed) {
            game->is_paused = false;
        } 
        if (game->pause_main_menu->is_pressed) {
            game->is_paused = false;
            game->game_state = GAME_STATE_MAIN_MENU;
        }
        if (game->pause_exit->is_pressed) {
            game->is_running = false;
        }
    } else {
        update_interface(game->game_interface, &game->input, commands, game->assets);
    }
}

static void update_main_menu_state(Game *game, RendererCommands *commands) {
    if (game->main_menu_state == MAIN_MENU_MAIN_SCREEN) {
        update_interface(game->main_menu_interface, &game->input, commands, game->assets);
        if (game->main_menu_start_game_button->is_pressed) {
            game->game_state = GAME_STATE_PLAY;
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
        update_interface(game->settings_interface, &game->input, commands, game->assets);
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
    if (memcmp(&game->renderer.settings, &game->renderer_settings, sizeof(RendererSettings)) != 0) {
        if (game->renderer.settings.display_size != game->renderer_settings.display_size) {
            build_interface_for_window_size(game);
        }
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
