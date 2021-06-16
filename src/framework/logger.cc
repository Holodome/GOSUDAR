#include "framework/logger.hh"
#include "framework/os.hh"

static FileHandle log_file_handle = {};
static size_t log_file_index = 0;

void logger_init() {
    char buffer[64];
    RealWorldTime time = OS::get_real_world_time();
    Str::format(buffer, sizeof(buffer), "logs/exec%u.%u.%u.%u.%u.%u.log", time.year, time.month, time.day, time.hour, time.second, time.millisecond);
    os->mkdir("logs");
    log_file_handle = OS::open_file(buffer, false);
    assert(OS::is_file_handle_valid(log_file_handle));
}

void logger_cleanup() {
    OS::close_file(log_file_handle);
}

void printv(const char *format, va_list args) {
    static char buffer[4096];
    size_t len = Str::formatv(buffer, sizeof(buffer), format, args);
    vprintf(buffer, args);
    OS::write_file(log_file_handle, log_file_index, len, buffer);
    log_file_index += len;
}

void print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printv(format, args);
    va_end(args);
}

void logprintv(const char *tag, const char *format, va_list args) {
    print("[%s] ", tag);
    printv(format, args);
}

void logprint(const char *tag, const char *format, ...) {
    va_list args;
    va_start(args, format);
    logprintv(tag, format, args);
    va_end(args);
}

void logprintln(const char *tag, const char *format, ...) {
    va_list args;
    va_start(args, format);
    logprintv(tag, format, args);
    va_end(args);
    print("\n");
}