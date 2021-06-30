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

// All game-related data is stored here. Like player resources, debug thigs etc.
struct GameState {
    MemoryArena arena;
    MemoryArena frame_arena;
    
    // Interface interface;
    
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
    
    size_t DEBUG_last_frame_sim_region_entity_count;
};

void game_state_init(GameState *game_state);
void update_and_render(GameState *game_state, Input *input, RendererCommands *commands, Assets *assets);

#define GAME_WORLD_HH 1
#endif
