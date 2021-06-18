#include "game/world.hh"
#include "game/game.hh"

static Entity empty_entity() {
    Entity result;
    result.id = (EntityId)-1;
    result.kind = EntityKind::None;
    result.flags = 0;
    return result;
}

Vec3 World::map_pos_to_world_pos(Vec2 map) {
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
    assert(this->entity_count < this->max_entity_count);
    u32 result = this->entity_count++;
    Entity *dst = &this->entities[result];
    *dst = *src;
    dst->id = result;
    return result;
}

void World::add_tree_entity(Vec2 pos) {
    Entity tree = empty_entity();
    tree.kind = EntityKind::Tree;
    tree.flags = EntityFlags_IsDrawable;
    tree.texture_name = "tree";
    tree.pos = pos;
    tree.chops_left = 3;
    this->add_entity(&tree);
}

void World::add_player_enitity() {
    Entity player = empty_entity();
    player.kind = EntityKind::Player;
    player.flags = EntityFlags_IsDrawable | EntityFlags_IsUpdatable;
    player.pos = Vec2(0, 0);
    player.texture_name = "dude";
    player.health = 100;
    this->player_id = this->add_entity(&player);
}

void World::get_tile_v(Vec2i coord, Vec3 out[4]) {
    i32 x = coord.x;
    i32 y = coord.y;
    out[0] = Vec3(x, 0, y) * this->tile_size;
    out[1] = Vec3(x, 0, y + 1) * this->tile_size;
    out[2] = Vec3(x + 1, 0, y) * this->tile_size;
    out[3] = Vec3(x + 1, 0, y + 1) * this->tile_size;
}