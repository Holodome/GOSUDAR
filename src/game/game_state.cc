#include "game/game_state.hh"
#include "game/game.hh"
#include "renderer/renderer.hh"

#include "lib/perlin.hh"

#include "game/mesh_utils.cc"

void GameState::init() {
    logprintln("GameState", "Init start");
    
    game->tex_lib.load("e:\\dev\\GAMEMEME\\dog.jpg", "dog");
    game->tex_lib.load("e:\\dev\\GAMEMEME\\dude.png", "dude");
    
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
        this->camera.update_input();
    } else {
        
    }
}

void GameState::update_logic() {
    dev_ui.window("Dev", Rect(500, 0, 400, 300));
    
    this->camera.recalculate_matrices();
    Vec3 ray_dir = this->camera.screen_to_world(game->input.mpos);
    f32 t = 0;
    if (ray_intersect_plane(Vec3(0, 1, 0), 0, Ray(camera.pos, ray_dir), &t) && t > 0) {
        this->point_on_plane = camera.pos + ray_dir * t;
    } else {
        this->point_on_plane = Vec3(0, 1, 0);
    }
    dev_ui.value("Selected point", this->point_on_plane);
    dev_ui.window_end();
    this->player_pos = this->camera.pos;
    
    dev_ui.window("Debug", Rect(0, 0, 400, 400));
    dev_ui.textf("DevUI focused: %s", (dev_ui.is_focused ? "true" : "false"));
    dev_ui.textf("Draw call count: %llu", game->renderer.statistics.draw_call_count);
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
    game->renderer.set_renderering_3d(camera.mvp);
    // // Draw map
    Vec2i map_size = Vec2i(10, 10);
    f32 tile_size = 2.0f;
    for (size_t y = 0; y < map_size.y; ++y) {
        for (size_t x = 0; x < map_size.x; ++x) {
            Vec4 color = Colors::green;
            game->renderer.imm_draw_quad(Vec3(x, 0, y) * tile_size, Vec3(x, 0, y + 1) * tile_size,
                                         Vec3(x + 1, 0, y) * tile_size, Vec3(x + 1, 0, y + 1) * tile_size,
                                         color);
        }
    }
    if (Math::length(this->point_on_plane - Vec3(0, 1, 0)) > 0.001f) {
        i32 x = this->point_on_plane.x / tile_size;
        i32 y = this->point_on_plane.z / tile_size;
        if (0 <= x && x < map_size.x && 0 <= y && y < map_size.y) {
            game->renderer.imm_draw_quad_outline(Vec3(x, 0, y) * tile_size, Vec3(x, 0, y + 1) * tile_size,
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
    game->renderer.imm_draw_quad(top_left, bottom_left, top_right, bottom_right, Colors::white, game->tex_lib.get_tex("dog"));
    
    game->renderer.set_renderering_2d(game->input.winsize);
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
