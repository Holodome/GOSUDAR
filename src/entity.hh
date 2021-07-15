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

inline EntityID entity_id(u32 value) { return { value }; }

#define NULL_ENTITY_ID (entity_id(0))

struct OrderID {
    u32 value;
};

struct Entity {
    EntityID id;
    Vec2 p;
    u32 flags;
    u32 kind;
    u32 world_object_flags;
    u32 world_object_kind;
    // resource
    u32 resource_interactions_left;
    // building
    f32 build_progress;      
    // pawn
    OrderID order;
};

#define ENTITY_HH 1
#endif
