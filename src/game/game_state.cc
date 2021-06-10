#include "game/game_state.hh"
#include "game/game.hh"
#include "renderer/renderer.hh"

#include "lib/perlin.hh"

static Mesh *make_cube() {
    Vec3 p0 = Vec3(0);
    Vec3 p1 = Vec3(1);
    Vec3 p[] = {
        Vec3(p1.x, p0.y, p0.z), Vec3(p1.x, p0.y, p1.z), 
        Vec3(p0.x, p0.y, p1.z), Vec3(p0.x, p0.y, p0.z),
        Vec3(p1.x, p1.y, p0.z), Vec3(p1.x, p1.y, p1.z), 
        Vec3(p0.x, p1.y, p1.z), Vec3(p0.x, p1.y, p0.z),
        Vec3(p1.x, p1.y, p1.z), Vec3(p1.x, p0.y, p1.z),
        Vec3(p1.x, p1.y, p1.z), Vec3(p0.x, p1.y, p1.z),
        Vec3(p0.x, p0.y, p1.z), Vec3(p0.x, p0.y, p1.z),
        Vec3(p0.x, p1.y, p1.z), Vec3(p0.x, p1.y, p0.z),
        Vec3(p0.x, p0.y, p0.z), Vec3(p0.x, p1.y, p0.z),
        Vec3(p1.x, p0.y, p0.z), Vec3(p1.x, p1.y, p0.z),
        Vec3(p1.x, p0.y, p0.z), Vec3(p1.x, p0.y, p1.z),
        Vec3(p0.x, p0.y, p0.z), Vec3(p1.x, p1.y, p0.z),
    };
    Vec3 n[] = {
        Vec3(0.000000, 0.000000, -0.999756), Vec3(0.000000, -0.999756, 0.000000), 
        Vec3(0.000000, -0.999756, 0.000000), Vec3(0.000000, -0.999756, 0.000000),
        Vec3(0.999756, 0.000000, 0.000000), Vec3(0.000000, 0.999756, 0.000000),
        Vec3(0.000000, 0.999756, 0.000000), Vec3(0.000000, 0.999756, 0.000000), 
        Vec3(0.999756, 0.000000, 0.000000), Vec3(0.999756, 0.000000, 0.000000), 
        Vec3(-0.000000, 0.000000, 0.999756), Vec3(-0.000000, 0.000000, 0.999756), 
        Vec3(-0.000000, 0.000000, 0.999756), Vec3(-0.999756, -0.000000, -0.000000), 
        Vec3(-0.999756, -0.000000, -0.000000), Vec3(-0.999756, -0.000000, -0.000000), 
        Vec3(0.000000, 0.000000, -0.999756), Vec3(0.000000, 0.000000, -0.999756), 
        Vec3(0.000000, -0.999756, 0.000000), Vec3(0.000000, 0.999756, 0.000000),
        Vec3(0.999756, 0.000000, 0.000000), Vec3(-0.000000, 0.000000, 0.999756),
        Vec3(-0.999756, -0.000000, -0.000000), Vec3(0.000000, 0.000000, -0.999756),
    };
    Vec2 uv[] = {
        Vec2(0.000000, 0.333333), Vec2(0.000000, 1.000000), Vec2(0.000000, 1.000000), 
        Vec2(0.000000, 0.666667), Vec2(0.000000, 0.000000), Vec2(0.000000, 0.666667), 
        Vec2(0.000000, 0.666667), Vec2(0.000000, 1.000000), Vec2(0.000000, 0.000000), 
        Vec2(0.000000, 0.333333), Vec2(0.000000, 0.333333), Vec2(0.000000, 0.666667), 
        Vec2(0.000000, 0.666667), Vec2(0.000000, 0.333333), Vec2(0.000000, 0.000000), 
        Vec2(0.000000, 0.000000), Vec2(0.000000, 0.333333), Vec2(0.000000, 0.000000), 
        Vec2(0.000000, 0.666667), Vec2(0.000000, 1.000000), Vec2(0.000000, 0.333333), 
        Vec2(0.000000, 0.333333), Vec2(0.000000, 0.333333), Vec2(0.000000, 0.000000),
    };
    u32 vi[] = {
        1, 2, 3, 7, 6, 5, 4, 8, 9, 10, 11, 12, 13, 14, 15, 0, 16,
        17, 18, 1, 3, 19, 7, 5, 20, 4, 9, 21, 10, 12, 22, 13, 15, 23, 0, 17,  
    };
    
    Vertex vertices[ARRAY_SIZE(p)];
    for (size_t i = 0; i < ARRAY_SIZE(p); ++i) {
        vertices[i].p = p[i];
        vertices[i].uv = uv[i];
        vertices[i].n = n[i];
        vertices[i].c = Vec4(1);
    } 
    
    return new Mesh(vertices, ARRAY_SIZE(p), vi, ARRAY_SIZE(vi));
}

