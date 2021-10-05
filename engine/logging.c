#include "logging.h"
#include "lib/stream.h"

#include "filesystem.h"

// @TODO(hl): Replace this 
#include <time.h> // localtime

#define LOGGING_BUFFER_SIZE KB(16)
#define LOGGING_BUFFER_THRESHOLD KB(4)

#define ANSI_COLOR_NONE   "\033[0m"
#define ANSI_COLOR_RED    "\033[31m"
#define ANSI_COLOR_YELLOW "\033[33m"
#define ANSI_COLOR_GREEN  "\033[32m"
#define ANSI_COLOR_BLUE   "\033[34m"

enum {
    LOG_COLOR_NONE,
    LOG_COLOR_RED,
    LOG_COLOR_YELLOW,
    LOG_COLOR_GREEN,
    LOG_COLOR_BLUE,
};

const char *LOG_COLOR_STRS[] = {
    ANSI_COLOR_NONE,
    ANSI_COLOR_RED,
    ANSI_COLOR_YELLOW,
    ANSI_COLOR_GREEN,
    ANSI_COLOR_BLUE
};

#define ASSERT_INITIALIZED assert(state && state->is_initialized)

typedef struct Logging_State {
    bool is_initialized;
    
    File_ID log_file_id;
    OutStream log_stream;
    u32 current_color;
} Logging_State;  

static Logging_State *state;

// @NOTE(hl): These are used to duplcate output to console while also writing to the file
static void 
log_outputv(const char *msg, va_list args) {
    ASSERT_INITIALIZED;
    out_streamv(&state->log_stream, msg, args);
    OutStream *stdout = get_stdout_stream();
    out_streamv(stdout, msg, args);
    // @TODO(hl): Option for flushing on specific log levels
    out_stream_flush(stdout); 
}

static void 
log_outputf(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    log_outputv(msg, args);
}

static void 
log_set_color(u32 color) {
    state->current_color = color;
    out_streamf(get_stdout_stream(), "%s", LOG_COLOR_STRS[state->current_color]);
}

static void 
log_time() {
    time_t t = time(0);
    struct tm current_time = *localtime(&t);
    log_outputf("%d-%02d-%02d %02d:%02d:%02d", 
        current_time.tm_year + 1900, current_time.tm_mon + 1, current_time.tm_mday,
        current_time.tm_hour, current_time.tm_min, current_time.tm_sec);
}

struct Logging_State *
create_logging_state(const char *filename) {
    // @LEAK
    Logging_State *state_local = mem_alloc(sizeof(Logging_State));
    state_local->is_initialized = true;
    state_local->log_file_id = fs_open_file(filename, FILE_MODE_WRITE);
    init_out_streamf(&state_local->log_stream, fs_get_handle(state_local->log_file_id), 
        mem_alloc(LOGGING_BUFFER_SIZE), LOGGING_BUFFER_SIZE,
        LOGGING_BUFFER_THRESHOLD);
    init_logging(state_local);
    return state;
}

void 
init_logging(struct Logging_State *state_init) {
    state = state_init;
}

void shutdown_logging() {
    ASSERT_INITIALIZED;
    out_stream_flush(&state->log_stream);
    mem_free(state->log_stream.bf, state->log_stream.bf_sz);
}

void 
log_debugv(const char *msg, va_list args) {
    ASSERT_INITIALIZED;
   
    log_set_color(LOG_COLOR_GREEN);
    log_time();
    log_outputf(" DEBUG ");
    log_outputv(msg, args);
    log_outputf("\n");
    log_set_color(LOG_COLOR_NONE);
}

void 
log_debug(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    log_debugv(msg, args);
}

void 
log_infov(const char *msg, va_list args) {
    ASSERT_INITIALIZED;
   
    log_set_color(LOG_COLOR_YELLOW);
    log_time();
    log_outputf(" INFO ");
    log_outputv(msg, args);
    log_outputf("\n");
    log_set_color(LOG_COLOR_NONE);
}

void 
log_info(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    log_infov(msg, args);
}

void 
log_warnv(const char *msg, va_list args) {
    ASSERT_INITIALIZED;
   
    log_set_color(LOG_COLOR_BLUE);
    log_time();
    log_outputf(" WARN ");
    log_outputv(msg, args);
    log_outputf("\n");
    log_set_color(LOG_COLOR_NONE);
}

void 
log_warn(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    log_warnv(msg, args);
}

void 
log_errorv(const char *msg, va_list args) {
    ASSERT_INITIALIZED;
   
    log_set_color(LOG_COLOR_RED);
    log_time();
    log_outputf(" ERROR ");
    log_outputv(msg, args);
    log_outputf("\n");
    log_set_color(LOG_COLOR_NONE);
}

void 
log_error(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    log_errorv(msg, args);
}
