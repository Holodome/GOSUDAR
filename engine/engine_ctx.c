#include "engine_ctx.h"

#include "lib/strings.h"

const char *GAME_MODULE_FUNCTION_NAMES[] = {
    "game_update"
};
const uptr GAME_MODULE_FUNCTIONS_COUNT = ARRAY_SIZE(GAME_MODULE_FUNCTION_NAMES);

uptr 
engine_ctx_fmt_local_filepath(char *bf, uptr bf_sz, 
    Engine_Ctx *ctx, const char *local_path) {
    return fmt(bf, bf_sz, "%s/%s", ctx->executable_folder, local_path);
}