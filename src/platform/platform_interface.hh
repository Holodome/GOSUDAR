/*
Author: Holodome
Date: 03.10.2021
File: src/platform/input.hh
Version: 0
*/
#pragma once
#include "lib/general.hh"
#include "math/vec.hh"

enum KeyKind : u32 {
    KEY_NONE,
    KEY_W,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_Z,
    KEY_B,
    KEY_X,
    KEY_SHIFT,
    KEY_CTRL,
    KEY_ALT,
    KEY_SPACE,
    KEY_ENTER, 
    KEY_ESCAPE,
    KEY_BACKSPACE,
    KEY_DELETE, 
    KEY_HOME, 
    KEY_END,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_MOUSE_LEFT,
    KEY_MOUSE_RIGHT,
    KEY_COUNT
};  

inline bool 
is_mouse_keycode(u32 keycode) {
    return KEY_MOUSE_LEFT <= keycode && keycode <= KEY_MOUSE_RIGHT;
}

// A way of platform layer communcating with game.
// In the begging of the frame platform layer supplies gaem with all information about user input 
// it needs, during the frame game can modify some values in this struct to make commands to the 
// platform layer, like go fullscreen or switch vsync
// The idea of this structure is not to provide api, but simply pass data between modules
struct PlatformInterface {
    vec2 display_size;
    vec2 mpos;
    vec2 mdelta;
    f32 mwheel; 
    bool is_keys_down       [KEY_COUNT];
    u8 keys_transition_count[KEY_COUNT];
    f32 frame_dt;
    u32 utf32;
    bool is_quit_requested;
    bool window_size_changed;
    
    i16 *sound_samples;
    u64 sample_count_to_output;
    u64 samples_per_second;
    // Settings that can be changed
    bool fullscreen;
    bool vsync;
};

inline void 
update_key_state(PlatformInterface *input, KeyKind key, bool new_down) {
    input->keys_transition_count[key] += TO_BOOL(input->is_keys_down[key] != new_down);
    input->is_keys_down[key] = new_down;
}
