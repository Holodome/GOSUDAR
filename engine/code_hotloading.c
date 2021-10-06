#include "code_hotloading.h"

#include "lib/memory.h"
#include "lib/strings.h"
#include "engine_ctx.h"

void 
code_hotload_unload(Code_Hotloading_Module *module) {
    if (module->dll.handle) {
        os_unload_dll(module->dll);
        module->dll.handle = 0;
    }    
    
    module->is_valid = false;
    mem_zero(module->functions, sizeof(void *) * module->function_count);
}

void 
code_hotload(Code_Hotloading_Module *module) {
    if (os_file_exists(module->lock_path)) {
        return;
    }
    const char *source_dll_name = module->dll_path;
    log_info("(re)Loading game module '%s'", source_dll_name);
    // Create temporary file to load code from
    // This is needed in order to keep the source dll untocuhed, so it 
    // can be rewritten to by compiler
    char temp_dll_name[4096];
    fmt(temp_dll_name, sizeof(temp_dll_name), "%s.temp",
            source_dll_name);
    
    module->dll_write_time = os_get_file_write_time(source_dll_name);
    // Make a couple of attempts but don't go into infinite loop
    for (u32 attempt = 0; attempt < 128; ++attempt) {
        if (os_copy_file(source_dll_name, temp_dll_name)) {
            break;
        }
    }
    
    module->dll = os_load_dll(temp_dll_name);
    if (module->dll.handle) {
        module->is_valid = true;
        for (uptr i = 0; i < module->function_count; ++i) {
            const char *func_name = module->function_names[i];
            void *func = os_dll_symb(module->dll, func_name);
            if (!func) {
                log_error("Failed to load func '%s' from module '%s'", func_name, source_dll_name);
                module->is_valid = false;
            } else {
                module->functions[i] = func;
            }
        }
    } else {
        // @NOTE(hl): This function should only be called if file exists, so
        // think of reasons why it could fail to load
        // assert(false);
    }
    // Temporary file is deleted as its contents are loaded into current process
    os_delete_file(temp_dll_name);
    
    if (!module->is_valid) {
        log_info("Failed to load module '%s'", source_dll_name);
        code_hotload_unload(module);
    } else {
        log_info("Module '%s' loaded", source_dll_name);
    }
}

void 
code_hotload_update(Code_Hotloading_Module *module) {
    File_Time current_file_time = os_get_file_write_time(module->dll_path);
    if (os_cmp_file_write_time(current_file_time, module->dll_write_time) != 0) {
        code_hotload_unload(module);
        code_hotload(module);
    }    
}