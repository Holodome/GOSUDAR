#if !defined(DEBUG_HH)

#include "general.hh"

#include <intrin.h>

#if INTERNAL_BUILD

#define DEBUG_MAX_EVENT_ARRAY_COUNT 2
#define DEBUG_MAX_FRAME_COUNT 4
#define DEBUG_MAX_EVENT_COUNT (1 << 16)
#define DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME 128
CT_ASSERT(IS_POW2(DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME));

#define DEBUG_VALUE_TYPE_LIST() \
DEBUG_VALUE_TYPE(u8)            \
DEBUG_VALUE_TYPE(u16)           \
DEBUG_VALUE_TYPE(u32)           \
DEBUG_VALUE_TYPE(u64)           \
DEBUG_VALUE_TYPE(i8)            \
DEBUG_VALUE_TYPE(i16)           \
DEBUG_VALUE_TYPE(i32)           \
DEBUG_VALUE_TYPE(i64)           \
DEBUG_VALUE_TYPE(vec2)          \
DEBUG_VALUE_TYPE(vec3)          \
DEBUG_VALUE_TYPE(vec4)          \
DEBUG_VALUE_TYPE(bool)          \
DEBUG_VALUE_TYPE(f32)           \
DEBUG_VALUE_TYPE(f64)           \

enum {
    DEBUG_EVENT_NONE,
    DEBUG_EVENT_FRAME_MARKER,
    DEBUG_EVENT_BEGIN_BLOCK,
    DEBUG_EVENT_END_BLOCK,
    DEBUG_EVENT_BEGIN_VALUE_BLOCK,
    DEBUG_EVENT_END_VALUE_BLOCK,
    DEBUG_EVENT_ARENA_NAME,
    DEBUG_EVENT_ARENA_SUPRESS,
    DEBUG_EVENT_ARENA_BLOCK_ALLOCATE,
    DEBUG_EVENT_ARENA_BLOCK_TRUNCATE,
    DEBUG_EVENT_ARENA_BLOCK_FREE,
    DEBUG_EVENT_ARENA_ALLOCATE,
    DEBUG_EVENT_VALUE_SWITCH,
    DEBUG_EVENT_VALUE_DRAG,
#define DEBUG_VALUE_TYPE(_name) DEBUG_EVENT_VALUE_##_name,
    DEBUG_VALUE_TYPE_LIST()
#undef DEBUG_VALUE_TYPE
};

struct DebugMemoryOP {
    struct MemoryBlock *block;
    struct MemoryBlock *arena_lookup_block;
    u32 allocated_size;
    u32 offset_in_block;
};

