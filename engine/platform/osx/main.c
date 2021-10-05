#include "engine_ctx.h"

#include "filesystem.h"
#include "logging.h"
#include "code_hotloading.h"

static Engine_Ctx ctx;

int main(void) {
    ctx.filesystem = create_filesystem();
    ctx.logging_state = create_logging_state("game.log");
    
    char buffer[1024];
    uptr executable_path_len = os_fmt_executable_path(buffer, sizeof(buffer));
    log_info("Executable %s", buffer);
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
    
    u32 width = 1280;
    u32 height = 720;
    os_create_window(&ctx.win_state, width, height);
    
    Game_Module_Functions game_functions = {0};
    Code_Hotloading_Module game_module = {0};
    game_module.function_names = GAME_MODULE_FUNCTION_NAMES;
    game_module.functions = (void **)&game_functions;

    for (;;) {
        os_poll_window_events(&ctx.win_state);
        if (ctx.win_state.is_quit_requested) {
            break;
        }
    }
        
    return 0;
}