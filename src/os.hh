#if !defined(OS_H)

#include "lib.hh"

#include "input.hh"

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
    bool errors;
    u8 storage[8];
};

struct OS;

OS *os_init();
void os_init_renderer_backend(OS *os);

void init_renderer_backend(OS *os);
Input *update_input(OS *os);
void update_window(OS *os);

void go_fullscreen(bool fullscreen);

RealWorldTime get_real_world_time();

// Virtual memory management
void *os_alloc(size_t size);
void os_free(void *ptr);
// Filesytem
FileHandle open_file(const char *name, bool read = true);
size_t get_file_size(FileHandle handle);
bool file_handle_valid(FileHandle handle);
void read_file(FileHandle handle, size_t offset, size_t size, void *dest);
void write_file(FileHandle handle, size_t offset, size_t size, const void *source);
void close_file(FileHandle handle);

void mkdir(const char *name);
void sleep(u32 ms);
    
#define OS_H 1
#endif
