#if !defined(INPUT_HH)

#include "lib.hh"

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

// A way of platform layer communcating with game.
// In the begging of the frame platform layer supplies gaem with all information about user input 
// it needs, during the frame game can modify some values in this struct to make commands to the 
// platform layer, like go fullscreen or switch vsync
// The idea of this structure is not to provide api, but simply pass data between modules
struct Platform {
    Vec2 winsize;
    Vec2 mpos;
    Vec2 mdelta;
    f32 mwheel; 
    bool is_keys_down[KEY_COUNT];
    u8 keys_transition_count[KEY_COUNT];
    f32 frame_dt;
    u32 utf32;
    bool is_quit_requested;
    
    i16 *sound_samples;
    u64 sample_count_to_output;
    u64 samples_per_second;
    // Settings that can be changed
    bool vsync;
    bool fullscreen;
};

inline void update_key_state(Platform *input, u32 key, bool new_down) {
    input->keys_transition_count[key] += (bool)(input->is_keys_down[key] != new_down);
    input->is_keys_down[key] = new_down;
}

enum {
    INPUT_ACCESS_TOKEN_NO_LOCK, 
    INPUT_ACCESS_TOKEN_GAME_INTERFACE,  
    INPUT_ACCESS_TOKEN_GAME_MENU,  
    INPUT_ACCESS_TOKEN_DEV_UI,  
    INPUT_ACCESS_TOKEN_ALL,  
};

// Wrapper for input locking in case some game sections overlap
// So when game detects that dev ui is focused, it locks input for it
// Access tokens are ordered by priority
// For example, game may set lock for interface after updating, but dev ui is actually focused
// Then we let interface use input this frame, but set more high priority dev ui to be locked
// This actually allows single frame input delay, but due to imm nature of dev ui this is hard to 
// do different. And usually there will be only maximum of two levels of input anyway, so 
// this delay does not occure in game circumstances, and can be easilly avoided in other cases, 
// for example setting lock in the begging of the frame
struct InputManager {
    Platform *input;
    u32 access_token;
};

inline InputManager create_input_manager(Platform *input) {
    InputManager result;
    result.input = input;
    result.access_token = INPUT_ACCESS_TOKEN_NO_LOCK;
    return result;
}

void lock_input(InputManager *manager, u32 access_token) {
    assert(!manager->access_token);
    manager->access_token = access_token;
}

void unlock_input(InputManager *manager) {
    assert(manager->access_token);
    manager->access_token = INPUT_ACCESS_TOKEN_NO_LOCK;
}

bool is_key_pressed(InputManager *manager, u32 key, u32 access_token) {
    bool result = false;
    if (manager->access_token == access_token || access_token == INPUT_ACCESS_TOKEN_ALL) {
        result = manager->input->is_keys_down[key] && manager->input->keys_transition_count[key];
    }
    return result;
}

bool is_key_released(InputManager *manager, u32 key, u32 access_token) {
    bool result = false;
    if (manager->access_token == access_token || access_token == INPUT_ACCESS_TOKEN_ALL) {
        result = !manager->input->is_keys_down[key] && manager->input->keys_transition_count[key];
    }
    return result;
}

bool is_key_held(InputManager *manager, u32 key, u32 access_token) {
    bool result = false;
    if (manager->access_token == access_token || access_token == INPUT_ACCESS_TOKEN_ALL) {
        result = manager->input->is_keys_down[key];
    }
    return result;
}

f32 get_mwheel(InputManager *manager) {
    return manager->input->mwheel;
}

Vec2 mouse_p(InputManager *manager) {
    return manager->input->mpos;
}

Vec2 mouse_d(InputManager *manager) {
    return manager->input->mdelta;
}

Vec2 window_size(InputManager *manager) {
    return manager->input->winsize;   
}

f32 get_dt(InputManager *manager) {
    return manager->input->frame_dt;
}

#define INPUT_HH 1
#endif
