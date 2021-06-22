#include "game/game.hh"

void game_init(Game *game) {
    logprintln("Game", "Init start");
    game->is_running = true;   
    game->dev_mode = 0;
    
    game->os.init();
    game->os.init_renderer_backend();
    f32 init_start = game->os.get_time();
    size_t renderer_arena_size = MEGABYTES(256);
    arena_init(&game->renderer.arena, os_alloc(renderer_arena_size), renderer_arena_size);
    game->renderer.init();
    
    size_t assets_arena_size = MEGABYTES(256);
    arena_init(&game->assets.arena, os_alloc(assets_arena_size), assets_arena_size);
    // @CLEAN
    game->assets.renderer = &game->renderer;
    game->assets.init();
    game->renderer.white_texture = game->assets.get_tex(Asset_White);
    
    size_t frame_arena_size = MEGABYTES(256);
    arena_init(&game->frame_arena, os_alloc(frame_arena_size), frame_arena_size);
    size_t world_arena_size = MEGABYTES(256);
    arena_init(&game->world.world_arena, os_alloc(world_arena_size), world_arena_size);
    
    game->world.frame_arena = &game->frame_arena;
    world_init(&game->world);
    f32 init_end = game->os.get_time();
    
    size_t dev_ui_arena_size = MEGABYTES(8);
    arena_init(&game->dev_ui.arena, os_alloc(dev_ui_arena_size), dev_ui_arena_size);
    dev_ui_init(&game->dev_ui, &game->assets);
    // Effectively it is not whole init time, but time of game initialization-related routines,
    // cause there is little point in recording time spend on os-related stuff. It should be profiled separately
    // and is (probably) inconsistent due to tf os does 
    logprintln("Game", "Init took %llums", (u64)((init_end - init_start) * 1000));
}

void game_cleanup(Game *game) {
    logprintln("Game", "Cleanup");
    os_free(game->frame_arena.data);
    os_free(game->world.world_arena.data);
    // Mem::free(game->dev_ui.arena.data);
    game->assets.cleanup();
    os_free(game->assets.arena.data);
    game->renderer.cleanup();
    os_free(game->renderer.arena.data);
    game->os.cleanup();
}

void game_update_and_render(Game *game) {
    arena_clear(&game->frame_arena);
    game->os.update_input(&game->input);
    game->input.update();
#define MIN_DT 0.001f
#define MAX_DT 0.1f
    game->input.dt = Math::clamp(game->input.dt, MIN_DT, MAX_DT);
    
    if (game->input.is_quit_requested) {
        game->is_running = false;
    }
    
    game->renderer.begin_frame();
    game->renderer.set_draw_region(game->input.winsize);
    game->renderer.clear(Vec4(0.2));
    
    TempMemory sim_memory = temp_memory_begin(&game->frame_arena);
    SimRegion *sim = begin_sim(&game->frame_arena, &game->world);
    sim->frame_arena = &game->frame_arena;
    do_sim(sim, &game->input, &game->renderer, &game->assets);
    end_sim(sim, &game->input);
    temp_memory_end(sim_memory);

    RenderGroup interface_render_group = render_group_begin(&game->renderer, &game->assets,
        Mat4x4::ortographic_2d(0, game->input.winsize.x, game->input.winsize.y, 0));
    interface_render_group.has_depth = false;
    
    game->dev_ui.mouse_d = game->input.mdelta;
    game->dev_ui.mouse_p = game->input.mpos;
    game->dev_ui.is_mouse_pressed = game->input.is_key_held(Key::MouseLeft);
    DevUILayout dev_ui = dev_ui_begin(&game->dev_ui);
    dev_ui_labelf(&dev_ui, "FPS: %.3f; DT: %ums; D: %llu; E: %llu", 1.0f / game->input.dt, (u32)(game->input.dt * 1000), 
        game->renderer.statistics.draw_call_count, game->world.entity_count);
    Entity *player = get_entity(&game->world, game->world.camera_followed_entity_id);
    dev_ui_labelf(&dev_ui, "P: (%.3f %.3f); Chunk: (%d %d)", player->sim.p.x, player->sim.p.y,
        player->world_pos.chunk.x, player->world_pos.chunk.y);
    
    dev_ui_end(&dev_ui, &interface_render_group);
    game->os.update_window();
}
