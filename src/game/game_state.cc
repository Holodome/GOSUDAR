#include "game/game_state.hh"
#include "game/game.hh"
#include "framework/renderer.hh"

#include "lib/perlin.hh"

void GameState::init() {
    logprintln("GameState", "Init start");
    
    this->local_dev_ui.init();
    dev_ui->font = assets->get_font("consolas");
    logprintln("GameState", "Init end");
}

void GameState::cleanup() {
    logprintln("GameState", "Cleanup");
}

void GameState::update_logic() {
    if (game->input.is_key_pressed(Key::F3)) {
        this->settings.enable_devui = !this->settings.enable_devui;
    }
    if (this->settings.enable_devui) {
        if (game->input.is_key_pressed(Key::F8)) {
            this->settings.focus_devui = !this->settings.focus_devui;
        }   
    }
    dev_ui->is_enabled = this->settings.enable_devui;
    dev_ui->is_focused = this->settings.focus_devui;
    
    bool is_game_focused = !this->settings.focus_devui || !this->settings.enable_devui;
    if (is_game_focused) {
        this->world.update();
    }
    
    dev_ui->window("Debug");
    dev_ui->textf("DevUI focused: %s", (dev_ui->is_focused ? "true" : "false"));
    dev_ui->textf("Draw call count: %llu", renderer->statistics.draw_call_count);
    dev_ui->textf("FPS: %.1f; DT: %.1fms", 1.0f / game->input.dt, game->input.dt * 1000.0f);
    if (dev_ui->checkbox("Fullscreen", &this->settings.fullscreen)) {
        game->os.go_fullscreen(this->settings.fullscreen);
    }
    if (dev_ui->button("Close game")) {
        game->is_running = false;
    }
    dev_ui->window_end();
}

void GameState::render() {
    this->world.render();
    dev_ui->end_frame();
}

void GameState::update() {
    dev_ui->begin_frame();
    update_logic();
    // @TODO find way to assert no calls to renderer made before this point...
    renderer->set_draw_region(game->input.winsize);
    renderer->clear(Vec4(0.2));
    render();   
}
