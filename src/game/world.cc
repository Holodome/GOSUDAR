#include "game/world.hh"
#include "game/game.hh"

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
    this->player_id = this->add_entity(&player);
}