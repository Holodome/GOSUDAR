#include "game/game.hh"

void game_init(Game *game) {
    logprintln("Game", "Init start");
    game->is_running = true;   
    game->dev_mode = 0;
    
    game->os.init();
    game->os.init_renderer_backend();
    f32 init_start = game->os.get_time();
    size_t renderer_arena_size = MEGABYTES(256);
    arena_init(&game->renderer.arena, Mem::alloc(renderer_arena_size), renderer_arena_size);
    game->renderer.init();
    
    size_t assets_arena_size = MEGABYTES(256);
    arena_init(&game->assets.arena, Mem::alloc(assets_arena_size), assets_arena_size);
    // @CLEAN
    game->assets.renderer = &game->renderer;
    game->assets.init();
    game->renderer.white_texture = game->assets.get_tex(Asset_White);
    
    size_t frame_arena_size = MEGABYTES(8);
    arena_init(&game->frame_arena, Mem::alloc(frame_arena_size), frame_arena_size);
    size_t world_arena_size = MEGABYTES(8);
    arena_init(&game->world.world_arena, Mem::alloc(world_arena_size), world_arena_size);
    
    game->world.frame_arena = &game->frame_arena;
    world_init(&game->world);
    f32 init_end = game->os.get_time();
    // Effectively it is not whole init time, but time of game initialization-related routines,
    // cause there is little point in recording time spend on os-related stuff. It should be profiled separately
    // and is (probably) inconsistent due to tf os does 
    logprintln("Game", "Init took %llums", (u64)((init_end - init_start) * 1000));
}

void game_cleanup(Game *game) {
    logprintln("Game", "Cleanup");
    Mem::free(game->frame_arena.data);
    Mem::free(game->world.world_arena.data);
    // Mem::free(game->dev_ui.arena.data);
    game->assets.cleanup();
    Mem::free(game->assets.arena.data);
    game->renderer.cleanup();
    Mem::free(game->renderer.arena.data);
    game->os.cleanup();
}

void game_update_and_render(Game *game) {
    arena_clear(&game->frame_arena);
    game->os.update_input(&game->input);
    game->input.update();
    
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
    
    f32 interface_tile_size = 30.0f;
    imm_draw_rect(&interface_render_group, Rect((game->input.winsize.x) * 0.5f - interface_tile_size, game->input.winsize.y - interface_tile_size,
                                                         interface_tile_size * 2.0f, interface_tile_size), Vec4(0.4f, 0.4f, 0.4f, 0.8f));
    imm_draw_rect(&interface_render_group, Rect((game->input.winsize.x) * 0.5f - interface_tile_size, game->input.winsize.y - interface_tile_size,
                                                         interface_tile_size, interface_tile_size), Colors::white, Rect(0, 0, 1, 1),
                                                         Asset_WoodIcon);
    // char wood_count_display[32];
    // snprintf(wood_count_display, sizeof(wood_count_display), "%u", game->world.wood_count);
    // imm_draw_text(&interface_render_group, Vec2(game->input.winsize.x * 0.5f, game->input.winsize.y - interface_tile_size), Colors::black, wood_count_display,
    //                        Asset_Font, 1.0f);
    // render_group_end(&interface_render_group);
    
    // renderer.render();
    game->os.update_window();
}
