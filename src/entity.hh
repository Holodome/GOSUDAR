#if !defined(ENTITY_HH)

#include "lib.hh"
#include "game_enums.hh"

enum {
    ENTITY_FLAG_IS_DELETED = 0x1,
    ENTITY_FLAG_IS_ANCHOR  = 0x2,
    ENTITY_FLAG_HAS_WORLD_PLACEMENT = 0x4,
};

struct EntityID {
    u32 value;
};

bool is_same(EntityID a, EntityID b) {
    return a.value == b.value;
}

bool is_null(EntityID a) {
    return a.value == 0;
}

struct Entity {
    EntityID id;
    Vec2 p;
    u32 flags;
    u32 kind;
    u32 world_object_flags;
    u32 world_object_kind;
    u32 resource_interactions_left;
    f32 build_progress;      
};

#define ENTITY_HH 1
#endif
