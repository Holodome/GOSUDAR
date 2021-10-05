/*
Author: Holodome
Date: 04.10.2021
File: engine/code_hotloading.h
Version: 0
*/
#pragma once
#include "lib/general.h"

#include "platform/os.h"

struct Engine_Ctx;

typedef struct {
    // If this is false, no functions from module should be called
    bool is_valid;
    
    const char *dll_path;
    // @NOTE(hl): Lock is file created while compiling and deleted after compilation is finished.
    const char *lock_path;
    
    DLL_Handle dll;
    File_Time dll_write_time;
    
    u32 function_count;
    void **functions; 
    const char **function_names; 
} Code_Hotloading_Module;

ENGINE_PUB void code_hotload(Code_Hotloading_Module *module);
ENGINE_PUB void code_hotload_unload(Code_Hotloading_Module *module);
ENGINE_PUB void code_hotload_update(Code_Hotloading_Module *module);