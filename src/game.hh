#if !defined(GAME_H)

#include "lib.hh"

#include "debug.hh"
#include "os.hh"
#include "renderer.hh"
#include "assets.hh"
#include "render_group.hh"
#include "interface.hh"

struct PlayingSound {
    AssetID sound_id;
    f64 play_cursor;
    bool is_finished;
    PlayingSound *next;
};

enum GameStateKind {
    GAME_STATE_MAIN_MENU,  
    GAME_STATE_PLAY,  
};

enum MainMenuState {
    MAIN_MENU_MAIN_SCREEN,  
    MAIN_MENU_SETTINGS,  
};

struct Camera {
    f32 pitch;
    f32 yaw;
    f32 distance_from_player;
};

#define TILE_SIZE 1.0f
#define TILES_IN_CHUNK 4
#define CHUNK_SIZE (1.0f * TILES_IN_CHUNK)
#define CELLS_IN_TILE 8
#define CELLS_IN_CHUNK CELLS_IN_TILE * TILES_IN_CHUNK
#define CELL_SIZE (TILE_SIZE / CELLS_IN_TILE)

inline Vec2 floor_to_cell(Vec2 pos) {
    pos.x = floorf(pos.x / CELL_SIZE) * CELL_SIZE;
    pos.y = floorf(pos.y / CELL_SIZE) * CELL_SIZE;
    return pos;
}

struct EntityID {
    u32 value;
};

struct Entity {
    EntityID id;
    Vec2 p;
    u32 flags;
    u32 kind;
    u32 world_object_flags;
    u32 world_object_kind;
    u32 resource_interactions_left;
    f32 build_progress;      
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
    Camera cam;
    Entity entities[4096];
    u32 entity_count;
    EntityID camera_followed_entity;
    Mat4x4 view;
    Mat4x4 projection;
    Mat4x4 mvp;
    
    bool is_paused;
    UIElement *pause_interface;
    UIListener *pause_continue;
    UIListener *pause_main_menu;
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

struct EntityIterator {
    Game *game;
    u64 idx;
    Entity *ptr;
};

static void set_entity_to_next_not_deleted(EntityIterator *iter) {
    while ((iter->idx < iter->game->entity_count) && (iter->game->entities[iter->idx].flags & ENTITY_FLAG_IS_DELETED)) {
        ++iter->idx;
    }
    
    if (iter->idx < iter->game->entity_count) {
        iter->ptr = iter->game->entities + iter->idx;
    } else {
        iter->ptr = 0;
    }
}

EntityIterator iterate_all_entities(Game *game) {
    EntityIterator iter;
    iter.game = game;
    iter.idx = 0;
    set_entity_to_next_not_deleted(&iter);
    return iter;
}

bool is_valid(EntityIterator *iter) {
    return iter->ptr != 0;    
}

void advance(EntityIterator *iter) {
    ++iter->idx;
    set_entity_to_next_not_deleted(iter);
}

#define GAME_H 1
#endif
