#include "engine_ctx.h"

#include "platform/osx/osx.h"

int main(void) {
    Engine_Ctx ctx = {0};
    ctx.os.open_file = osx_open_file;
    ctx.os.close_file = osx_close_file;
    ctx.os.read_file = osx_read_file;
    ctx.os.write_file = osx_write_file;
    ctx.os.get_file_size = osx_get_file_size;
    ctx.os.write_stdout = osx_write_stdout;
    ctx.os.write_stderr = osx_write_stderr;
    
    u32 width = 1280;
    u32 height = 720;
    osx_create_window(&ctx.win_state, width, height);

    for (;;) {
        osx_poll_window_events(&ctx.win_state);
        if (ctx.win_state.is_quit_requested) {
            break;
        }
    }
        
    return 0;
}