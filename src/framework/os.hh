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
    Vec2 winsize;
    Vec2 mpos;
    Vec2 mdelta;
    f32 mwheel; 
    KeyState keys[(u32)Key::Count];
    f32 time;
    f32 dt;
    bool is_quit_requested;
    
    u32 utf32;
    
    bool is_key_pressed(Key key) {
        KeyState *k = keys + (uintptr_t)key;
        return k->is_down && k->transition_count;
    }
    
    bool is_key_held(Key key) {
        KeyState *k = keys + (uintptr_t)key;
        return k->is_down;
    }
};

struct RealWorldTime {
    u32 year;
    u32 month;
    u32 day;
    u32 hour;
    u32 minute;
    u32 second;
    u32 millisecond;
};

struct FileHandle {
    u8 storage[8];
};

struct FileWritetime {
    u8 storage[8];  
};

struct OS {
    // Platform-specific data
    OSInternal *internal = 0;
    
    // Initializes window
    void init();
    void cleanup();
    
    
    // Loads all opengl functions
    void init_renderer_backend();
    void update_input(Input *input);
    void update_window();
    
    void go_fullscreen(bool fullscreen);
    
    f32 get_time() const;
    static RealWorldTime get_real_world_time();
    
    static void mkdir(const char *name);
    static void sleep(u32 ms);
    
    static FileHandle open_file(const char *name, bool read = true);
    static bool is_file_handle_valid(FileHandle handle);
    static size_t get_file_size(FileHandle handle);
    static void read_file(FileHandle handle, size_t offset, size_t size, void *dest);
    static void write_file(FileHandle handle, size_t offset, size_t size, const void *source);
    static void close_file(FileHandle handle);
    
    static FileWritetime get_file_write_time(const char *name);
    static bool file_write_time_cmp(FileWritetime a, FileWritetime b);
};

// Virtual memory management
void *os_alloc(size_t size);
void os_free(void *ptr);

#define OS_H 1
#endif
