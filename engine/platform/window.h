/*
Author: Holodome
Date: 03.10.2021
File: src/platform/input.h
Version: 0
*/
#pragma once
#include "lib/general.h"

#include "math/vec.h"

enum {
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

struct Vulkan_Ctx;

// A way of platform layer communcating with game.
// In the begging of the frame platform layer supplies gaem with all information about user input 
// it needs, during the frame game can modify some values in this struct to make commands to the 
// platform layer, like go fullscreen or switch vsync
// The idea of this structure is not to provide api, but simply pass data between modules
typedef struct Window_State {
    void *internal;
    
    Vec2 display_size;
    Vec2 mpos;
    Vec2 mdelta;
    f32 mwheel; 
    bool is_keys_down       [KEY_COUNT];
    u8 keys_transition_count[KEY_COUNT];
    f32 frame_dt;
    u32 utf32;
    bool is_quit_requested;
    bool window_size_changed;
    
    bool fullscreen;
    bool vsync;
} Window_State;

ENGINE_PUB void update_key_state(Window_State *input, u32 key, bool new_down);

ENGINE_PUB void create_window(Window_State *state, u32 width, u32 height);
ENGINE_PUB void poll_window_events(Window_State *state);
ENGINE_PUB void create_vulkan_surface(Window_State *state, struct Vulkan_Ctx *ctx);
