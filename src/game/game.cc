#include "game/game.hh"

void game_init(Game *game) {
    logprintln("Game", "Init start");
    game->is_running = true;   
    
    game->os.init();
    game->os.init_renderer_backend();
    f32 init_start = game->os.get_time();
    size_t renderer_arena_size = MEGABYTES(256);
    arena_init(&game->renderer.arena, os_alloc(renderer_arena_size), renderer_arena_size);
    renderer_init(&game->renderer);
    
    size_t assets_arena_size = MEGABYTES(256);
    arena_init(&game->assets.arena, os_alloc(assets_arena_size), assets_arena_size);
    // @TODO
    game->assets.renderer = &game->renderer;
    game->assets.init();
    
    size_t frame_arena_size = MEGABYTES(256);
    arena_init(&game->game_state.frame_arena, os_alloc(frame_arena_size), frame_arena_size);
    size_t world_arena_size = MEGABYTES(256);
    arena_init(&game->game_state.arena, os_alloc(world_arena_size), world_arena_size);
    game_state_init(&game->game_state);
    f32 init_end = game->os.get_time();
    
    size_t dev_ui_arena_size = MEGABYTES(8);
    arena_init(&game->dev_ui.arena, os_alloc(dev_ui_arena_size), dev_ui_arena_size);
    game->dev_ui.assets = &game->assets;
    dev_ui_init(&game->dev_ui, &game->assets);
    logprintln("Game", "Init took %llums", (u64)((init_end - init_start) * 1000));
}

void game_cleanup(Game *game) {
    logprintln("Game", "Cleanup");
    os_free(game->game_state.frame_arena.data);
    os_free(game->game_state.arena.data);
    // Mem::free(game->dev_ui.arena.data);
    game->assets.cleanup();
    os_free(game->assets.arena.data);
    renderer_cleanup(&game->renderer);
    os_free(game->renderer.arena.data);
    game->os.cleanup();
}

void game_update_and_render(Game *game) {
    game->os.update_input(&game->input);
#define MIN_DT 0.001f
#define MAX_DT 0.1f
    game->input.dt = Math::clamp(game->input.dt, MIN_DT, MAX_DT);
    if (game->input.is_key_pressed(Key::Escape)) {
        game->is_running = false;
    }    
    if (game->input.is_quit_requested) {
        game->is_running = false;
    }
    
    RendererCommands *commands = renderer_begin_frame(&game->renderer, game->input.winsize, Vec4(0.2));
    update_and_render(&game->game_state, &game->input, commands, &game->assets);
    RenderGroup interface_render_group = render_group_begin(commands, &game->assets,
        setup_2d(Mat4x4::ortographic_2d(0, game->input.winsize.x, game->input.winsize.y, 0)));
    
    game->dev_ui.mouse_d = game->input.mdelta;
    game->dev_ui.mouse_p = game->input.mpos;
    game->dev_ui.is_mouse_pressed = game->input.is_key_held(Key::MouseLeft);
    DevUILayout dev_ui = dev_ui_begin(&game->dev_ui);
    dev_ui_labelf(&dev_ui, "FPS: %.3f; DT: %ums; D: %llu; E: %llu; S: %llu", 1.0f / game->input.dt, (u32)(game->input.dt * 1000), 
        game->renderer.statistics.draw_call_count, game->game_state.world->entity_count,
        game->game_state.DEBUG_last_frame_sim_region_entity_count);
    Entity *player = get_world_entity(game->game_state.world, game->game_state.camera_followed_entity_id);
    Vec2 player_pos = DEBUG_world_pos_to_p(player->world_pos);
    dev_ui_labelf(&dev_ui, "P: (%.2f %.2f); O: (%.3f %.3f); Chunk: (%d %d)", 
        player_pos.x, player_pos.y,
        player->world_pos.offset.x, player->world_pos.offset.y,
        player->world_pos.chunk.x, player->world_pos.chunk.y);
    dev_ui_labelf(&dev_ui, "Wood: %u; Gold: %u", game->game_state.wood_count, game->game_state.gold_count);    
    dev_ui_labelf(&dev_ui, "Building mode: %s", game->game_state.is_in_building_mode ? "true" : "false");
    if (!is_same(game->game_state.interactable, null_id())) {
        SimEntity *interactable = &get_world_entity(game->game_state.world, game->game_state.interactable)->sim;
        if (interactable->world_object_flags & WORLD_OBJECT_FLAG_IS_BUILDING) {
            dev_ui_labelf(&dev_ui, "Building build progress: %.2f", interactable->build_progress);
        }
    }
    if (game->game_state.interaction_kind) {
        dev_ui_labelf(&dev_ui, "I: %u%%", (i32)(game->game_state.interaction_current_time / game->game_state.interaction_time * 100));    
    }
    dev_ui_end(&dev_ui, &interface_render_group);
    renderer_end_frame(&game->renderer);
    game->os.update_window();
}
