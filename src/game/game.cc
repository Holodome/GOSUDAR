#include "game/game.hh"

#include <thirdparty/stb_vorbis.h>

Sound sound_load(const char *filename) {
    Sound result;
    int channels;
    int sample_rate;
    result.sample_count = stb_vorbis_decode_filename(filename, &channels, &sample_rate, &result.samples);
    result.channels = channels;
    result.sample_rate = sample_rate;
    result.sample_count *= result.channels;
    return result;
}

void game_init(Game *game) {
    logprintln("Game", "Init start");
    game->is_running = true;   
    game->debug_state = DEBUG_init();
    game->os = os_init();
    init_renderer_backend(game->os);
    renderer_init(&game->renderer);
    
    Sound sound = sound_load("music.ogg");
    AudioSource source = {};
    source.sound = sound;
    source.is_playing = 0;
    source.play_position = 0;
    game->audio.sources[game->audio.sources_count++] = source;
    
    game->assets = assets_init(&game->renderer);
    game_state_init(&game->game_state);
}

void game_cleanup(Game *game) {
}

void game_update_and_render(Game *game) {
    FRAME_MARKER();
    DEBUG_begin_frame(game->debug_state);
    
    DEBUG_SWITCH(&game->game_state.show_grid, "Show grid");
    {DEBUG_VALUE_BLOCK("Debug")
        DEBUG_VALUE(game->debug_state->value_blocks_allocated, "Value blocks allocated");
        DEBUG_VALUE(game->debug_state->debug_open_blocks_allocated, "Blocks allocated");
        DEBUG_VALUE(game->debug_state->debug_values_allocated, "Values allocated");
    }
    {DEBUG_VALUE_BLOCK("Memory")
        DEBUG_VALUE(game->debug_state->arena.peak_size >> 10, "Debug arena size");
        DEBUG_VALUE(game->game_state.frame_arena.peak_size >> 10, "Frame arena size");
        DEBUG_VALUE(game->game_state.arena.peak_size >> 10, "Game arena size");
        DEBUG_VALUE(game->renderer.arena.peak_size >> 10, "Renderer arena size");
        DEBUG_VALUE(game->assets->arena.peak_size >> 10, "Assets arena size");
    }
    
    Input *os_input = update_input(game->os);
    InputManager input_ = create_input_manager(os_input);
    InputManager *input = &input_;
    if (is_key_pressed(input, KEY_ESCAPE, INPUT_ACCESS_TOKEN_ALL) || os_input->is_quit_requested) {
        game->is_running = false;
    }    
    
    update_audio(&game->audio, os_input->sound_samples, os_input->sample_count_to_output);
    
    RendererCommands *commands = renderer_begin_frame(&game->renderer, window_size(input), Vec4(0.2));
    update_and_render(&game->game_state, input, commands, game->assets);
    DEBUG_update(game->debug_state, input, commands, game->assets);
    renderer_end_frame(&game->renderer);
    update_window(game->os);
    DEBUG_frame_end(game->debug_state);
}