static Mesh *make_rect() {
    Vec3 p[] = {
        Vec3(0, 0, 0),
        Vec3(0, 1, 0),
        Vec3(1, 0, 0),
        Vec3(1, 1, 0),
    };
    u32 vi[] = {
        0, 1, 2, 1, 2, 3
    };
    Vertex vertices[ARRAY_SIZE(p)];
    for (size_t i = 0; i < ARRAY_SIZE(p); ++i) {
        vertices[i].p = p[i];
        vertices[i].uv = p[i].xy;
        vertices[i].n = Vec3(0, 0, 1);
        vertices[i].c = Vec4(1);
    }
    return new Mesh(vertices, ARRAY_SIZE(p), vi, ARRAY_SIZE(vi));
}

static Mesh *make_map(Vec2i size, f32 *height_map) {
    u32 index_count = (size.x - 1) * (size.y - 1) * 6;
#if 0
    TempArray<Index> indices(index_count);
    TempArray<Vertex> vertices((size_t)size.product());
    u32 cursor = 0;
    for (u32 i = 0; i < size.y; ++i) {
        for (u32 j = 0; j < size.x; ++j) {
            vertices[cursor].p = Vec3(i, height_map[i * size.x + j], j);
            vertices[cursor].c = bilerp(Colors::red, Colors::green, Colors::blue, Colors::white, 
                                        (f32)j / (f32)size.x, (f32)i / (f32)size.y);
            vertices[cursor].n = Vec3(0, 1, 0);
            vertices[cursor].uv = Vec2(0, 0);
            ++cursor;
        }
    }
    
    cursor = 0;
    for (u32 i = 0; i < size.y - 1; ++i) {
        for (u32 j = 0; j < size.x - 1; ++j) {
            u32 v00 = (i + 0) * size.x + (j + 0);
            u32 v01 = (i + 0) * size.x + (j + 1);
            u32 v10 = (i + 1) * size.x + (j + 0);
            u32 v11 = (i + 1) * size.x + (j + 1);
            
            indices[cursor++] = v00;
            indices[cursor++] = v01;
            indices[cursor++] = v11;
            indices[cursor++] = v11;
            indices[cursor++] = v10;
            indices[cursor++] = v00;
        }
    }
    
    return new Mesh(vertices.data, vertices.len, indices.data, indices.len);
#else 
    TempArray<Index> indices(index_count);
    TempArray<Vertex> vertices(index_count);
    TempArray<Vertex> actual_vertices((size_t)size.product());
    for (u32 i = 0; i < size.y; ++i) {
        for (u32 j = 0; j < size.x; ++j) {
            actual_vertices[i * size.x + j].p = Vec3(i, height_map[i * size.x + j], j);
            actual_vertices[i * size.x + j].c = bilerp(Colors::red, Colors::green, Colors::blue, Colors::white, 
                                        (f32)j / (f32)size.x, (f32)i / (f32)size.y);
            actual_vertices[i * size.x + j].n = Vec3(0, 1, 0);
            actual_vertices[i * size.x + j].uv = Vec2(0, 0);
        }
    }
    u32 cursor = 0;
    for (u32 i = 0; i < size.y - 1; ++i) {
        for (u32 j = 0; j < size.x - 1; ++j) {
            u32 v00 = (i + 0) * size.x + (j + 0);
            u32 v01 = (i + 0) * size.x + (j + 1);
            u32 v10 = (i + 1) * size.x + (j + 0);
            u32 v11 = (i + 1) * size.x + (j + 1);
            
            vertices[cursor] = actual_vertices[v00];
            vertices[cursor].n = triangle_normal(actual_vertices[v00].p, actual_vertices[v01].p, actual_vertices[v11].p);
            vertices[cursor].c = (actual_vertices[v00].c, actual_vertices[v01].c, actual_vertices[v11].c) / 3.0f;
            indices[cursor] = cursor;
            ++cursor;
            vertices[cursor] = actual_vertices[v01];
            indices[cursor] = cursor;
            ++cursor;
            vertices[cursor] = actual_vertices[v11];
            indices[cursor] = cursor;
            ++cursor;
            vertices[cursor] = actual_vertices[v11];
            vertices[cursor].n = triangle_normal(actual_vertices[v11].p, actual_vertices[v10].p, actual_vertices[v00].p);
            vertices[cursor].c = (actual_vertices[v11].c + actual_vertices[v10].c + actual_vertices[v00].c) / 3.0f;
            indices[cursor] = cursor;
            ++cursor;
            vertices[cursor] = actual_vertices[v10];
            indices[cursor] = cursor;
            ++cursor;
            vertices[cursor] = actual_vertices[v00];
            indices[cursor] = cursor;
            ++cursor;
        }
    }
    
    return new Mesh(vertices.data, vertices.len, indices.data, indices.len);
#endif 
}

