/*
Author: Holodome
Date: 04.10.2021
File: engine/platform/osx/osx.h
Version: 0
*/
#pragma once
#include "lib/general.h"

#include "platform/os.h"
#include "platform/window.h"

// @CLEANUP This can be removed from here and put into some osx_internal.h file
// .m files interface
// Not to mess up with file build configurations, all objective-c functions are put 
// into separate file and are interfaced via opaque pointers
void osx_create_window_internal(Window_State *state, u32 width, u32 height);
void osx_poll_window_events_internal(Window_State *state);
u32 osx_scancode_to_key(u32 scancode);