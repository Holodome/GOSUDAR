#if !defined(OS_H)

#include "lib/lib.hh"

struct Renderer;
struct OSInternal;

enum struct Key {
    None = 0x0,
    W,
    A,
    S,
    D,
    Shift,
    Ctrl,
    Space,
    MouseLeft,
    MouseRight,
    Count
};

struct KeyState {
    bool is_down;
    int transition_count;  

    void update(bool is_down) {
        if (this->is_down != is_down) {
            this->is_down = is_down;
            ++transition_count;
        }
    }
};

struct Input {
    Vec2 winsize;
    Vec2 mpos;
    Vec2 mdelta;
    f32 mwheel;  
    KeyState keys[(u32)Key::Count];
    f32 time, dt;
    bool is_quit_requested;
    
    f32 lmc_click_time;
    bool lmc_doubleclick;
    f32 keys_down_time[(u32)Key::Count];
    
    bool is_key_pressed(Key key) {
        KeyState *k = keys + (uintptr_t)key;
        return k->is_down && k->transition_count;
    }
    
    bool is_key_held(Key key) {
        KeyState *k = keys + (uintptr_t)key;
        return k->is_down;
    }
    
    bool is_key_held_te(Key key, f32 repeat_rate, f32 repeat_delay) {
        u32 ku = (u32)key;
        bool result = false;
        f32 t = keys_down_time[ku];
        if (t == 0) {
            result = true;
        } else {
            if (t > repeat_delay) {
                f32 press_mod = fmodf(t - repeat_delay, repeat_rate);
                f32 abs_mod = fmodf(t - repeat_delay - dt, repeat_rate);
                if ((press_mod > repeat_rate * 0.5f) != (abs_mod > repeat_rate * 0.5f)) {
                    result = true;
                }
            }
        }
        
        return result;
    }
    
    // Called after platform specific update
    void update() {
        for (u32 i = 0; i < (u32)Key::Count; ++i) {
            if (is_key_held((Key)i)) {
                if (keys_down_time[i] < 0) {
                    keys_down_time[i] = 0;
                } else {
                    keys_down_time[i] += dt;
                }
            } else {
                keys_down_time[i] = -1;
            }
        }    
        
        lmc_doubleclick = false;
        if (is_key_pressed(Key::MouseLeft)) {
            if (time - lmc_click_time < 0.30f) {
                lmc_doubleclick = true;
                lmc_click_time = 0;
            } else {
                lmc_click_time = time;
            }
        }
    }
};

struct OS {
    // Platform-specific data
    OSInternal *internal;
    
    // Initializes window
    void init();
    // Loads all opengl functions
    void init_renderer_backend();
    // Sets up time counting
    void prepare_to_start();
    void update_input(Input *input);
    void update_window();
};

#define OS_H 1
#endif
