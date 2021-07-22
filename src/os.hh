#if !defined(OS_H)

#include "lib.hh"

#include "input.hh"
#include "mem.hh"

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
    bool no_errors;
    u8 storage[8];
};

struct OS;

// @CLEANUP do we really have to do this ugly display_size passing?
OS *os_init(vec2 *display_size);

void init_renderer_backend(OS *os);
Platform *os_begin_frame(OS *os);
void os_end_frame(OS *os);

RealWorldTime get_real_world_time();
// Virtual memory management
void *os_alloc(uptr size);
void os_free(void *ptr);
// Filesytem
FileHandle open_file(const char *name, bool read = true);
uptr get_file_size(FileHandle handle);
bool file_handle_valid(FileHandle handle);
void read_file(FileHandle handle, uptr offset, uptr size, void *dest);
void write_file(FileHandle handle, uptr offset, uptr size, const void *source);
void close_file(FileHandle handle);

void DEBUG_out_string(const char *format, ...);

void mkdir(const char *name);
void sleep(u32 ms);

//#define DEFUAL_ALLOC_FLAGS OS_BLOCK_ALLOC_BOUNDS_CHECK
//MemoryBlock *os_alloc_block(uptr size, u32 flags = DEFUAL_ALLOC_FLAGS);

#define OS_H 1
#endif
