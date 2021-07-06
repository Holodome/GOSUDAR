#if !defined(GAME_WORLD_HH)

#include "lib.hh"
#include "world.hh"
#include "assets.hh"
#include "input.hh"

enum {
    PLAYER_INTERACTION_KIND_NONE,
    // Make this separate from destroy not to overcomplicate things trying to use less names
    PLAYER_INTERACTION_KIND_MINE_RESOURCE,
    PLAYER_INTERACTION_KIND_BUILD,
    PLAYER_INTERACTION_KIND_SENTINEL  
};

struct FrameData {
    Assets *assets;
    Vec2 mouse_projection;
    SimEntity *camera_followed_entity;
    SimRegion *sim;
    Mat4x4 view;
    Mat4x4 projection;
    InputManager *input;
};  

enum {
    WORLD_OBJECT_SETTINGS_FLAG_IS_RESOURCE = 0x1,
    WORLD_OBJECT_SETTINGS_FLAG_IS_BUILDING = 0x2,
};

struct WorldObjectSettings {
    u32 resource_gain;
    u32 resource_kind;
    u32 flags;
};  

struct PlayingSound {
    AssetID sound_id;
    f64 play_cursor;
    bool is_finished;
    PlayingSound *next;
};

// All game-related data is stored here. Like player resources, debug thigs etc.
struct GameState {
    MemoryArena arena;
    MemoryArena *frame_arena;
    
    // RendererSettings last_frame_renderer_settings;
    RendererSettings renderer_settings;
    
    u64 playing_sounds_allocated;
    f32 global_volume;
    PlayingSound *first_playing_sound;
    PlayingSound *first_free_playing_sound;
        
    WorldObjectSettings world_object_settings[10];
    
    SimCamera cam;
    EntityID camera_followed_entity_id;
    // Generated world size
    Vec2i min_chunk;
    Vec2i max_chunk;
    
    World *world;
    u32 wood_count;
    u32 gold_count;
    
    EntityID interactable;
    u32 interaction_kind;
    f32 interaction_time;
    f32 interaction_current_time;
    
    u8 selected_building;
    bool is_in_building_mode;
    bool allow_camera_controls;
    bool show_grid;
};

WorldObjectSettings *get_object_settings(GameState *game_state, u32 world_object_kind);

void game_state_init(GameState *game_state, MemoryArena *frame_arena);
void update_and_render(GameState *game_state, InputManager *input, RendererCommands *commands, Assets *assets);

void play_sound(GameState *game_state, AssetID sound_id);
void update_sound(GameState *game_state, Assets *assets, Platform *platform);

#define GAME_WORLD_HH 1
#endif
