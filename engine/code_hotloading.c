#include "code_hotloading.h"

#include "lib/memory.h"
#include "lib/strings.h"
#include "engine_ctx.h"

void 
code_hotload_unload(Code_Hotloading_Module *module) {
    if (module->dll) {
        os_unload_dll(module->dll);
        module->dll = 0;
    }    
    
    module->is_valid = false;
    mem_zero(module->functions, sizeof(void *) * module->function_count);
}

void 
code_hotload(Code_Hotloading_Module *module) {
    const char *source_dll_name = module->dll_path;
    
    char temp_dll_name[4096];
    fmt(temp_dll_name, sizeof(temp_dll_name), "%s.%u.temp",
            source_dll_name, module->temp_dll_number);
    module->dll_write_time = os_get_file_write_time(source_dll_name);
    for (u32 attempt = 0; attempt < 128; ++attempt) {
        if (os_copy_file(source_dll_name, temp_dll_name)) {
            break;
        }
    }
    
    module->dll = os_load_dll(temp_dll_name);
    if (module->dll) {
        module->is_valid = true;
        for (uptr i = 0; i < module->function_count; ++i) {
            const char *func_name = module->function_names[i];
            void *func = os_dll_symb(module->dll, func_name);
            if (!func) {
                log_error("Failed to load func '%s' from dll '%s'", func_name, source_dll_name);
                module->is_valid = false;
            } else {
                module->functions[i] = func;
            }
        }
    }
    os_delete_file(temp_dll_name);
    
    if (!module->is_valid) {
        code_hotload_unload(module);
    }
}

void 
code_hotload_update(Code_Hotloading_Module *module) {
    File_Time current_file_time = os_get_file_write_time(module->dll_path);
    if (os_cmp_file_write_time(current_file_time, module->dll_write_time) != 0) {
        code_hotload_unload(module);
        module->dll_write_time = current_file_time;
        code_hotload(module);
    }    
}