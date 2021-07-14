#if !defined(WORLD_STATE_HH)

#include "lib.hh"
#include "sim_region.hh"

struct Camera {
    f32 pitch;
    f32 yaw;
    f32 distance_from_player;
};

enum {
    ORDER_QUEUE_ENTRY_NONE,
    ORDER_QUEUE_ENTRY_CHOP,
};

struct OrderQueueEntry {
    u32 kind;
    EntityID destination_id;
    bool is_assigned;
    
    OrderQueueEntry *next;
    OrderQueueEntry *prev;
};

enum {
    WORLD_OBJECT_TYPE_NONE,
    WORLD_OBJECT_TYPE_RESOURCE,
    WORLD_OBJECT_TYPE_BUILDING,
};

struct WorldObjectSpec {
    u32 type;
    u32 resource_kind;
    u32 default_resource_interactions;
};

#define MAX_PLAYER_PAWNS 32

// Structure that defines all data related to game world - anythting that can or should
// be saved is placed here
struct WorldState {
    MemoryArena *arena;
    MemoryArena *frame_arena;
    
    World *world;
    
    WorldObjectSpec world_object_specs[WORLD_OBJECT_KIND_SENTINEL];
    u32    anchor_count;
    Anchor anchors[MAX_ANCHORS];
    
	EntityID pawns[MAX_PLAYER_PAWNS];
	u32 pawn_count;
    
    Camera cam;    
    EntityID camera_followed_entity;
    EntityID mouse_selected_entity;
    
    Mat4x4 view;
    Mat4x4 projection;
    Mat4x4 mvp;
    Vec3 cam_p;
    Vec2 mouse_projection;
    
    bool draw_frames;
    
    OrderQueueEntry order_queue;
    OrderQueueEntry *order_free_list;
    u32 orders_allocated;
};

void world_state_init(WorldState *world_state, MemoryArena *arena, MemoryArena *frame_arena);
void update_and_render_world_state(WorldState *world_state, InputManager *input, RendererCommands *commands, Assets *assets);

#define WORLD_STATE_HH 1
#endif
