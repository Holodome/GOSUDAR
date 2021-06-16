#include "framework/profiler.hh"
#include "framework/os.hh"

#if ENABLE_PROFILER

static FileHandle profile_file_handle = {};
static size_t profile_file_idx = 0;

void profiler_init() {
    char buffer[64];
    RealWorldTime time = OS::get_real_world_time();
    Str::format(buffer, sizeof(buffer), "logs/%u.%u.%u.%u.%u.%u.prof", time.year, time.month, time.day, time.hour, time.second, time.millisecond);
    os->mkdir("logs");
    profile_file_handle = OS::open_file(buffer, false);
    assert(OS::is_file_handle_valid(profile_file_handle));
    
    u64 start = __rdtscp(&____rdtscp_v);
    OS::sleep(100);
    u64 end = __rdtscp(&____rdtscp_v);
    u64 clocks_in_second = (end - start) * 10;
    OS::write_file(profile_file_handle, 0, sizeof(clocks_in_second), &clocks_in_second);
    profile_file_idx += sizeof(clocks_in_second);
}

void profiler_cleanup() {
    OS::close_file(profile_file_handle);
}

void profiler_record_event(ProfilerEvent event) {
    u8 buffer[4096];
    size_t cursor = 0;
    memcpy(buffer, &event.kind, sizeof(event.kind));
    cursor += sizeof(event.kind);
    memcpy(buffer + cursor, &event.clock, sizeof(event.clock));
    cursor += sizeof(event.clock);
    size_t debug_name_len = strlen(event.debug_name) + 1;
    memcpy(buffer + cursor, event.debug_name, debug_name_len);
    cursor += debug_name_len;
    size_t name_len = strlen(event.debug_name) + 1;
    memcpy(buffer + cursor, event.name, name_len);
    cursor += name_len;
    OS::write_file(profile_file_handle, profile_file_idx, cursor, buffer);
    profile_file_idx += cursor;
}

#endif