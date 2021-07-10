#if !defined(WORLD_STATE_HH)

#define WORLD_STATE_MAX_ANCHORS 32

// Anchor is some object in world that has its own simulation region
// So due to game limitations we want to have different distance parts of the world simulated,
// but we want only simulate parts that interest us
// After simulation is ended, all anchor entities are written in world, 
// so in next simulation begin we know what parts of the world to simulate
struct Anchor {
    u32 chunk_x;
    u32 chunk_y;
    u32 radius;
    bool is_renderable;
};

struct WorldState {
    World *world;
    // It may be more benefitial to have a linked list here due to anchors being able to be moved and deleted
    // but ancgor count is usually small enough that we don't care
    u32    anchor_count;
    Anchor anchors[WORLD_STATE_MAX_ANCHORS];
};

void update_world_state(struct Game *game, WorldState *world_state);

#define WORLD_STATE_HH 1
#endif
