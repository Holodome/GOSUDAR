#include "engine_ctx.h"

#include "lib/strings.h"

GAME_UPDATE_SIGNATURE(game_update) {
    UNUSED(ctx);
    // log_debug("Hello");
    return ctx->win_state.is_quit_requested;
}