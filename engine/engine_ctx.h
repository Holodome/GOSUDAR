/*
Author: Holodome
Date: 04.10.2021
File: engine/engine_ctx.h
Version: 0

This engine is designed to support fast iteration via allowing hot code reloading.
This is done by having the engine code contain main function,
which does all engine initilization stuff needed and calls game function from some game code.
Because of this approach, no static variables can be shared across modules.

There is no point in having ability to hot reload code if the compile times are slow.
Because of that, engine does everything it can to minimizs the number of recompilations needed
across modules.
For example, platform-specific code doesn't need to be compiled with game, as it can be 
put into function pointers and supplied via some structure.

That is why API is made in the way that minizies os calls in data-driven way.
For example, all frame data is contained in single structure and modules can communicate 
with each other using these modules.

Because a lot of platform-specific operations are slow by their nature (memory allocation,
file operations etc.), we don't loose much by calling functions by pointers.

While this approach can seem complicated, this allows code to be written in any manner possible 
on the game part. And if function pointer approach seems compilcated, it is always good to 
remember that there is only finite set of feeatures from platform that engine needs to access - 
and most of them can be expressed without function calls.
So, we try this design and see how it goes.

The main function is located deep inside the platform code. Thus, each platform has its own 
version of main defined. This is made to simplify thinking about engine code,
where some platform-specific functions are epxporeted and thus located not in global scope.
Otherwise there should be two platform APIs: One for game layer, other for platform
*/
#pragma once 
#include "lib/general.h"

#include "platform/os.h"
#include "platform/window.h"
#include "logging.h"

typedef struct {
    void *game_data; 
    
    struct Logging_State *logging_state;
    struct FS_Ctx *filesystem;
    Window_State win_state;
} Engine_Ctx;

#define GAME_UPDATE_SIGNATURE(_name) bool _name(Engine_Ctx *ctx)
typedef GAME_UPDATE_SIGNATURE(Game_Update_Func);

typedef struct {
    Game_Update_Func *update;
} Game_Module_Functions;

const char *GAME_MODULE_FUNCTION_NAMES[] = {
    "game_update"
};
