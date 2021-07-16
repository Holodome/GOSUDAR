#if !defined(ENTITY_HH)

#include "lib.hh"
#include "game_enums.hh"

enum {
    ENTITY_FLAG_IS_DELETED = 0x1,
    ENTITY_FLAG_IS_ANCHOR  = 0x2,
    ENTITY_FLAG_HAS_WORLD_PLACEMENT  = 0x4,
};

struct EntityID {
    u32 value;
};

struct OrderID {
    u32 value;
};

struct ParticleEmitterID {
    u32 value;
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
    u32 resource_gain;
};

enum {
    INTERACTION_KIND_NONE,
    INTERACTION_KIND_MINE_RESOURCE,
};

// @TODO see if it is better inlined as entity fields
struct Interaction {
    u32 kind; // Used to check if interaction exists
    EntityID entity;
    f32 time;
    f32 current_time;
    
    ParticleEmitterID particle_emitter;
};

struct Entity {
    EntityID id;
    Vec2 p;
    u32 flags;
    u32 kind;
    u32 world_object_kind;
    // resource
    u32 resource_interactions_left;
    // building
    f32 build_progress;      
    // pawn
    OrderID order;
    Interaction interaction;
};

#define ENTITY_HH 1
#endif
