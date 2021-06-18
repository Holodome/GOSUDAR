#include "game/game_state.hh"
#include "game/game.hh"
#include "framework/renderer.hh"

#include "lib/perlin.hh"

void GameState::init() {
    logprintln("GameState", "Init start");
    
    size_t frame_arena_size = MEGABYTES(8);
    this->frame_arena.init(Mem::alloc(frame_arena_size), frame_arena_size);
    size_t world_arena_size = MEGABYTES(8);
    this->world.world_arena.init(Mem::alloc(world_arena_size), world_arena_size);
    this->world.entity_count = 0;
    this->world.max_entity_count = 4096;
    this->world.entities = (Entity *)this->world.world_arena.alloc(sizeof(Entity) * this->world.max_entity_count);
    size_t devui_arena_size = MEGABYTES(8);
    this->local_dev_ui.arena.init(Mem::alloc(devui_arena_size), devui_arena_size);
    this->local_dev_ui.init();
    for (u32 i = 0; i < 50; ++i) {
        f32 r1 = (f32)rand() / RAND_MAX;
        f32 r2 = (f32)rand() / RAND_MAX;
        f32 x = r1 * this->world.tile_size * this->world.map_size.x;
        f32 y = r2 * this->world.tile_size * this->world.map_size.y;
        this->world.add_tree_entity(Vec2(x, y));
    }   
    this->world.add_player_enitity();
    logprintln("GameState", "Init end");
}

void GameState::cleanup() {
    Mem::free(this->frame_arena.data);
    Mem::free(this->world.world_arena.data);
    Mem::free(this->local_dev_ui.arena.data);
    logprintln("GameState", "Cleanup");
}

void GameState::update_logic() {
    bool previous_fullscreen = this->fullscreen;
    if (game->input.is_key_pressed(Key::F1)) {
        this->dev_mode ^= DevMode_StopSimulation;   
    } else if (game->input.is_key_pressed(Key::F2)) {
        this->dev_mode = 0;
    } else if (game->input.is_key_pressed(Key::F3)) {
        this->dev_mode ^= DevMode_DevUI;
    } else if (game->input.is_key_pressed(Key::F4)) {
        if (this->dev_mode & DevMode_DevUI) {
            this->dev_mode ^= DevMode_DevUIFocused;
        }   
    } else if (game->input.is_key_pressed(Key::F5)) {
        this->dev_mode ^= DevMode_FreeCamera;
    } else if (game->input.is_key_pressed(Key::F11)) {
        this->fullscreen = !this->fullscreen;
    } 
    
    dev_ui->is_enabled = (this->dev_mode & DevMode_DevUI);
    dev_ui->is_focused = (this->dev_mode & DevMode_DevUIFocused);
    
    if (!(this->dev_mode & DevMode_StopSimulation) && !(this->dev_mode & DevMode_DevUIFocused)) {
        // Move player
        for (size_t entity_idx = 0; entity_idx < this->world.entity_count; ++entity_idx) {
            Entity *entity = &this->world.entities[entity_idx];
            if (!(entity->flags & EntityFlags_IsUpdatable)) {
                continue;
            }
            
            switch (entity->kind) {
                case EntityKind::Player: {
                    f32 move_coef = 4.0f * game->input.dt;
                    f32 z_speed = 0;
                    if (game->input.is_key_held(Key::W)) {
                        z_speed = move_coef;
                    } else if (game->input.is_key_held(Key::S)) {
                        z_speed = -move_coef;
                    }
                    entity->pos.x += z_speed *  Math::sin(this->world.camera.yaw);
                    entity->pos.y += z_speed * -Math::cos(this->world.camera.yaw);
                    
                    f32 x_speed = 0;
                    if (game->input.is_key_held(Key::D)) {
                        x_speed = move_coef;
                    } else if (game->input.is_key_held(Key::A)) {
                        x_speed = -move_coef;
                    }
                    entity->pos.x += x_speed * Math::cos(this->world.camera.yaw);
                    entity->pos.y += x_speed * Math::sin(this->world.camera.yaw);           
                } break;
            }
        }
        
        this->world.camera.center_pos = this->world.map_pos_to_world_pos(this->world.entities[this->world.player_id].pos);
        this->world.camera.update();
        this->world.camera.recalculate_matrices();
        
        Vec3 ray_dir = this->world.camera.screen_to_world(game->input.mpos);
        f32 t = 0;
        if (ray_intersect_plane(Vec3(0, 1, 0), 0, Ray(this->world.camera.pos, ray_dir), &t) && t > 0) {
            this->world.point_on_plane = world.camera.pos + ray_dir * t;
        } else {
            this->world.point_on_plane = Vec3(0, 1, 0);
        }
    }
    
    dev_ui->window("Debug");
    dev_ui->textf("DevUI focused: %s", (dev_ui->is_focused ? "true" : "false"));
    dev_ui->textf("Draw call count: %llu", renderer->statistics.draw_call_count);
    dev_ui->textf("FPS: %.1f; DT: %.1fms", 1.0f / game->input.dt, game->input.dt * 1000.0f);
    if (dev_ui->button("Close game")) {
        game->is_running = false;
    }
    dev_ui->checkbox("Draw sprite frames", &this->draw_sprite_frames);
    dev_ui->window_end();
    if (previous_fullscreen != this->fullscreen) {
        game->os.go_fullscreen(this->fullscreen);
    }
    
    dev_ui->window("Game");
    dev_ui->value("Wood count", this->wood_count);
    dev_ui->window_end();
}

