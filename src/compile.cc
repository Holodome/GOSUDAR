#include "debug.cc"
#include "mips.cc"
#include "renderer.cc"
#include "render_group.cc"
#include "assets.cc"
#include "dev_ui.cc"
#include "world.cc"
#include "sim_region.cc"
#include "world_state.cc"
#include "orders.cc"
#include "game.cc"
#include "interface.cc"
#include "main.cc"
#include "os.cc"
#if GOSUDAR_NO_STDIO
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
#endif 