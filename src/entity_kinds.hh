#if !defined(ENTITY_KINDS_HH)

// Entity flags
enum {
    ENTITY_FLAG_IS_DELETED = 0x1,
    ENTITY_FLAG_SINGLE_FRAME_LIFESPAN = 0x2,
    ENTITY_FLAG_SENTINEL
};
// Entity kind
enum {
    ENTITY_KIND_NONE,
    // Special case entity, we probably want to replace this with Pawn when we have AI
    // And update player independent in game state
    ENTITY_KIND_PLAYER, 
    // Tree,building,deposit - basically everything that has placement within cells
    ENTITY_KIND_WORLD_OBJECT,
    ENTITY_KIND_SENTINEL,
};  

enum {
    WORLD_OBJECT_FLAG_IS_BUILT = 0x1,
    WORLD_OBJECT_FLAG_IS_BLUEPRINT = 0x2
};

// World object kind
enum {
    WORLD_OBJECT_KIND_NONE,
    WORLD_OBJECT_KIND_TREE_FOREST,
    WORLD_OBJECT_KIND_TREE_DESERT,
    WORLD_OBJECT_KIND_TREE_JUNGLE,
    WORLD_OBJECT_KIND_GOLD_DEPOSIT,
    WORLD_OBJECT_KIND_BUILDING1,
    WORLD_OBJECT_KIND_BUILDING2,
    WORLD_OBJECT_KIND_SENTINEL,
};  

enum {
    RESOURCE_KIND_NONE,
    RESOURCE_KIND_WOOD,
    RESOURCE_KIND_GOLD,
    RESOURCE_KIND_SENTINEL,
};

#define ENTITY_KINDS_HH 1
#endif
