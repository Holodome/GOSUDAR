/*
Author: Holodome
Date: 04.10.2021
File: engine/platform/osx/osx.h
Version: 0
*/
#pragma once
#include "general.h"

#include "platform/os.h"
#include "platform/window.h"

OS_OPEN_FILE(osx_open_file);
OS_CLOSE_FILE(osx_close_file);
OS_WRITE_FILE(osx_write_file);
OS_READ_FILE(osx_read_file);
OS_GET_FILE_SIZE(osx_get_file_size);
OS_WRITE_STDOUT(osx_write_stdout);
OS_WRITE_STDERR(osx_write_stderr);

struct OSX_Window_State_Internal;

// high-level interface
void osx_create_window(Window_State *state, u32 width, u32 height);
void osx_poll_window_events(Window_State *state);

// @CLEANUP This can be removed from here and put into some osx_internal.h file
// .m files interface
// Not to mess up with file build configurations, all objective-c functions are put 
// into separate file and are interfaced via opaque pointers
void osx_create_window_internal(Window_State *state, u32 width, u32 height);
void osx_poll_window_events_internal(Window_State *state);
u32 osx_scancode_to_key(u32 scancode);