#include "game/game_state.hh"
#include "game/game.hh"
#include "framework/renderer.hh"

#include "lib/perlin.hh"

void GameState::init() {
    logprintln("GameState", "Init start");
    
    this->local_dev_ui.init();
    dev_ui->font = assets->get_font("consolas");
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
    logprintln("GameState", "Cleanup");
}

void GameState::update_logic() {
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
    } 
    
    dev_ui->is_enabled = (this->dev_mode & DevMode_DevUI);
    dev_ui->is_focused = (this->dev_mode & DevMode_DevUIFocused);
    
    if (!(this->dev_mode & DevMode_StopSimulation)) {
        // Move player
        for (size_t entity_idx = 0; entity_idx < this->world.entities.len; ++entity_idx) {
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
    if (dev_ui->checkbox("Fullscreen", &this->fullscreen)) {
        game->os.go_fullscreen(this->fullscreen);
    }
    if (dev_ui->button("Close game")) {
        game->is_running = false;
    }
    dev_ui->window_end();
}

void GameState::render() {
    dev_ui->window("World");
    // dev_ui->drag_float3("Player pos", this->player_pos.e);
    dev_ui->value("Selected point", this->world.point_on_plane);
    dev_ui->value("Camera pos", this->world.camera.pos);
    dev_ui->window_end();
    renderer->set_renderering_3d(this->world.camera.mvp);
    // Draw map
    for (size_t y = 0; y < this->world.map_size.y; ++y) {
        for (size_t x = 0; x < this->world.map_size.x; ++x) {
            Vec4 color = Colors::green;
            renderer->imm_draw_quad(Vec3(x, 0, y) * this->world.tile_size, Vec3(x, 0, y + 1) * this->world.tile_size,
                                         Vec3(x + 1, 0, y) * this->world.tile_size, Vec3(x + 1, 0, y + 1) * this->world.tile_size,
                                         color);
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
    
    Array<u32> drawable_entity_ids;
    for (size_t entity_idx = 0; entity_idx < this->world.entities.len; ++entity_idx) {
        Entity *entity = &this->world.entities[entity_idx];
        if (!(entity->flags & EntityFlags_IsDrawable)) {
            continue;
        }
        drawable_entity_ids.add(entity->id);
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
    qsort_s(drawable_entity_ids.data, drawable_entity_ids.len, sizeof(*drawable_entity_ids.data), sort_lambda, &this->world);
    // Sort by distance to camera
    for (size_t drawable_idx = 0; drawable_idx < drawable_entity_ids.len; ++drawable_idx) {
        Entity *entity = &this->world.entities[drawable_entity_ids[drawable_idx]];
        Vec3 billboard[4];
        this->world.get_billboard_positions(World::map_pos_to_world_pos(entity->pos), 0.5f, 0.5f, billboard);
        renderer->imm_draw_quad(billboard, assets->get_tex(entity->texture_name.data));
    }
    dev_ui->end_frame();
}

void GameState::update() {
    dev_ui->begin_frame();
    update_logic();
    // @TODO find way to assert no calls to renderer made before this point...
    renderer->set_draw_region(game->input.winsize);
    renderer->clear(Vec4(0.2));
    render();   
}
