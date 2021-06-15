#include "game/world.hh"
#include "game/game.hh"

void World::init() {
    
}

void World::cleanup() {
    
}

void World::update() {
    this->camera.update_input();
    
    this->camera.recalculate_matrices();
    Vec3 ray_dir = this->camera.screen_to_world(game->input.mpos);
    f32 t = 0;
    if (ray_intersect_plane(Vec3(0, 1, 0), 0, Ray(camera.pos, ray_dir), &t) && t > 0) {
        this->point_on_plane = camera.pos + ray_dir * t;
    } else {
        this->point_on_plane = Vec3(0, 1, 0);
    }
    dev_ui->value("Selected point", this->point_on_plane);
    dev_ui->window_end();
    this->player_pos = this->camera.pos;
}

void World::render() {
    renderer->set_renderering_3d(camera.mvp);
    // Draw map
    Vec2i map_size = Vec2i(10, 10);
    f32 tile_size = 2.0f;
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
    
    Vec3 right = this->camera.mvp.get_x();
    Vec3 up = this->camera.mvp.get_y();
    f32 width = 0.5f;
    f32 height = 0.5f;
    Vec3 mid_bottom = this->point_on_plane;
    Vec3 top_left = mid_bottom - right * width * 0.5f + up * height;
    Vec3 top_right = top_left + right * width;
    Vec3 bottom_left = top_left - up * height;
    Vec3 bottom_right = top_right - up * height;
    renderer->imm_draw_quad(top_left, bottom_left, top_right, bottom_right, Colors::white, game->tex_lib.get_tex("dog"));
}