void GameState::init() {
    logprintln("GameState", "Init start");
    cube = make_cube();
    rect = make_rect();
    Vec2i map_size(30, 30);
    TempArray<f32> height_map(map_size.product());
    Perlin perlin = Perlin(0.35f, 5, 4, 100);
    for (u32 i = 0; i < map_size.y; ++i) {
        for (u32 j = 0; j < map_size.x; ++j) {
            f32 noise = perlin.get_perlin_noise(i, j);
            height_map[i * map_size.x + j] = noise;
        }
    }
    map = make_map(map_size, height_map.data);
    logprintln("GameState", "Init end");
    
    this->tex_lib.texture_ids.set("a", 2);
    this->tex_lib.texture_ids.set("b", 3);
    this->tex_lib.texture_ids.set("c", 4);
    size_t *a, *b, *c;
    this->tex_lib.texture_ids.get("a", &a);
    this->tex_lib.texture_ids.get("b", &b);
    this->tex_lib.texture_ids.get("c", &c);
    printf("%u %u %u\n", *a, *b, *c); 
    this->tex_lib.texture_ids.del("a");
    this->tex_lib.texture_ids.get("a", &a);
    printf("%llu\n", a); 
}

void GameState::cleanup() {
    logprintln("GameState", "Cleanup");
    delete map;
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
    game->dev_ui.is_enabled = this->settings.enable_devui;
    game->dev_ui.is_focused = this->settings.focus_devui;
    
    bool is_game_focused = !this->settings.focus_devui || !this->settings.enable_devui;
    if (is_game_focused) {
        update_camera();
    } else {
        
    }
}

void GameState::update_logic() {
    camera.projection = Mat4x4::perspective(Math::rad(60), game->input.winsize.aspect_ratio(), 0.1f, 100.0f);
    camera.view = Mat4x4::identity() * Mat4x4::rotation(camera.rot.y, Vec3(1, 0, 0)) * Mat4x4::rotation(camera.rot.x, Vec3(0, 1, 0)) * Mat4x4::translate(-camera.pos);
}

void GameState::render() {
    game->renderer.set_renderering_3d(camera.projection, camera.view);
    game->renderer.immediate_begin();
    game->renderer.set_shader();
    game->renderer.set_texture();
    game->renderer.draw_mesh(cube);
    game->renderer.immediate_flush();
    game->renderer.immediate_begin();
    game->renderer.set_shader(game->renderer.terrain_shader);
    game->renderer.set_texture();
    game->renderer.draw_mesh(map);
    game->renderer.immediate_flush();
    
    game->dev_ui.window("Debug", Rect(0, 0, 400, 400));
    game->dev_ui.textf("DevUI focused: %s", (game->dev_ui.is_focused ? "true" : "false"));
    game->dev_ui.textf("Draw call count: %llu", game->renderer.statistics.draw_call_count);
    game->dev_ui.textf("FPS: %.1f; DT: %.1fms", 1.0f / game->input.dt, game->input.dt * 1000.0f);
    if (game->dev_ui.checkbox("Fullscreen", &this->settings.fullscreen)) {
        game->os.go_fullscreen(this->settings.fullscreen);
    }
    if (game->dev_ui.button("Close game")) {
        game->is_running = false;
    }
    game->dev_ui.drag_float3("Camera pos", camera.pos.e);
    game->dev_ui.drag_float3("Camera rot", camera.rot.e);
    game->dev_ui.window_end();
}

void GameState::update() {
    game->renderer.set_draw_region(game->input.winsize);
    game->renderer.clear(Vec4(0.2));

    update_input();
    update_logic();
    render();   
}
