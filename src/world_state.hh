#if !defined(WORLD_STATE_HH)

#include "lib.hh"
#include "sim_region.hh"

struct Camera {
    f32 pitch;
    f32 yaw;
    f32 distance_from_player;
};

struct WorldState {
    MemoryArena *arena;
    MemoryArena *frame_arena;
    
    World *world;
    // It may be more benefitial to have a linked list here due to anchors being able to be moved and deleted
    // but anchor count is usually small enough that we don't care
    u32    anchor_count;
    Anchor anchors[MAX_ANCHORS];
    
    Camera cam;    
    EntityID camera_followed_entity;
    // This can be condiered per-frame data,
    // but it needs to be saved in order
    // to render when updating is not done
    Mat4x4 view;
    Mat4x4 projection;
    Mat4x4 mvp;
    Vec3 cam_p;
    Vec2 mouse_projection;
    
    bool draw_frames;
};

void world_state_init(WorldState *world_state, MemoryArena *arena, MemoryArena *frame_arena);
void update_and_render_world_state(WorldState *world_state, InputManager *input, RendererCommands *commands, Assets *assets);

#define WORLD_STATE_HH 1
#endif