void GameState::render() {
    renderer->set_renderering_3d(this->world.camera.mvp);
    // Draw map
    for (size_t y = 0; y < this->world.map_size.y; ++y) {
        for (size_t x = 0; x < this->world.map_size.x; ++x) {
            Vec3 tilev[4];
            this->world.get_tile_v(Vec2i(x, y), tilev);
            renderer->imm_draw_quad(tilev, assets->get_tex("grass"));
        }
    }
    if (Math::length(this->world.point_on_plane - Vec3(0, 1, 0)) > 0.001f) {
        i32 x = this->world.point_on_plane.x / this->world.tile_size;
        i32 y = this->world.point_on_plane.z / this->world.tile_size;
        if (0 <= x && x < this->world.map_size.x && 0 <= y && y < this->world.map_size.y) {
            renderer->imm_draw_quad_outline(Vec3(x, 0, y) * this->world.tile_size, Vec3(x, 0, y + 1) * this->world.tile_size,
                                            Vec3(x + 1, 0, y) * this->world.tile_size, Vec3(x + 1, 0, y + 1) * this->world.tile_size,
                                            Colors::black, 0.02f);
        }
    }
    
    size_t max_drawable_count = this->world.entity_count;
    TempMemory zsort = temp_memory_begin(&this->frame_arena);
    size_t drawable_entity_id_count = 0;
    u32 *drawable_entity_ids = (u32 *)this->frame_arena.alloc(max_drawable_count * sizeof(u32));
    for (size_t entity_idx = 0; entity_idx < this->world.entity_count; ++entity_idx) {
        Entity *entity = &this->world.entities[entity_idx];
        if (!(entity->flags & EntityFlags_IsDrawable)) {
            continue;
        }
        drawable_entity_ids[drawable_entity_id_count++] = entity->id;
    }
    
    auto sort_lambda = [](void *ctx, const void *a, const void *b){
        World *world = (World *)ctx;
        EntityId id1 = *((EntityId *)a);
        EntityId id2 = *((EntityId *)b);
        Entity *ae = &world->entities[id1];
        Entity *be = &world->entities[id2];
        int result = 0;
        f32 a_v = Math::dot(world->camera.mvp.get_z(), World::map_pos_to_world_pos(ae->pos) - world->camera.pos);
        f32 b_v = Math::dot(world->camera.mvp.get_z(), World::map_pos_to_world_pos(be->pos) - world->camera.pos);
        result = (int)(a_v < b_v ? -1 : 1);
        return (int)(-result);
    };
    // Sort by distance to camera
    qsort_s(drawable_entity_ids, drawable_entity_id_count, sizeof(u32), sort_lambda, &this->world);
    for (size_t drawable_idx = 0; drawable_idx < drawable_entity_id_count; ++drawable_idx) {
        Entity *entity = &this->world.entities[drawable_entity_ids[drawable_idx]];
        Vec3 billboard[4];
        this->world.get_billboard_positions(World::map_pos_to_world_pos(entity->pos), 0.5f, 0.5f, billboard);
        renderer->imm_draw_quad(billboard, assets->get_tex(entity->texture_name));
        if (this->draw_sprite_frames) {
            renderer->imm_draw_quad_outline(billboard[0], billboard[1], billboard[2], billboard[3], Colors::black, 0.01f);   
        }
    }
    temp_memory_end(zsort);
    dev_ui->end_frame();
}

void GameState::update() {
    this->frame_arena.clear();
    dev_ui->begin_frame();
    update_logic();
    // @TODO find way to assert no calls to renderer made before this point...
    renderer->set_draw_region(game->input.winsize);
    renderer->clear(Vec4(0.2));
    render();   
}
