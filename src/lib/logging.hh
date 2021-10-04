/*
Author: Holodome
Date: 03.10.2021
File: src/lib/logging.hh
Version: 0
*/
#pragma once 
#include "lib/general.hh"

void init_logging(const char *filename);
void shutdown_logging();

void log_debugv(const char *msg, va_list args);
ATTR((__format__ (__printf__, 1, 2)))
void log_debug(const char *msg, ...);

void log_infov(const char *msg, va_list args);
ATTR((__format__ (__printf__, 1, 2)))
void log_info(const char *msg, ...);

void log_warnv(const char *msg, va_list args);
ATTR((__format__ (__printf__, 1, 2)))
void log_warn(const char *msg, ...);

void log_errorv(const char *msg, va_list args);
ATTR((__format__ (__printf__, 1, 2)))
void log_error(const char *msg, ...);

