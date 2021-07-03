#if !defined(DEBUG_HH)

#include "lib.hh"

#include "game/dev_ui.hh"

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
    DEBUG_EVENT_END_BLOCK,
    DEBUG_EVENT_BEGIN_VALUE_BLOCK,
    DEBUG_EVENT_END_VALUE_BLOCK,
    DEBUG_EVENT_VALUE_SWITCH,
    DEBUG_EVENT_VALUE_u64,
    DEBUG_EVENT_VALUE_f32,
    DEBUG_EVENT_VALUE_Vec2,
    DEBUG_EVENT_VALUE_Vec3,
    DEBUG_EVENT_VALUE_Vec2i,
};

struct DebugEvent {
    u8 type;          // DebugEventType
    u64 clock;        // rdstc
    const char *debug_name; // see DEBUG_NAME
    const char *name;       // user-defined block name
    union {
        bool *value_switch;
        u64 value_u64;
        f32 value_f32;
        Vec2 value_Vec2;
        Vec3 value_Vec3;
        Vec2i value_Vec2i;
    };
};

struct DebugTable {
    u32 current_event_array_index;
    u64 event_array_index_event_index; // (array_index << 32 | event_index)
    
    u32 event_counts [DEBUG_MAX_EVENT_ARRAY_COUNT];
    DebugEvent events[DEBUG_MAX_EVENT_ARRAY_COUNT][DEBUG_MAX_EVENT_COUNT];
};

extern DebugTable *debug_table;

#define DEBUG_NAME__(a, b, c) a "|" #b "|" #c
#define DEBUG_NAME_(a, b, c) DEBUG_NAME__(a, b, c)
#define DEBUG_NAME() DEBUG_NAME_(__FILE__, __LINE__, __COUNTER__)

// @TODO(hl): Can actualy replace interlocked_add with interlocked_increment
#define RECORD_DEBUG_EVENT_INTERNAL(event_type, debug_name_init, name_init)                   \
    u64 array_index_event_index = debug_table->event_array_index_event_index++;               \
    u32 event_index = array_index_event_index & 0xFFFFFFFF;                                   \
    assert(event_index < ARRAY_SIZE(debug_table->events[0]));                                 \
    DebugEvent *event = debug_table->events[array_index_event_index >> 32] + event_index;     \
    event->clock = __rdtsc();                                                                 \
    event->type = (u8)event_type;                                                             \
    event->debug_name = debug_name_init;                                                      \
    event->name = name_init;                                                                  
    
#define RECORD_DEBUG_EVENT(_event_type, _debug_name_init, _name_init)           \
    do {                                                                        \
        RECORD_DEBUG_EVENT_INTERNAL(_event_type, _debug_name_init, _name_init); \
    } while (0);

#define TIMED_BLOCK__(debug_name, name, number) DebugTimedBlock __timed_block_##number(debug_name, name)
#define TIMED_BLOCK_(debug_name, name, number) TIMED_BLOCK__(debug_name, name, number)
#define TIMED_BLOCK(name) TIMED_BLOCK_(DEBUG_NAME(), name, __LINE__)

#define BEGIN_BLOCK_(debug_name, name) RECORD_DEBUG_EVENT(DEBUG_EVENT_BEGIN_BLOCK, debug_name, name)
#define BEGIN_BLOCK(name) BEGIN_BLOCK_(DEBUG_NAME(), name)
#define END_BLOCK_(debug_name, name) RECORD_DEBUG_EVENT(DEBUG_EVENT_END_BLOCK, debug_name, name)
#define END_BLOCK() END_BLOCK_(DEBUG_NAME(), "#END_BLOCK")
#define TIMED_FUNCTION() TIMED_BLOCK((const char *)__FUNCTION__)

#define FRAME_MARKER() RECORD_DEBUG_EVENT(DEBUG_EVENT_FRAME_MARKER, DEBUG_NAME(), "#FRAME_MARKER")

