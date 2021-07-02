
static FileHandle log_file_handle = {};
static size_t log_file_index = 0;

void logger_init() {
    char buffer[64];
    RealWorldTime time = get_real_world_time();
    snprintf(buffer, sizeof(buffer), "logs/%u.%u.%u.%u.%u.%u.log", time.year, time.month, time.day, time.hour, time.second, time.millisecond);
    mkdir("logs");
    log_file_handle = open_file(buffer, false);
    assert(file_handle_valid(log_file_handle));
}

void logger_cleanup() {
    close_file(log_file_handle);
}

void printv(const char *format, va_list args) {
    static char buffer[4096];
    size_t len = vsnprintf(buffer, sizeof(buffer), format, args);
    vprintf(buffer, args);
    write_file(log_file_handle, log_file_index, len, buffer);
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