#include "game/world.hh"
#include "game/game.hh"
 
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

void World::init() {
    
}

void World::cleanup() {
    
}

void World::update() {
    // Move player
    f32 move_coef = 4.0f * game->input.dt;
    f32 z_speed = 0;
    if (game->input.is_key_held(Key::W)) {
        z_speed = move_coef;
    } else if (game->input.is_key_held(Key::S)) {
        z_speed = -move_coef;
    }
    this->player_pos.x += z_speed *  Math::sin(this->camera.yaw);
    this->player_pos.z += z_speed * -Math::cos(this->camera.yaw);
    
    f32 x_speed = 0;
    if (game->input.is_key_held(Key::D)) {
        x_speed = move_coef;
    } else if (game->input.is_key_held(Key::A)) {
        x_speed = -move_coef;
    }
    this->player_pos.x += x_speed * Math::cos(this->camera.yaw);
    this->player_pos.z += x_speed * Math::sin(this->camera.yaw);
    this->camera.center_pos = this->player_pos;
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
    dev_ui->window("World");
    dev_ui->drag_float3("Player pos", this->player_pos.e);
    dev_ui->value("Selected point", this->point_on_plane);
    dev_ui->value("Camera pos", this->camera.pos);
    dev_ui->window_end();
    renderer->set_renderering_3d(camera.mvp);
    // Draw map
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
    
    Vec3 player_v[4];
    this->get_billboard_positions(this->player_pos, 0.5f, 0.5f, player_v);
    renderer->imm_draw_quad(player_v, assets->get_tex("dude"));
}