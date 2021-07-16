#if !defined(WORLD_STATE_HH)

#include "lib.hh"
#include "sim_region.hh"
#include "orders.hh"

struct Camera {
    f32 pitch;
    f32 yaw;
    f32 distance_from_player;
};

#define DISTANCE_TO_MOUSE_SELECT (1.0f)
#define DISTANCE_TO_MOUSE_SELECT_SQ (DISTANCE_TO_MOUSE_SELECT * DISTANCE_TO_MOUSE_SELECT)
#define DISTANCE_TO_INTERACT (0.25f)
#define DISTANCE_TO_INTERACT_SQ SQ(DISTANCE_TO_INTERACT)
#define MAX_PLAYER_PAWNS 32
#define PAWN_DISTANCE_TO_PLAYER 3.0f
#define PAWN_DISTANCE_TO_PLAYER_SQ SQ(PAWN_DISTANCE_TO_PLAYER)
#define PAWN_SPEED 3.0f

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
    OrderSystem order_system;
    
    u32 wood_count;
};

void world_state_init(WorldState *world_state, MemoryArena *arena, MemoryArena *frame_arena);
void update_and_render_world_state(WorldState *world_state, InputManager *input, RendererCommands *commands, Assets *assets);

#define WORLD_STATE_HH 1
#endif
