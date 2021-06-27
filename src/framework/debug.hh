#if !defined(DEBUG_HH)

#include "lib/general.hh"
#include "lib/math.hh"
#include "lib/memory_arena.hh"

#include <intrin.h>

#define DEBUG_MAX_EVENT_ARRAY_COUNT 2
#define DEBUG_MAX_FRAME_COUNT 4
#define DEBUG_MAX_EVENT_COUNT 65536
#define DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME 128
CT_ASSERT(IS_POW2(DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME));

enum {
    DEBUG_EVENT_NONE,
    DEBUG_EVENT_FRAME_MARKER,
    DEBUG_EVENT_BEGIN_BLOCK,
    DEBUG_EVENT_END_BLOCK
};

struct DebugEvent {
    u8 type;          // DebugEventType
    u64 clock;        // rdstc
    const char *debug_name; // see DEBUG_NAME
    const char *name;       // user-defined block name
};

struct DebugTable {
    u32 current_event_array_index;
    volatile u64 event_array_index_event_index; // (array_index << 32 | event_index)
    
    u32 event_counts [DEBUG_MAX_EVENT_ARRAY_COUNT];
    DebugEvent events[DEBUG_MAX_EVENT_ARRAY_COUNT][DEBUG_MAX_EVENT_COUNT];
};

extern DebugTable *debug_table;

#define DEBUG_NAME__(a, b, c) a "|" #b "|" #c
#define DEBUG_NAME_(a, b, c) DEBUG_NAME__(a, b, c)
#define DEBUG_NAME(name) DEBUG_NAME_(__FILE__, __LINE__, __COUNTER__)

// @TODO(hl): Can actualy replace interlocked_add with interlocked_increment
#define RecordDebugEvent(event_type, debug_name_init, name_init)                                  \
    do                                                                                            \
    {                                                                                             \
        u64 array_index_event_index = debug_table->event_array_index_event_index++;               \
        u32 event_index = array_index_event_index & 0xFFFFFFFF;                                   \
        assert(event_index < ARRAY_SIZE(debug_table->events[0]));                                 \
        DebugEvent *event = debug_table->events[array_index_event_index >> 32] + event_index;     \
        event->clock = __rdtsc();                                                                 \
        event->type = (u8)event_type;                                                             \
        event->debug_name = debug_name_init;                                                      \
        event->name = name_init;                                                                  \
    } while (0)

#define TIMED_BLOCK__(debug_name, name, number) DebugTimedBlock __timed_block_##number(debug_name, name)
#define TIMED_BLOCK_(debug_name, name, number) TIMED_BLOCK__(debug_name, name, number)
#define TIMED_BLOCK(name) TIMED_BLOCK_(DEBUG_NAME(name), name, __LINE__)

#define BEGIN_BLOCK_(debug_name, name) RecordDebugEvent(DEBUG_EVENT_BEGIN_BLOCK, debug_name, name)
#define BEGIN_BLOCK(name) BEGIN_BLOCK_(DEBUG_NAME(name), name)
#define END_BLOCK_(debug_name, name) RecordDebugEvent(DEBUG_EVENT_END_BLOCK, debug_name, name)
#define END_BLOCK() END_BLOCK_(DEBUG_NAME("#END_BLOCK"), "#END_BLOCK")
// @NOTE(hl): Cast to  char * beacuse clang has trobules with implicit cast from const char [] to char *
#define TIMED_FUNCTION() TIMED_BLOCK((char *)__FUNCTION__)

#define FRAME_MARKER() RecordDebugEvent(DEBUG_EVENT_FRAME_MARKER, DEBUG_NAME("#FRAME_MARKER"), "#FRAME_MARKER")

// This is a way of wrapping timed block into a struct, so we don't have to create it and destroy manually.
// when struct is created, construct is called - block is started
// struct goes out of scope - destructor is called - block is ended
struct DebugTimedBlock {
    DebugTimedBlock(char *debug_name, char *name) {
        BEGIN_BLOCK_(debug_name, name);
    }

    ~DebugTimedBlock() {
        END_BLOCK();
    }
};

struct DebugFrameRegion {
    f32 time_min;
    f32 time_max;
    const char *debug_name; // see DEBUG_NAME
    const char *name;       // user-defined block name
};

struct DebugRecord {
    const char *debug_name;
    const char *name;
    u32 times_called;
    u64 total_clocks;
};  

struct DebugRecordHash {
    u32 debug_name_hash;
    u32 index;
};

struct DebugFrame {
    u64 collation_clocks;
    u64 begin_clock;
    u64 end_clock;
    
    u32 records_count;
    DebugRecord records[DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME];
    DebugRecordHash records_hash[DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME];
};

struct DebugOpenBlock {
    u32 frame_index;
    DebugEvent *opening_event;
    DebugOpenBlock *parent;

    DebugOpenBlock *next_free;
};

struct DebugState {
    MemoryArena collate_arena;

    DebugTable debug_table;
    
    u32 frame_index;
    DebugFrame frames[DEBUG_MAX_FRAME_COUNT];
    DebugOpenBlock *current_open_block;
        
    DebugOpenBlock *first_free_block;
    u32 collation_array_index;
    bool is_paused;
};

DebugState *debug_init();
void debug_frame_end(DebugState *debug_state);

#define DEBUG_HH 1
#endif
