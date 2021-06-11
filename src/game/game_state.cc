#include "game/game_state.hh"
#include "game/game.hh"
#include "renderer/renderer.hh"

#include "lib/perlin.hh"

#include "game/mesh_utils.cc"

void GameState::init() {
    logprintln("GameState", "Init start");
    
    game->tex_lib.load("dog.jpg", "dog");
    
    cube = make_cube();
    rect = make_rect();
    this->dev_ui.font = new Font("c:\\windows\\fonts\\consola.ttf", 32);
    logprintln("GameState", "Init end");
}

void GameState::cleanup() {
    logprintln("GameState", "Cleanup");
    delete this->dev_ui.font;
    delete rect;
    delete cube;
}

void GameState::update_camera() {
    if (game->input.is_key_held(Key::MouseLeft)) {
        f32 x_view_coef = 1.0f * game->input.dt;
        f32 y_view_coef = 0.6f * game->input.dt;
        f32 x_angle_change = game->input.mdelta.x * x_view_coef;
        f32 y_angle_change = game->input.mdelta.y * y_view_coef;
        camera.rot.x += x_angle_change;
        camera.rot.x = Math::unwind_rad(camera.rot.x);
        camera.rot.y += y_angle_change;
        camera.rot.y = Math::clamp(camera.rot.y, -Math::HALF_PI, Math::HALF_PI);
    }
    
    f32 move_coef = 4.0f * game->input.dt;
    f32 z_speed = 0;
    if (game->input.is_key_held(Key::W)) {
        z_speed = move_coef;
    } else if (game->input.is_key_held(Key::S)) {
        z_speed = -move_coef;
    }
    camera.pos.x += z_speed *  sinf(camera.rot.x);
    camera.pos.z += z_speed * -cosf(camera.rot.x);
    
    f32 x_speed = 0;
    if (game->input.is_key_held(Key::D)) {
        x_speed = move_coef;
    } else if (game->input.is_key_held(Key::A)) {
        x_speed = -move_coef;
    }
    camera.pos.x += x_speed *  sinf(camera.rot.x + Math::HALF_PI);
    camera.pos.z += x_speed * -cosf(camera.rot.x + Math::HALF_PI);
    
    f32 y_speed = 0;
    if (game->input.is_key_held(Key::Ctrl)) {
        y_speed = -move_coef;
    } else if (game->input.is_key_held(Key::Space)) {
        y_speed = move_coef;
    }
    camera.pos.y += y_speed;
}

void GameState::update_input() {
    if (game->input.is_key_pressed(Key::F3)) {
        this->settings.enable_devui = !this->settings.enable_devui;
    }
    if (this->settings.enable_devui) {
        if (game->input.is_key_pressed(Key::F8)) {
            this->settings.focus_devui = !this->settings.focus_devui;
        }   
    }
    dev_ui.is_enabled = this->settings.enable_devui;
    dev_ui.is_focused = this->settings.focus_devui;
    
    bool is_game_focused = !this->settings.focus_devui || !this->settings.enable_devui;
    if (is_game_focused) {
        update_camera();
    } else {
        
    }
}

void GameState::update_logic() {
    camera.projection = Mat4x4::perspective(Math::rad(60), game->input.winsize.aspect_ratio(), 0.1f, 100.0f);
    camera.view = Mat4x4::identity() * Mat4x4::rotation(camera.rot.y, Vec3(1, 0, 0)) * Mat4x4::rotation(camera.rot.x, Vec3(0, 1, 0)) * Mat4x4::translate(-camera.pos);
    
    dev_ui.window("Debug", Rect(0, 0, 400, 400));
    dev_ui.textf("DevUI focused: %s", (dev_ui.is_focused ? "true" : "false"));
    dev_ui.textf("Draw call count: %llu", game->renderer.last_frame_statisitcs.draw_call_count);
    dev_ui.textf("FPS: %.1f; DT: %.1fms", 1.0f / game->input.dt, game->input.dt * 1000.0f);
    if (dev_ui.checkbox("Fullscreen", &this->settings.fullscreen)) {
        game->os.go_fullscreen(this->settings.fullscreen);
    }
    if (dev_ui.button("Close game")) {
        game->is_running = false;
    }
    dev_ui.drag_float3("Camera pos", camera.pos.e);
    dev_ui.drag_float3("Camera rot", camera.rot.e);
    dev_ui.window_end();
}

void GameState::render() {
    game->renderer.set_renderering_3d(camera.projection, camera.view);
    // Draw map
 
    game->renderer.imm_draw_rect(Rect(0, 0, 1, 1), Vec4(1), Rect(0, 0, 1, 1),  game->tex_lib.get_tex("dog"));   
}

void GameState::update() {
    this->dev_ui.begin_frame();
    update_input();
    update_logic();
    // @TODO find way to assert no calls to renderer made before this point...
    game->renderer.set_draw_region(game->input.winsize);
    game->renderer.clear(Vec4(0.2));
    render();   
    this->dev_ui.end_frame();
}
