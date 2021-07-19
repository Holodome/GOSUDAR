#if !defined(AUDIO_HH)

#include "lib.hh"
#include "entity.hh"

enum {
    AUDIO_CATEGORY_NONE,
    AUDIO_CATEGORY_SFX,
    AUDIO_CATEGORY_MUSIC,
    AUDIO_CATEGORY_SENTINEL,
};

struct PlayingSound {
    u32 category;
    AssetID sound_id;
    f64 play_cursor;
};

#define MAX_SOUNDS 128

struct AudioSystem {
    f64 global_volume;
    f64 category_volumes[AUDIO_CATEGORY_SENTINEL];
    
    PlayingSound sounds[MAX_SOUNDS];
    bool sounds_play[MAX_SOUNDS];
};

void init_audio_system(AudioSystem *sys);
AudioID play_audio(AudioSystem *sys, AssetID sound_id);
void stop_audio(AudioSystem *sys, AudioID sound_id);
void update_audio(AudioSystem *sys, Assets *assets, Platform *platform);

#define AUDIO_HH 1
#endif 