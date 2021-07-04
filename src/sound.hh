#if !defined(SOUND_HH)

#include "lib.hh"

struct Sound {
    u32 channels;
    u32 sample_rate;
    i16 *samples;
    u32  sample_count;  
};

struct AudioSource {
    Sound sound;
    f64 play_position;
    bool is_playing;
};

#define AUDIO_SOURCES_MAX_COUNT 256

struct Audio {
    u32 sources_count;
    AudioSource sources[AUDIO_SOURCES_MAX_COUNT];
};

void update_audio(Audio *audio, i16 *sample_out, u64 sample_count_to_output) {
    for (size_t i = 0; i < audio->sources_count; ++i) {
        AudioSource *source = audio->sources + i;
        Sound *sound = &source->sound;
          
        for (size_t write_sample = 0; write_sample < sample_count_to_output; ++write_sample) {
            f64 start_play_position = source->play_position;
            
            f64 target_play_position = start_play_position + (f64)sound->channels * (f64)sound->sample_rate;
            if (target_play_position >= sound->sample_count) {
                target_play_position -= sound->sample_count;
            }
            // Get source samples
            i16 start_left_sample, start_right_sample;
            {
                u64 left_idx = (u64)start_play_position;
                if (sound->channels == 2) {
                    left_idx = left_idx ^ (left_idx & 0x1);
                }
                u64 right_idx = left_idx + (sound->channels - 1);
                
                i16 first_left_sample = sound->samples[left_idx];
                i16 first_right_sample = sound->samples[right_idx];
                i16 second_left_sample = sound->samples[left_idx + sound->channels];
                i16 second_right_sample = sound->samples[right_idx + sound->channels];
                start_left_sample = (i16)(first_left_sample + (second_left_sample - first_left_sample) * 
                    (start_play_position / sound->channels - (u64)(start_play_position / sound->channels)));
                start_right_sample = (i16)(first_right_sample + (second_right_sample - first_right_sample) * 
                    (start_play_position / sound->channels - (u64)(start_play_position / sound->channels)));
            }
            i16 target_left_sample, target_right_sample;
            {
                u64 left_idx = (u64)target_play_position;
                if (sound->channels == 2) {
                    left_idx = left_idx ^ (left_idx & 0x1);
                }
                u64 right_idx = left_idx + (sound->channels - 1);
                
                i16 first_left_sample = sound->samples[left_idx];
                i16 first_right_sample = sound->samples[right_idx];
                i16 second_left_sample = sound->samples[left_idx + sound->channels];
                i16 second_right_sample = sound->samples[right_idx + sound->channels];
                target_left_sample = (i16)(first_left_sample + (second_left_sample - first_left_sample) * 
                    (start_play_position / sound->channels - (u64)(start_play_position / sound->channels)));
                target_right_sample = (i16)(first_right_sample + (second_right_sample - first_right_sample) * 
                    (start_play_position / sound->channels - (u64)(start_play_position / sound->channels)));
            }
            // Get write sample
            i16 left_sample = (i16)(((i64)start_left_sample + (i16)target_left_sample) / 2);
            i16 right_sample = (i16)(((i64)start_right_sample + (i16)target_right_sample) / 2);
            // Write sound
            *sample_out++ += left_sample;
            *sample_out++ += right_sample;
            
            source->play_position = target_play_position;
        }
    }
}

#define SOUND_HH 1
#endif
