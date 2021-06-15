#include "game/camera.hh"

#include "game/game.hh"
#include "game/ray_casting.hh"

void Camera::update_input() {
    if (game->input.is_key_held(Key::MouseLeft)) {
        f32 x_view_coef = 1.0f * game->input.dt;
        f32 y_view_coef = 0.6f * game->input.dt;
        f32 x_angle_change = game->input.mdelta.x * x_view_coef;
        f32 y_angle_change = game->input.mdelta.y * y_view_coef;
        this->rot.x += x_angle_change;
        this->rot.x = Math::unwind_rad(this->rot.x);
        this->rot.y += y_angle_change;
        this->rot.y = Math::clamp(this->rot.y, -Math::HALF_PI, Math::HALF_PI);
    }
    
    f32 move_coef = 4.0f * game->input.dt;
    f32 z_speed = 0;
    if (game->input.is_key_held(Key::W)) {
        z_speed = move_coef;
    } else if (game->input.is_key_held(Key::S)) {
        z_speed = -move_coef;
    }
    this->pos.x += z_speed *  sinf(this->rot.x);
    this->pos.z += z_speed * -cosf(this->rot.x);
    
    f32 x_speed = 0;
    if (game->input.is_key_held(Key::D)) {
        x_speed = move_coef;
    } else if (game->input.is_key_held(Key::A)) {
        x_speed = -move_coef;
    }
    this->pos.x += x_speed *  sinf(this->rot.x + Math::HALF_PI);
    this->pos.z += x_speed * -cosf(this->rot.x + Math::HALF_PI);
    
    f32 y_speed = 0;
    if (game->input.is_key_held(Key::Ctrl)) {
        y_speed = -move_coef;
    } else if (game->input.is_key_held(Key::Space)) {
        y_speed = move_coef;
    }
    this->pos.y += y_speed;
}

void Camera::recalculate_matrices() {
    f32 vfov = Math::rad(60);
    this->projection = Mat4x4::perspective(vfov, game->input.winsize.aspect_ratio(), 0.1f, 100.0f);
    this->view = Mat4x4::identity() * Mat4x4::rotation(this->rot.y, Vec3(1, 0, 0)) * Mat4x4::rotation(this->rot.x, Vec3(0, 1, 0)) * Mat4x4::translate(-this->pos);
    this->mvp = this->projection * this->view;
}

Vec3 Camera::screen_to_world(Vec2 screen) {
    f32 x = (2.0f * screen.x) / game->input.winsize.x - 1.0f;
    f32 y = 1.0f - (2.0f * screen.y) / game->input.winsize.y;
    return uv_to_world(Vec2(x, y));
}

// UV is in [-1; 1]
Vec3 Camera::uv_to_world(Vec2 uv) {
    f32 x = uv.x;
    f32 y = uv.y;
    Vec3 ray_dc = Vec3(x, y, 1.0f);
    Vec4 ray_clip = Vec4(ray_dc.xy, -1.0f, 1.0f);
    Vec4 ray_eye = Mat4x4::inverse(this->projection) * ray_clip;
    ray_eye.z = -1.0f;
    ray_eye.w = 0.0f;
    Vec3 ray_world = Math::normalize((Mat4x4::inverse(this->view) * ray_eye).xyz);
    return ray_world;
}
