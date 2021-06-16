#if !defined(PROFILER_HH)

#include "lib/general.hh"
// __rdtscp
#include <intrin.h>

enum ProfilerEventKind {
    ProfilerEventKind_None        = 0x0,
    ProfilerEventKind_GameStart   = 0x1,
    ProfilerEventKind_FrameMarker = 0x2,
    ProfilerEventKind_BeginBlock  = 0x3,
    ProfilerEventKind_EndBlock    = 0x4,
    ProfilerEventKind_GameEnd     = 0x5,
};

struct ProfilerEvent {
    u8 kind; // ProfilerEventKind
    u64 clock;
    const char *debug_name;
    const char *name;
};

#define ENABLE_PROFILER 0
#if ENABLE_PROFILER

#define PROFILER_NAME__(a, b, c) a "|" #b "|" #c
#define PROFILER_NAME_(a, b, c) PROFILER_NAME__(a, b, c)
#define PROFILER_NAME PROFILER_NAME_(__FILE__, __LINE__, __COUNTER__)
#define PROFILER_EVENT(_event_type, _debug_name_init, _name_init) \
    do {                                                          \
        ProfilerEvent _event;                                     \
        _event.kind = _event_type;                                \
        _event.clock = __rdtscp(&____rdtscp_v);                   \
        _event.debug_name = _debug_name_init;                     \
        _event.name = _name_init;                                 \
        profiler_record_event(_event);                            \
    } while(0);
#define BEGIN_BLOCK_(_debug_name, _name) PROFILER_EVENT(ProfilerEventKind_BeginBlock, _debug_name, _name)
#define BEGIN_BLOCK(_name) BEGIN_BLOCK_(PROFILER_NAME, name)
#define END_BLOCK_(_debug_name, _name) PROFILER_EVENT(ProfilerEventKind_EndBlock, _debug_name, _name)
#define END_BLOCK() END_BLOCK_(PROFILER_NAME, "#END_BLOCK")

#define TIMED_BLOCK__(_debug_name, _name, _number) ProfilerTimedBlock __timed_block_##_number(_debug_name, _name)
#define TIMED_BLOCK_(_debug_name, _name, _number) TIMED_BLOCK__(_debug_name, _name, _number)
#define TIMED_BLOCK(_name) TIMED_BLOCK_(PROFILER_NAME, _name, __LINE__)

#define TIMED_FUNCTION TIMED_BLOCK((const char *)__FUNCTION__)
#define FRAME_MARKER PROFILER_EVENT(ProfilerEventKind_FrameMarker, PROFILER_NAME, "#FRAME_MARKER")
#define GAME_START PROFILER_EVENT(ProfilerEventKind_GameStart, PROFILER_NAME, "#GAME_START")
#define GAME_END PROFILER_EVENT(ProfilerEventKind_GameEnd, PROFILER_NAME, "#GAME_END")

void profiler_init();
void profiler_cleanup();

u32 ____rdtscp_v;

void profiler_record_event(ProfilerEvent event);

struct ProfilerTimedBlock {
    ProfilerTimedBlock(const char *debug_name, const char *name) {
        BEGIN_BLOCK_(debug_name, name);
    }
    
    ~ProfilerTimedBlock() {
        END_BLOCK();
    }
};

#else 

#define PROFILER_NAME__(a, b, c)
#define PROFILER_NAME_(a, b, c)
#define PROFILER_NAME
#define PROFILER_EVENT(...)
#define BEGIN_BLOCK_(...)
#define BEGIN_BLOCK(...)
#define END_BLOCK_(...)
#define END_BLOCK(...)
#define TIMED_BLOCK__(...)
#define TIMED_BLOCK_(...)
#define TIMED_BLOCK(...)
#define TIMED_FUNCTION
#define FRAME_MARKER
#define GAME_START
#define GAME_END

#define profiler_init() ((void)0)
#define profiler_cleanup() ((void)0)
#define profiler_record_event(...) ((void)0)
#endif 

#define PROFILER_HH 1
#endif