struct DebugEvent {
    u8 type;          // DebugEventType
    u64 clock;        // rdstc
    const char *debug_name; // see DEBUG_NAME
    const char *name;       // user-defined block name
    union {
        DebugMemoryOP mem_op;
        bool *value_switch;
        f32 *value_drag;
#define DEBUG_VALUE_TYPE(_name) _name value_##_name;
        DEBUG_VALUE_TYPE_LIST()
#undef DEBUG_VALUE_TYPE
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
#define DEBUG_VALUE_TYPE DEBUG_VALUE_PROC_DEF
DEBUG_VALUE_TYPE_LIST()
#undef DEBUG_VALUE_TYPE
#undef DEBUG_VALUE_PROC_DEF
#define DEBUG_VALUE(_value, _name) DEBUG_VALUE_(DEBUG_NAME(), _name, _value)
#define DEBUG_SWITCH(_value, _name)\
do { RECORD_DEBUG_EVENT_INTERNAL(DEBUG_EVENT_VALUE_SWITCH, DEBUG_NAME(), _name);\
event->value_switch = _value; \
} while (0);
#define DEBUG_DRAG(_value, _name) \
do { RECORD_DEBUG_EVENT_INTERNAL(DEBUG_EVENT_VALUE_DRAG, DEBUG_NAME(), _name); \
event->value_drag = _value;\
} while (0);
#define DEBUG_BEGIN_VALUE_BLOCK_(_debug_name, _name) RECORD_DEBUG_EVENT(DEBUG_EVENT_BEGIN_VALUE_BLOCK, _debug_name, _name)
#define DEBUG_BEGIN_VALUE_BLOCK(_name) DEBUG_BEGIN_VALUE_BLOCK_(DEBUG_NAME(), _name)
#define DEBUG_END_VALUE_BLOCK()   RECORD_DEBUG_EVENT(DEBUG_EVENT_END_VALUE_BLOCK, DEBUG_NAME(), "#END_VALUE_BLOCK")
#define DEBUG_VALUE_BLOCK__(_debug_name, _name, _number) DebugValueBlockHelper __value_block__##_number(_debug_name, _name);
#define DEBUG_VALUE_BLOCK_(_debug_name, _name, _number) DEBUG_VALUE_BLOCK__(_debug_name, _name, _number)
#define DEBUG_VALUE_BLOCK(_name) DEBUG_VALUE_BLOCK_(DEBUG_NAME(), _name, __LINE__)
#define DEBUG_ARENA_NAME(_arena, _name) \
do { \
RECORD_DEBUG_EVENT_INTERNAL(DEBUG_EVENT_ARENA_NAME, DEBUG_NAME(), _name);\
event->mem_op.arena_lookup_block = (_arena)->current_block; \
} while(0);
#define DEBUG_ARENA_SUPRESS(_arena) \
do { \
RECORD_DEBUG_EVENT_INTERNAL(DEBUG_EVENT_ARENA_SUPRESS, DEBUG_NAME(), "Supress");\
event->mem_op.arena_lookup_block = (_arena)->current_block;\
} while (0);

#define DEBUG_ARENA_ALLOCATE(_debug_name, _block, _size, _block_offset) \
do { \
RECORD_DEBUG_EVENT_INTERNAL(DEBUG_EVENT_ARENA_ALLOCATE, _debug_name, "Allocate");\
event->mem_op.block = (_block); \
event->mem_op.allocated_size = (_size); \
event->mem_op.offset_in_block = (_block_offset);\
} while(0);

#define DEBUG_ARENA_BLOCK_ALLOCATE(_block) \
do { \
RECORD_DEBUG_EVENT_INTERNAL(DEBUG_EVENT_ARENA_BLOCK_ALLOCATE, DEBUG_NAME(), "BlockAllocate");\
event->mem_op.arena_lookup_block = (_block)->next;\
event->mem_op.block = (_block);\
event->mem_op.allocated_size = (_block)->size;\
} while(0);

#define DEBUG_ARENA_BLOCK_TRUNCATE(_block) \
do { \
RECORD_DEBUG_EVENT_INTERNAL(DEBUG_EVENT_ARENA_BLOCK_TRUNCATE, DEBUG_NAME(), "BlockTruncate");\
event->mem_op.block = (_block);\
event->mem_op.allocated_size = (_block)->used;\
} while(0);

#define DEBUG_ARENA_BLOCK_FREE(_block) \
do { \
RECORD_DEBUG_EVENT_INTERNAL(DEBUG_EVENT_ARENA_BLOCK_FREE, DEBUG_NAME(), "BlockFree");\
event->mem_op.block = (_block);\
} while(0);

struct DebugValueBlockHelper {
    DebugValueBlockHelper(const char *debug_name, const char *name) {
        DEBUG_BEGIN_VALUE_BLOCK_(debug_name, name);
    }
    
    ~DebugValueBlockHelper() {
        DEBUG_END_VALUE_BLOCK();
    }
};

struct DebugTimedBlock {
    DebugTimedBlock(const char *debug_name, const char *name) {
        BEGIN_BLOCK_(debug_name, name);
    }
    
    ~DebugTimedBlock() {
        END_BLOCK();
    }
};

// Debug system is defined in that way that game itself has no access to it - everything has to go
// through event system
struct DebugState;
DebugState *DEBUG_init();
void DEBUG_begin_frame(DebugState *debug_state);
void DEBUG_update(DebugState *debug_state, GameLinks links);
void DEBUG_frame_end(DebugState *debug_state);

#else 

struct DebugState;
#define DEBUG_init(...) 0
#define DEBUG_begin_frame(...) ((void)0)
#define DEBUG_update(...) ((void)0)
#define DEBUG_frame_end(...) ((void)0)

#define DEBUG_NAME()
#define TIMED_BLOCK(...)
#define BEGIN_BLOCK(...)
#define END_BLOCK(...)
#define TIMED_FUNCTION(...)
#define FRAME_MARKER(...)
#define DEBUG_VALUE(...)
#define DEBUG_SWITCH(...)
#define DEBUG_DRAG(...)
#define DEBUG_BEGIN_VALUE_BLOCK(...)
#define DEBUG_END_VALUE_BLOC(...)
#define DEBUG_VALUE_BLOCK(...)

#endif 

#define DEBUG_HH 1
#endif
