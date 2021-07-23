#include "game.hh"

#define BUILD_TIME __DATE__ " " __TIME__

#if MEM_BOUNDS_CHECKING_POLICY == MEM_ALLOC_UNDERFLOW_CHECK
#define BOUNDS_CHECK_POLICY_STR "Underflow"
#elif MEM_BOUNDS_CHECKING_POLICY == MEM_ALLOC_OVERFLOW_CHECK
#define BOUNDS_CHECK_POLICY_STR "Overflow"
#else 
#error Invalid memory checking policy
#endif 

#if MEM_DO_HARD_BOUNDS_CHECKING
#define MEMORY_CHECK_TYPE_STR "Hard bounds check: " BOUNDS_CHECK_POLICY_STR
#elif MEM_DO_BOUNDS_CHECKING
#define MEMORY_CHECK_TYPE_STR "Soft bounds check: " BOUNDS_CHECK_POLICY_STR
#else
#define MEMORY_CHECK_TYPE_STR "No bounds checking"
#endif 

int main() {
    outf("Running internal build\n");
    outf("Build: " BUILD_TIME "\n");
    outf("Memory: " MEMORY_CHECK_TYPE_STR "\n");
    
    Game *game = game_init();
    while (game->is_running) {
        game_update_and_render(game);
    }
    return 0;
}
