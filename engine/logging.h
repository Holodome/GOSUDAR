/*
Author: Holodome
Date: 03.10.2021
File: src/lib/logging.hh
Version: 0
*/
#pragma once 
#include "lib/general.h"

struct Logging_State;

struct Logging_State *create_logging_state(const char *filename);
void init_logging(struct Logging_State *state);
void shutdown_logging(struct Logging_State *state);

ENGINE_PUB void log_debugv(const char *msg, va_list args);
ATTR((__format__ (__printf__, 1, 2)))
ENGINE_PUB void log_debug(const char *msg, ...);

ENGINE_PUB void log_infov(const char *msg, va_list args);
ATTR((__format__ (__printf__, 1, 2)))
ENGINE_PUB void log_info(const char *msg, ...);

ENGINE_PUB void log_warnv(const char *msg, va_list args);
ATTR((__format__ (__printf__, 1, 2)))
ENGINE_PUB void log_warn(const char *msg, ...);

ENGINE_PUB void log_errorv(const char *msg, va_list args);
ATTR((__format__ (__printf__, 1, 2)))
ENGINE_PUB void log_error(const char *msg, ...);

