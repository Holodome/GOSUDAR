#ifndef COMPILE_GAME
#include "engine_ctx.h"

#include "filesystem.h"
#include "logging.h"
#include "code_hotloading.h"

static Engine_Ctx ctx;

const char *GAME_MODULE_FUNCTION_NAMES[] = {
    "game_update"
};
const uptr GAME_MODULE_FUNCTIONS_COUNT = ARRAY_SIZE(GAME_MODULE_FUNCTION_NAMES);

static void 
init_ctx() {
    ctx.filesystem = create_filesystem();
    ctx.logging_state = create_logging_state("game.log");
    
    char buffer[4096];
    uptr executable_path_len = os_fmt_executable_path(buffer, sizeof(buffer));
    log_info("Executable path   %s", buffer);
    char *last_slash = buffer;
    char *cursor = last_slash;
    while (cursor < buffer + executable_path_len) {
        if (*cursor == '/') {
            last_slash = cursor;
        }
        ++cursor;
    }
    uptr folder_len = last_slash - buffer;
    buffer[folder_len] = 0;
    log_info("Executable folder %s", buffer);
    os_chdir(buffer);
    os_fmt_cwd(buffer, sizeof(buffer));
    log_debug("CWD: %s", buffer);
    // @LEAK
    ctx.executable_folder = mem_alloc_str(buffer);
}

static void 
init_game_hotloading(Game_Module_Functions *game_functions, Code_Hotloading_Module *module) {
    module->function_names = GAME_MODULE_FUNCTION_NAMES;
    module->function_count = GAME_MODULE_FUNCTIONS_COUNT;
    module->functions = (void **)game_functions;
    char buffer[4096];
    engine_ctx_fmt_local_filepath(buffer, sizeof(buffer), 
        &ctx, "game.dylib");
    // @LEAK
    module->dll_path = mem_alloc_str(buffer);
    engine_ctx_fmt_local_filepath(buffer, sizeof(buffer),
        &ctx, "game.tmp");
    // @LEAK
    module->lock_path = mem_alloc_str(buffer);
    log_info("Game code path %s", module->dll_path);
    log_info("Game code lock path %s", module->lock_path);
    code_hotload(module);
}


int main(void) {
    init_ctx();
    
    u32 width = 1280;
    u32 height = 720;
    os_create_window(&ctx.win_state, width, height);
    
    Game_Module_Functions game_functions = {0};
    Code_Hotloading_Module game_module = {0};
    init_game_hotloading(&game_functions, &game_module);
    
    for (;;) {
        os_poll_window_events(&ctx.win_state);
        bool should_end = false;
        if (game_module.is_valid) {
            should_end = game_functions.update(&ctx);
        }
        if (should_end) {
            break;
        }
        code_hotload_update(&game_module);
    }
        
    return 0;
}
#endif