#if !defined(SOUND_HH)

#include "lib.hh"

struct AudioSource {
    struct AssetSound *sound;
    bool is_playing;
    // Cursor in samples of plating sound.
    // In case sound play rate is different from output rate, we a have a float for cursor
    f64 play_cursor;
};

#define AUDIO_SOURCES_MAX_COUNT 256

struct Audio {
    u32 sources_count;
    AudioSource sources[AUDIO_SOURCES_MAX_COUNT];
};

#define SOUND_HH 1
#endif
