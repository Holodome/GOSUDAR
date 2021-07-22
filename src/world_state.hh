#if !defined(WORLD_STATE_HH)

#include "lib.hh"
#include "sim_region.hh"
#include "orders.hh"
#include "particle_system.hh"

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
#define WORLD_EPSILON 0.001f
#define WORLD_VISUAL_EPSILON 0.01f
#define CAMERA_FOV rad(60)
#define CAMERA_NEAR_PLANE 0.001f
#define CAMERA_FAR_PLANE  10000.0f
#define MIN_CAM_PITCH (HALF_PI * 0.1f)
#define MAX_CAM_PITCH (HALF_PI * 0.5f)
#define X_VIEW_COEF 1.0f
#define Y_VIEW_COEF 0.6f
#define MOVE_COEF 16.0f
#define ZOOM_COEF 1.0f

// Structure that defines all data related to game world - anythting that can or should
// be saved is placed here
struct WorldState {
    MemoryArena arena;
    
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
    vec3 cam_p;
    vec2 mouse_projection;
    
    bool draw_frames;
    OrderSystem order_system;
    ParticleSystem particle_system;
    
    u32 wood_count;
};

WorldState *world_state_init();
void update_and_render_world_state(WorldState *world_state, GameLinks links);

#define WORLD_STATE_HH 1
#endif
