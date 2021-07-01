#if !defined(GAME_WORLD_HH)

#include "lib/lib.hh"
#include "game/world.hh"

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
};  

enum {
    INTERFACE_ELEMENT_NONE,   
    INTERFACE_ELEMENT_RECTANGLE,   
    INTERFACE_ELEMENT_TEXT,   
    INTERFACE_ELEMENT_BUTTON,   
    INTERFACE_ELEMENT_IMAGE,   
    INTERFACE_ELEMENT_SENTINEL,   
};

struct InterfaceElement {
    u8 kind;  
    Rect rect;
    const char *text;
    AssetID image_id;
    u32 value_id;
    bool is_alive;
    
    InterfaceElement *parent;
    InterfaceElement *first_child;
    InterfaceElement *next;
};

#define MAX_INTERFACE_ELEMENTS 1024

struct InterfaceStats {
    bool is_mouse_over_element;
    bool interaction_occured;
    u32 value_id;
};

struct Interface {
    u32 elements_count;
    InterfaceElement elements[MAX_INTERFACE_ELEMENTS];
    InterfaceElement *first_element;
    
    AssetInfo *font_info;
    FontData *font;
};

void init_interface_for_game_state(Interface *interface, Assets *assets);
InterfaceStats interface_update(Interface *interface, Input *input);
void interface_render(Interface *interface, RenderGroup *render_group);

enum {
    GAME_STATE_BUTTON_NONE,  
    GAME_STATE_BUTTON_BUILDING_MODE,  
    GAME_STATE_BUTTON_MOVE_CAMERA,  
    GAME_STATE_BUTTON_MOVE_BUILDING1,  
    GAME_STATE_BUTTON_MOVE_BUILDING2,  
};

enum {
    INPUT_ACCESS_TOKEN_NO_LOCK, 
    INPUT_ACCESS_TOKEN_GAME_INTERFACE,  
    INPUT_ACCESS_TOKEN_GAME_MENU,  
    INPUT_ACCESS_TOKEN_DEV_UI,  
    INPUT_ACCESS_TOKEN_ALL,  
};

// Wrapper for input locking in case some game sections overlap
// So when game detects that dev ui is focused, it locks input for it
// Access tokens are ordered by priority
// For example, game may set lock for interface after updating, but dev ui is actually focused
// Then we let interface use input this frame, but set more high priority dev ui to be locked
// This actually allows single frame input delay, but due to imm nature of dev ui this is hard to 
// do different. And usually there will be only maximum of two levels of input anyway, so 
// this delay does not occure in game circumstances, and can be easilly avoided in other cases, 
// for example setting lock in the begging of the frame
struct InputManager {
    Input *input;
    u32 access_token;
};

void manage_input(InputManager *manager);
void lock_input(InputManager *manager, u32 access_token);
void unlock_input(InputManager *manager);
bool is_key_pressed(InputManager *manager, Key key, u32 access_token);
bool is_key_released(InputManager *manager, Key key, u32 access_token);
bool is_key_held(InputManager *manager, Key key, u32 access_token);
f32 get_mwheel(InputManager *manager);
Vec2 mouse_p(InputManager *manager);
Vec2 mouse_d(InputManager *manager);
Vec2 window_size(InputManager *manager);
f32 get_dt(InputManager *manager);

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
