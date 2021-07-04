#if !defined(GAME_WORLD_HH)

#include "lib.hh"
#include "world.hh"
#include "assets.hh"
#include "input.hh"
#include "interface.hh"

enum {
    PLAYER_INTERACTION_KIND_NONE,
    // Make this separate from destroy not to overcomplicate things trying to use less names
    PLAYER_INTERACTION_KIND_MINE_RESOURCE,
    PLAYER_INTERACTION_KIND_BUILD,
    PLAYER_INTERACTION_KIND_SENTINEL  
};

struct FrameData {
    Vec2 mouse_projection;
    SimEntity *camera_followed_entity;
    SimRegion *sim;
    Mat4x4 view;
    Mat4x4 projection;
    InputManager *input;
};  

enum {
    GAME_STATE_BUTTON_NONE,  
    GAME_STATE_BUTTON_BUILDING_MODE,  
    GAME_STATE_BUTTON_MOVE_CAMERA,  
    GAME_STATE_BUTTON_MOVE_BUILDING1,  
    GAME_STATE_BUTTON_MOVE_BUILDING2,  
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

// All game-related data is stored here. Like player resources, debug thigs etc.
struct GameState {
    MemoryArena arena;
    MemoryArena frame_arena;
    
    GameStateInterface inter;
    WorldObjectSettings world_object_settings[WORLD_OBJECT_KIND_SENTINEL];
    
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

void game_state_init(GameState *game_state);
void update_and_render(GameState *game_state, InputManager *input, RendererCommands *commands, Assets *assets);

#define GAME_WORLD_HH 1
#endif
