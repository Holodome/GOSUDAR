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
    bool no_errors;
    u8 storage[8];
};

enum {
    OS_MEMORY_BLOCK_FLAG_UNDERFLOW = 0x1,
    OS_MEMORY_BLOCK_FLAG_OVERFLOW  = 0x2,
};

struct OSMemoryBlock {
    u32 flags;
    u64 size;
    u64 used;
    u8 *base;
    OSMemoryBlock *prev;
};

struct OS;

// @CLEANUP do we really have to do this ugly display_size passing?
OS *os_init(vec2 *display_size);

void init_renderer_backend(OS *os);
Platform *os_begin_frame(OS *os);
void os_end_frame(OS *os);

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

void DEBUG_out_string(const char *format, ...);

void mkdir(const char *name);
void sleep(u32 ms);

#define OS_H 1
#endif
