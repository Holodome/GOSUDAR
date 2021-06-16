#include "game/world.hh"
#include "game/game.hh"

static Vec3 map_pos_to_world_pos(Vec2 map) {
    return Vec3(map.x, 0, map.y);
}

static void get_billboard_positions(Vec3 mid_bottom, Vec3 right, Vec3 up, f32 width, f32 height, Vec3 out[4]) {
    Vec3 top_left = mid_bottom - right * width * 0.5f + up * height;
    Vec3 bottom_left = top_left - up * height;
    Vec3 top_right = top_left + right * width;
    Vec3 bottom_right = top_right - up * height;
    out[0] = top_left;
    out[1] = bottom_left;
    out[2] = top_right;
    out[3] = bottom_right;
}

void World::get_billboard_positions(Vec3 mid_bottom, f32 width, f32 height, Vec3 out[4]) {
    ::get_billboard_positions(mid_bottom, this->camera.mvp.get_x(), this->camera.mvp.get_y(), width, height, out);
}

u32 World::add_entity(const Entity *src) {
    u32 result = this->entities.add({});
    Entity *dst = &this->entities[result];
    *dst = *src;
    dst->id = result;
    return result;
}

void World::add_tree_entity(Vec2 pos) {
    Entity tree = {};
    tree.kind = EntityKind::Tree;
    tree.flags = EntityFlags_IsDrawable;
    tree.texture_name = "tree";
    tree.pos = pos;
    this->add_entity(&tree);
}

void World::add_player_enitity() {
    Entity player = {};
    player.kind = EntityKind::Player;
    player.flags = EntityFlags_IsDrawable | EntityFlags_IsUpdatable;
    player.pos = Vec2(0, 0);
    player.texture_name = "dude";
    this->player_entity_id = this->add_entity(&player);
}

void World::init() {
    logprintln("World", "init");
    for (u32 i = 0; i < 50; ++i) {
        f32 r1 = (f32)rand() / RAND_MAX;
        f32 r2 = (f32)rand() / RAND_MAX;
        f32 x = r1 * this->tile_size * this->map_size.x;
        f32 y = r2 * this->tile_size * this->map_size.y;
        this->add_tree_entity(Vec2(x, y));
    }   
    this->add_player_enitity();
}

void World::cleanup() {
    
}

void World::update() {
    TIMED_FUNCTION;
    // Move player
    for (size_t entity_idx = 0; entity_idx < this->entities.len; ++entity_idx) {
        Entity *entity = &this->entities[entity_idx];
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
                entity->pos.x += z_speed *  Math::sin(this->camera.yaw);
                entity->pos.y += z_speed * -Math::cos(this->camera.yaw);
                
                f32 x_speed = 0;
                if (game->input.is_key_held(Key::D)) {
                    x_speed = move_coef;
                } else if (game->input.is_key_held(Key::A)) {
                    x_speed = -move_coef;
                }
                entity->pos.x += x_speed * Math::cos(this->camera.yaw);
                entity->pos.y += x_speed * Math::sin(this->camera.yaw);           
            } break;
        }
    }
    
    this->camera.center_pos = map_pos_to_world_pos(this->entities[this->player_entity_id].pos);
    this->camera.update();
    this->camera.recalculate_matrices();
    
    Vec3 ray_dir = this->camera.screen_to_world(game->input.mpos);
    f32 t = 0;
    if (ray_intersect_plane(Vec3(0, 1, 0), 0, Ray(camera.pos, ray_dir), &t) && t > 0) {
        this->point_on_plane = camera.pos + ray_dir * t;
    } else {
        this->point_on_plane = Vec3(0, 1, 0);
    }
 
}

void World::render() {
    TIMED_FUNCTION;
    dev_ui->window("World");
    // dev_ui->drag_float3("Player pos", this->player_pos.e);
    dev_ui->value("Selected point", this->point_on_plane);
    dev_ui->value("Camera pos", this->camera.pos);
    dev_ui->window_end();
    renderer->set_renderering_3d(camera.mvp);
    // Draw map
    for (size_t y = 0; y < map_size.y; ++y) {
        for (size_t x = 0; x < map_size.x; ++x) {
            Vec4 color = Colors::green;
            renderer->imm_draw_quad(Vec3(x, 0, y) * tile_size, Vec3(x, 0, y + 1) * tile_size,
                                         Vec3(x + 1, 0, y) * tile_size, Vec3(x + 1, 0, y + 1) * tile_size,
                                         color);
        }
    }
    if (Math::length(this->point_on_plane - Vec3(0, 1, 0)) > 0.001f) {
        i32 x = this->point_on_plane.x / tile_size;
        i32 y = this->point_on_plane.z / tile_size;
        if (0 <= x && x < map_size.x && 0 <= y && y < map_size.y) {
            renderer->imm_draw_quad_outline(Vec3(x, 0, y) * tile_size, Vec3(x, 0, y + 1) * tile_size,
                                            Vec3(x + 1, 0, y) * tile_size, Vec3(x + 1, 0, y + 1) * tile_size,
                                            Colors::black, 0.02f);
        }
    }
    
    Array<u32> drawable_entity_ids;
    for (size_t entity_idx = 0; entity_idx < this->entities.len; ++entity_idx) {
        Entity *entity = &this->entities[entity_idx];
        if (!(entity->flags & EntityFlags_IsDrawable)) {
            continue;
        }
        drawable_entity_ids.add(entity->id);
    }
    // Sort by distance to camera
    for (size_t drawable_idx = 0; drawable_idx < drawable_entity_ids.len; ++drawable_idx) {
        Entity *entity = &this->entities[drawable_entity_ids[drawable_idx]];
        Vec3 billboard[4];
        this->get_billboard_positions(map_pos_to_world_pos(entity->pos), 0.5f, 0.5f, billboard);
        renderer->imm_draw_quad(billboard, assets->get_tex(entity->texture_name.data));
    }
}