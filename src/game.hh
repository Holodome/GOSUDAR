#if !defined(GAME_H)

#include "lib.hh"

#include "mem.hh"
#include "os.hh"

#include "assets.hh"
#include "renderer_api.hh"

#include "ui.hh"
#include "audio.hh"
#include "world.hh"
#include "sim_region.hh"
#include "world_state.hh"

#include "debug.hh"

enum StateKind {
    STATE_MAIN_MENU,  
    STATE_PLAY,  
};

enum {
    MAIN_MENU_MAIN_SCREEN,  
    MAIN_MENU_SETTINGS,  
};

enum {
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,
    GAME_STATE_SETTINGS,
};

struct MainMenuUI {
    UIID play_button;
    UIID settings_button;
    UIID exit_button;
};

struct SettingsUI {
    UIID vsync;
    UIID tex_filter;
    UIID mipmapping;
    UIID back;
    
    UIID volume;
};

struct GameUI {
    UIID toolbar;
};

struct GamePauseUI {
    UIID play;
    UIID settings;
    UIID button;
    UIID main_menu;
};

struct MainMenuState {
    u32 main_menu_state_kind;
};

struct PlayState {
    u32 game_state;
    WorldState world_state;
};

// Game is a object that decribes program as one element 
// It is responsible for initializing and updating all separate game elements (see GameLinks)
// 
// Policy on allocations: All modules should have their own allocators
struct Game {
    bool is_running;
    // Global temporary arena - easy way to allocate some stuff that doesn't need to exist for long
    // Just dump any temporary data here - during development and maybe even after we can afford not to 
    // worry about a couple of megabytes
    MemoryArena frame_arena;
    MemoryArena arena;
    
    OS *os;
    Renderer *renderer;
    Assets *assets;
    DebugState *debug_state;
    InputManager input;
    AudioSystem audio;
    RendererSettings renderer_settings;
    RendererCommands commands;
    UI *ui;
    
    StateKind state;  
    MainMenuState main_menu_state;;
    PlayState play_state;
};

void game_init(Game *game);
void game_cleanup(Game *game);
void game_update_and_render(Game *game);

#define GAME_H 1
#endif
