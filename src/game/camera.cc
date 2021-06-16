#include "game/camera.hh"

#include "game/game.hh"
#include "game/ray_casting.hh"

void Camera::init() {
    
}

void Camera::update() {
    this->distance_from_player -= game->input.mwheel;
    
    if (game->input.is_key_held(Key::MouseLeft)) {
        f32 x_view_coef = 1.0f * game->input.dt;
        f32 y_view_coef = 0.6f * game->input.dt;
        f32 x_angle_change = game->input.mdelta.x * x_view_coef;
        f32 y_angle_change = game->input.mdelta.y * y_view_coef;
        this->yaw += x_angle_change;
        this->yaw = Math::unwind_rad(this->yaw);
        this->pitch += y_angle_change;
        this->pitch = Math::clamp(this->pitch, 0.01f, Math::HALF_PI - 0.01f);
    }
    
    f32 horiz_distance = distance_from_player * Math::cos(this->pitch);
    f32 vert_distance = distance_from_player * Math::sin(this->pitch);
    f32 offsetx = horiz_distance * Math::sin(-this->yaw);
    f32 offsetz = horiz_distance * Math::cos(-this->yaw);
    this->pos.x = offsetx + this->center_pos.x;
    this->pos.z = offsetz + this->center_pos.z;
    this->pos.y = vert_distance;
}

void Camera::recalculate_matrices() {
    this->projection = Mat4x4::perspective(this->fov, game->input.winsize.aspect_ratio(), this->near_plane, this->far_plane);
    this->view = Mat4x4::identity() * Mat4x4::rotation(this->pitch, Vec3(1, 0, 0)) * Mat4x4::rotation(this->yaw, Vec3(0, 1, 0)) * Mat4x4::translate(-this->pos);
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