#define DEBUG_VALUE_PROC_DEF(_type)                                                \
inline void DEBUG_VALUE_(const char *debug_name, const char *name, _type value) {  \
    RECORD_DEBUG_EVENT_INTERNAL(DEBUG_EVENT_VALUE_##_type, debug_name, name);      \
    event->value_##_type = value;                                                  \
}
DEBUG_VALUE_PROC_DEF(u64)
DEBUG_VALUE_PROC_DEF(f32)
DEBUG_VALUE_PROC_DEF(Vec2)
DEBUG_VALUE_PROC_DEF(Vec3)
DEBUG_VALUE_PROC_DEF(Vec2i)
#define DEBUG_VALUE(_value, _name) DEBUG_VALUE_(DEBUG_NAME(), _name, _value)
#define DEBUG_SWITCH(_value, _name) do { RECORD_DEBUG_EVENT_INTERNAL(DEBUG_EVENT_VALUE_SWITCH, DEBUG_NAME(), _name); event->value_switch = _value; } while (0);
#define DEBUG_BEGIN_VALUE_BLOCK_(_debug_name, _name) RECORD_DEBUG_EVENT(DEBUG_EVENT_BEGIN_VALUE_BLOCK, _debug_name, _name)
#define DEBUG_BEGIN_VALUE_BLOCK(_name) DEBUG_BEGIN_VALUE_BLOCK_(DEBUG_NAME(), _name)
#define DEBUG_END_VALUE_BLOCK()   RECORD_DEBUG_EVENT(DEBUG_EVENT_END_VALUE_BLOCK, DEBUG_NAME(), "#END_VALUE_BLOCK")
#define DEBUG_VALUE_BLOCK__(_debug_name, _name, _number) DebugValueBlockHelper __value_block__##_number(_debug_name, _name);
#define DEBUG_VALUE_BLOCK_(_debug_name, _name, _number) DEBUG_VALUE_BLOCK__(_debug_name, _name, _number)
#define DEBUG_VALUE_BLOCK(_name) DEBUG_VALUE_BLOCK_(DEBUG_NAME(), _name, __LINE__)

struct DebugValueBlockHelper {
    DebugValueBlockHelper(const char *debug_name, const char *name) {
        DEBUG_BEGIN_VALUE_BLOCK_(debug_name, name);
    }
    
    ~DebugValueBlockHelper() {
        DEBUG_END_VALUE_BLOCK();
    }
};

// This is a way of wrapping timed block into a struct, so we don't have to create it and destroy manually.
// when struct is created, construct is called - block is started
// struct goes out of scope - destructor is called - block is ended
struct DebugTimedBlock {
    DebugTimedBlock(const char *debug_name, const char *name) {
        BEGIN_BLOCK_(debug_name, name);
    }

    ~DebugTimedBlock() {
        END_BLOCK();
    }
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
    DebugRecord records         [DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME];
    DebugRecordHash records_hash[DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME];
};

struct DebugOpenBlock {
    u32 frame_index;
    DebugEvent *opening_event;
    DebugOpenBlock *parent;
    DebugOpenBlock *next_free;
};

enum {
    DEV_MODE_NONE,  
    DEV_MODE_INFO,  
    DEV_MODE_PROFILER,  
    DEV_MODE_MEMORY,  
    DEV_MODE_SENTINEL,  
};

enum {
    DEBUG_VALUE_NONE,  
    DEBUG_VALUE_SWITCH,  
    DEBUG_VALUE_u64,  
    DEBUG_VALUE_f32,  
    DEBUG_VALUE_Vec2,  
    DEBUG_VALUE_Vec2i, 
    DEBUG_VALUE_Vec3, 
};

struct DebugValue {
    const char *name;
    u32 value_kind;
    union {
        bool *value_switch;
        u64 value_u64;  
        f32 value_f32;
        Vec2 value_Vec2;
        Vec3 value_Vec3;
        Vec2i value_Vec2i;
    };
    DebugValue *next;
};  

#define DEBUG_VALUE_BLOCK_MAX_DEPTH 8

struct DebugValueBlock {
    const char *name;
    DebugValue *first_value;
    DebugValueBlock *first_child;
    DebugValueBlock *next;
};

struct DebugState {
    MemoryArena arena;

    DebugTable debug_table;
    
    u32 frame_index;
    DebugFrame frames[DEBUG_MAX_FRAME_COUNT];
    u64 debug_open_blocks_allocated;
    DebugOpenBlock *first_free_block;
    u32 collation_array_index;
    bool is_paused;
    
    u64 debug_values_allocated;
    DebugValue *first_free_value;
    // DebugValue *first_value;
    u64 value_blocks_allocated;
    DebugValueBlock *first_free_value_block;
    DebugValueBlock *global_value_block;
    
    u64 total_frame_count;
    
    DevUI dev_ui;
    u32 dev_mode; 
};

DebugState *DEBUG_init();
void DEBUG_begin_frame(DebugState *debug_state);
void DEBUG_update(DebugState *debug_state, struct InputManager *input, RendererCommands *commands, Assets *assets);
void DEBUG_frame_end(DebugState *debug_state);

#define DEBUG_HH 1
#endif
