#include "game/game_state.hh"
#include "game/game.hh"
#include "renderer/renderer.hh"

GameState::GameState() {
}

void GameState::init() {
    // ui.font = new Font("c:/windows/fonts/arial.ttf", 32);
    Mesh *terrain_mesh = new Mesh("terrian");
    game->renderer.mesh_lib.add(terrain_mesh);
    camera_pos = Vec3(0, 0, 0);
    camera_rot = Vec3(0, 0, 0);
    logprint("GameState", "Initialized");
}

void GameState::update() {
    f32 view_coef = 1.0f * game->input.dt;
    f32 x_angle_change = game->input.mdelta.x * view_coef;
    f32 y_angle_change = game->input.mdelta.y * view_coef;
    camera_rot.x += x_angle_change;
    camera_rot.y += y_angle_change;
    
    f32 move_coef = 4.0f * game->input.dt;
    f32 z_speed = 0;
    if (game->input.is_key_held(Key::W)) {
        z_speed = move_coef;
    } else if (game->input.is_key_held(Key::S)) {
        z_speed = -move_coef;
    }
    camera_pos.x += z_speed *  sinf(camera_rot.x);
    camera_pos.z += z_speed * -cosf(camera_rot.x);
    
    f32 x_speed = 0;
    if (game->input.is_key_held(Key::D)) {
        x_speed = move_coef;
    } else if (game->input.is_key_held(Key::A)) {
        x_speed = -move_coef;
    }
    camera_pos.x += x_speed *  sinf(camera_rot.x + HALF_PI);
    camera_pos.z += x_speed * -cosf(camera_rot.x + HALF_PI);
    
    f32 y_speed = 0;
    if (game->input.is_key_held(Key::Ctrl)) {
        y_speed = -move_coef;
    } else if (game->input.is_key_held(Key::Space)) {
        y_speed = move_coef;
    }
    camera_pos.y += y_speed;
    
    Mat4x4 projection = Mat4x4::perspective(rad(60), game->input.winsize.aspect_ratio(), 0.1f, 100.0f);
    Mat4x4 view = Mat4x4::identity() * Mat4x4::translate(camera_pos) * Quat4::to_mat4x4(Quat4::euler(camera_rot.x, camera_rot.y, camera_rot.z));
    game->renderer.set_mats(projection, view);
    game->renderer.draw_mesh(game->renderer.mesh_lib.get("quad"), game->renderer.image_lib.get("assets/white.png"),
                             Vec3(0), Quat4::identity(), Vec3(200, 100, 1));
}