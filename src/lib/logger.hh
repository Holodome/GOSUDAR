#if !defined(LOGGER_HH)

#include "lib/general.hh"
#include "lib/str.hh"

void logprintv(const char *tag, const char *format, va_list args) {
    static char buffer[4096];
    Str::format(buffer, sizeof(buffer), "[%s] %s", tag, format);
    vprintf(buffer, args);
}

void logprint(const char *tag, const char *format, ...) {
    va_list args;
    va_start(args, format);
    logprintv(tag, format, args);
    va_end(args);
}

void print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

#define LOGGER_HH 1
#endif
