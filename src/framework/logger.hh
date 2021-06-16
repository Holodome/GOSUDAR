#if !defined(LOGGER_HH)

#include "lib/general.hh"
#include "lib/str.hh"

void logger_init();
void logger_cleanup();

void logprintv(const char *tag, const char *format, va_list args);
void logprint(const char *tag, const char *format, ...);
void print(const char *format, ...);
void logprintln(const char *tag, const char *format, ...);

#define LOGGER_HH 1
#endif
