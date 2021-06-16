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
    Enter, 
    Escape,
    
    Backspace,
    Delete, 
    Home, 
    End,
    Left,
    Right,
    
    MouseLeft,
    MouseRight,
    
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    
    Count
};

struct KeyState {
    bool is_down = false;
    int transition_count = 0;  

    void update(bool is_down) {
        if (this->is_down != is_down) {
            this->is_down = is_down;
            ++transition_count;
        }
    }
};

struct Input {
    Vec2 winsize = Vec2(0);
    Vec2 mpos = Vec2(0);
    Vec2 mdelta = Vec2(0);
    f32 mwheel = 0;  
    KeyState keys[(u32)Key::Count] = {};
    f32 time = 0, dt = 0;
    bool is_quit_requested = false;
    
    f32 lmc_click_time = 0;
    bool lmc_doubleclick = false;
    f32 keys_down_time[(u32)Key::Count] = {};
    u32 utf32 = 0;
    
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
    OSInternal *internal = 0;
    
    // Initializes window
    void init();
    void cleanup();
    
    f32 get_time() const;
    
    // Loads all opengl functions
    void init_renderer_backend();
    void update_input(Input *input);
    void update_window();
    
    void go_fullscreen(bool fullscreen);
};

#define OS_H 1
#endif
