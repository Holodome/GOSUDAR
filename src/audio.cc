#include "audio.hh"

void init_audio_system(AudioSystem *sys) {
    sys->global_volume = 1.0f;
    for (u32 i = 0; i < AUDIO_CATEGORY_SENTINEL; ++i) {
        sys->category_volumes[i] = 1.0f;
    }
}

AudioID play_audio(AudioSystem *sys, AssetID sound_id) {
    AudioID result = {};
    for (u32 i = 0; i < MAX_SOUNDS; ++i) {
        if (!sys->sounds_play[i]) {
            PlayingSound sound = {};
            sound.category = AUDIO_CATEGORY_NONE;
            sound.sound_id = sound_id;
            sys->sounds[i] = sound;
            sys->sounds_play[i] = true;
            result.value = i;
            break;
        }
    }
    return result;
}

void stop_audio(AudioSystem *sys, AudioID id) {
    assert(sys->sounds_play[id.value]);
    sys->sounds_play[id.value] = false;
}

void update_audio(AudioSystem *sys, Assets *assets, Platform *platform) {
    for (u32 i = 0; i < MAX_SOUNDS; ++i) {
        if (!sys->sounds_play[i]) {
            continue;
        }
        
        PlayingSound *playing_sound = sys->sounds + i;
        AssetInfo *sound_info = assets_get_info(assets, playing_sound->sound_id);
        AssetSound *sound = assets_get_sound(assets, playing_sound->sound_id);
        
        i16 *sample_out = platform->sound_samples;
        f64 sound_pitch = ((f64)sound_info->sample_rate / (f64)platform->samples_per_second);
        f64 sound_volume = sys->global_volume * sys->category_volumes[playing_sound->category];
        for (u64 write_sample = 0;
             write_sample < platform->sample_count_to_output;
             ++write_sample) {
            f64 start_play_cursor = playing_sound->play_cursor;
            
            f64 target_play_cursor = start_play_cursor + (f64)sound_info->channels * sound_pitch;
            if (target_play_cursor >= sound_info->sample_count) {
                target_play_cursor -= sound_info->sample_count;
            }
            // Get source samples
            i16 start_left_sample, start_right_sample;
            {
                u64 left_idx = (u64)start_play_cursor;
                if (sound_info->channels == 2) {
                    left_idx = left_idx ^ (left_idx & 0x1);
                }
                u64 right_idx = left_idx + (sound_info->channels - 1);
                
                i16 first_left_sample = sound->samples[left_idx];
                i16 first_right_sample = sound->samples[right_idx];
                i16 second_left_sample = sound->samples[left_idx + sound_info->channels];
                i16 second_right_sample = sound->samples[right_idx + sound_info->channels];
                start_left_sample = (i16)(first_left_sample + (second_left_sample - first_left_sample) * 
                                          (start_play_cursor / sound_info->channels - (u64)(start_play_cursor / sound_info->channels)));
                start_right_sample = (i16)(first_right_sample + (second_right_sample - first_right_sample) * 
                                           (start_play_cursor / sound_info->channels - (u64)(start_play_cursor / sound_info->channels)));
            }
            i16 target_left_sample, target_right_sample;
            {
                u64 left_idx = (u64)target_play_cursor;
                if (sound_info->channels == 2) {
                    left_idx = left_idx ^ (left_idx & 0x1);
                }
                u64 right_idx = left_idx + (sound_info->channels - 1);
                
                i16 first_left_sample = sound->samples[left_idx];
                i16 first_right_sample = sound->samples[right_idx];
                i16 second_left_sample = sound->samples[left_idx + sound_info->channels];
                i16 second_right_sample = sound->samples[right_idx + sound_info->channels];
                target_left_sample = (i16)(first_left_sample + (second_left_sample - first_left_sample) * 
                                           (target_play_cursor / sound_info->channels - (u64)(target_play_cursor / sound_info->channels)));
                target_right_sample = (i16)(first_right_sample + (second_right_sample - first_right_sample) * 
                                            (target_play_cursor / sound_info->channels - (u64)(target_play_cursor / sound_info->channels)));
            }
            // Get write sample
            i16 left_sample = (i16)(((i64)start_left_sample + (i64)target_left_sample) / 2 * sound_volume);
            i16 right_sample = (i16)(((i64)start_right_sample + (i64)target_right_sample) / 2 * sound_volume);
            // Write sound
            *sample_out++ += left_sample;
            *sample_out++ += right_sample;
            
            playing_sound->play_cursor = target_play_cursor;
            if (playing_sound->play_cursor >= sound_info->sample_count - sound_info->channels - 1) {
                sys->sounds_play[i] = false;
            }
        }
    }
}
