#if !defined(GAME_H)

#include "lib.hh"

#include "debug.hh"
#include "os.hh"
#include "renderer.hh"
#include "assets.hh"
#include "render_group.hh"

#include "game_state.hh"

enum GameStateKind {
    GAME_STATE_MAIN_MENU,  
    GAME_STATE_PLAY,  
};

// Game is a object that decribes program as one element 
// It contains several elements that are all used in game state
// Game state is the logic of the game
// It decides how to use all data recived from input, what to render when to close etc.
struct Game {
    bool is_running;
    // Global temporary arena - easy way to allocate some stuff that doesn't need to exist for long
    // Just dump any temporary data here - during development and maybe even after we can afford not to 
    // worry about a couple of megabytes
    MemoryArena frame_arena;
    MemoryArena arena;
    
    OS *os;
    Renderer renderer;
    Assets *assets;
    DebugState *debug_state;
    InputManager input;
    
    //
    // Renderer settings
    //
    RendererSettings renderer_settings;
    //
    // Sound
    //
    u64 playing_sounds_allocated;
    f32 global_volume;
    PlayingSound *first_playing_sound;
    PlayingSound *first_free_playing_sound;
    // Game state
    GameStateKind game_state;  
    //
    // Main menu state
    //
    UIElement *main_menu_interface;
    UIElement *main_menu_start_game_button;
    UIElement *main_menu_exit_button;
    //
    // Game state
    //
    bool is_paused;
    UIElement *pause_interface;
    UIElement *pause_continue;
    UIElement *pause_main_menu;
    UIElement *pause_exit;
};

void game_init(Game *game);
void game_cleanup(Game *game);
void game_update_and_render(Game *game);

#define GAME_H 1
#endif
