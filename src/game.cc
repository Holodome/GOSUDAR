#include "game.hh"

void update_audio(Audio *audio, Input *input) {
    TIMED_FUNCTION();
    for (size_t i = 0; i < audio->sources_count; ++i) {
        AudioSource *source = audio->sources + i;
        AssetSound *sound = source->sound;
          
        i16 *sample_out = input->sound_samples;
          
        for (size_t write_sample = 0; write_sample < input->sample_count_to_output; ++write_sample) {
            f64 start_play_cursor = source->play_cursor;
            
            f64 target_play_cursor = start_play_cursor + (f64)sound->channels * ((f64)sound->sample_rate / (f64)input->samples_per_second);
            if (target_play_cursor >= sound->sample_count) {
                target_play_cursor -= sound->sample_count;
            }
            // Get source samples
            i16 start_left_sample, start_right_sample;
            {
                u64 left_idx = (u64)start_play_cursor;
                if (sound->channels == 2) {
                    left_idx = left_idx ^ (left_idx & 0x1);
                }
                u64 right_idx = left_idx + (sound->channels - 1);
                
                i16 first_left_sample = sound->samples[left_idx];
                i16 first_right_sample = sound->samples[right_idx];
                i16 second_left_sample = sound->samples[left_idx + sound->channels];
                i16 second_right_sample = sound->samples[right_idx + sound->channels];
                start_left_sample = (i16)(first_left_sample + (second_left_sample - first_left_sample) * 
                    (start_play_cursor / sound->channels - (u64)(start_play_cursor / sound->channels)));
                start_right_sample = (i16)(first_right_sample + (second_right_sample - first_right_sample) * 
                    (start_play_cursor / sound->channels - (u64)(start_play_cursor / sound->channels)));
            }
            i16 target_left_sample, target_right_sample;
            {
                u64 left_idx = (u64)target_play_cursor;
                if (sound->channels == 2) {
                    left_idx = left_idx ^ (left_idx & 0x1);
                }
                u64 right_idx = left_idx + (sound->channels - 1);
                
                i16 first_left_sample = sound->samples[left_idx];
                i16 first_right_sample = sound->samples[right_idx];
                i16 second_left_sample = sound->samples[left_idx + sound->channels];
                i16 second_right_sample = sound->samples[right_idx + sound->channels];
                target_left_sample = (i16)(first_left_sample + (second_left_sample - first_left_sample) * 
                    (target_play_cursor / sound->channels - (u64)(target_play_cursor / sound->channels)));
                target_right_sample = (i16)(first_right_sample + (second_right_sample - first_right_sample) * 
                    (target_play_cursor / sound->channels - (u64)(target_play_cursor / sound->channels)));
            }
            // Get write sample
            i16 left_sample = (i16)(((i64)start_left_sample + (i64)target_left_sample) / 2);
            i16 right_sample = (i16)(((i64)start_right_sample + (i64)target_right_sample) / 2);
            // Write sound
            *sample_out++ += left_sample;
            *sample_out++ += right_sample;
            
            source->play_cursor = target_play_cursor;
        }
    }
}

void game_init(Game *game) {
    game->is_running = true;   
    game->debug_state = DEBUG_init();
    game->os = os_init();
    init_renderer_backend(game->os);
    renderer_init(&game->renderer);

    game->assets = assets_init(&game->renderer);
    game_state_init(&game->game_state);

    AssetSound *sound = assets_get_sound(game->assets, get_first_of_type(game->assets, ASSET_TYPE_SOUND));  
    AudioSource source;
    source.is_playing = true;
    source.sound = sound;
    source.play_cursor = 0.0f;
    game->audio.sources[game->audio.sources_count++] = source;
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
    
    RendererCommands *commands = renderer_begin_frame(&game->renderer, window_size(input), Vec4(0.2));
    update_and_render(&game->game_state, input, commands, game->assets);
    DEBUG_update(game->debug_state, input, commands, game->assets);
    update_audio(&game->audio, os_input);
    renderer_end_frame(&game->renderer);
    update_window(game->os);
    DEBUG_frame_end(game->debug_state);
}
