/*
Author: Holodome
Date: 04.10.2021
File: engine/code_hotloading.h
Version: 0
*/
#pragma once
#include "lib/general.h"

#include "platform/os.h"

typedef struct {
    bool is_valid;
    u32 temp_dll_number;
    
    char *const dll_path;
    char *const lock_path;
    
    void *dll; 
    File_Time dll_write_time;
    
    u32 function_count;
    void **functions; 
    const char **function_names; 
} Code_Hotloading_Module;