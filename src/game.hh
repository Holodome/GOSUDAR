#if !defined(GAME_H)

#include "lib.hh"

#include "debug.hh"
#include "os.hh"
#include "renderer.hh"
#include "assets.hh"
#include "render_group.hh"
#include "interface.hh"

#include "world.hh"
#include "sim_region.hh"

struct PlayingSound {
    AssetID sound_id;
    f64 play_cursor;
    bool is_finished;
    PlayingSound *next;
};

enum StateKind {
    STATE_MAIN_MENU,  
    STATE_PLAY,  
};

enum MainMenuState {
    MAIN_MENU_MAIN_SCREEN,  
    MAIN_MENU_SETTINGS,  
};

enum GameState {
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,
    GAME_STATE_SETTINGS,
};

struct Camera {
    f32 pitch;
    f32 yaw;
    f32 distance_from_player;
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
    StateKind state;  
    //
    // Main menu state
    //
    MemoryArena interface_arena;
    MainMenuState main_menu_state;
    UIElement *main_menu_interface;
    UIListener *main_menu_start_game_button;
    UIListener *main_menu_settings_button;
    UIListener *main_menu_exit_button;
    
    UIElement *settings_interface;
    UIListener *settings_vsync;
    UIListener *settings_texture_filtering;
    UIListener *settings_texture_mipmapping;
    UIListener *settings_back;
    //
    // Game state
    //
    GameState game_state;
    Camera cam;
    World *world;
    EntityID camera_followed_entity;
    Mat4x4 view;
    Mat4x4 projection;
    Mat4x4 mvp;
    
    UIElement *pause_interface;
    UIListener *pause_continue;
    UIListener *pause_main_menu;
    UIListener *pause_settings;
    UIListener *pause_exit;
    
    UIElement *game_interface;
    UIListener *game_interface_button_mine_resource;
    UIListener *game_interface_button_ground_interact;
    UIListener *game_interface_button_building1;
    UIListener *game_interface_button_building2;
};

void game_init(Game *game);
void game_cleanup(Game *game);
void game_update_and_render(Game *game);

#define GAME_H 1
#endif
