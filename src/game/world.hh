#if !defined(WORLD_HH)

#include "lib/lib.hh"

#include "game/camera.hh"

#define TILE_SIZE 1.0f
#define TILES_IN_CHUNK 4
#define CHUNK_SIZE (1.0f * TILES_IN_CHUNK)
#define CELLS_IN_TILE 8
#define CELLS_IN_CHUNK CELLS_IN_TILE * TILES_IN_CHUNK
#define CELL_SIZE (TILE_SIZE / CELLS_IN_TILE)

struct WorldPosition {
    Vec2i chunk; // Chunk coordinate
    Vec2 offset; // Offset in chunk, world coordinate units in [0; CHUNK_SIZE]
};  

// Add offset to base_pos and return new position
inline WorldPosition pos_add(WorldPosition base_pos, Vec2 offset);
inline Vec2 distance_between_pos(WorldPosition a, WorldPosition b);
inline bool is_same_chunk(WorldPosition a, WorldPosition b);
inline Vec2 world_pos_to_p(WorldPosition pos);
inline WorldPosition world_position_from_tile_position(Vec2i tile_position);

enum EntityKind {
    EntityKind_None = 0x0,
    EntityKind_Player,      // Special case entity, we probably want to replace this with Pawn when we have AI
                            // And update player independent in game state
    EntityKind_WorldObject, // Tree,building,deposit - basically everything that has placement within cells
};  

enum WorldObjectFlags {
    WorldObjectFlags_IsTree     = 0x1,
    WorldObjectFlags_IsBuilding = 0x2,
};

enum EntityFlags {
    EntityFlags_IsDeleted = 0x1,
};

struct EntityID {
    u32 value;    
};  

inline EntityID null_id();
inline bool is_same(EntityID a, EntityID b);
inline EntityID entity_id_from_storage_index(u32 index);

enum WorldObjectKind {
    WorldObjectKind_None = 0x0,
    WorldObjectKind_TreeForest,
    WorldObjectKind_TreeDesert,
    WorldObjectKind_TreeJungle,
    WorldObjectKind_GoldDeposit,
    WorldObjectKind_Building1,
    WorldObjectKind_Building2,
};

struct SimEntity {
    EntityID entity_id;
    EntityKind kind;
    u32 flags; // EntityFlags
    // For sim
    // EntityReference ref;
    Vec2 p;
    // WorldObject
    WorldObjectKind world_object;
    u32 world_object_flags;
    Vec2i min_cell;
};  

inline void add_flags(SimEntity *entity, u32 flags);
inline void remove_flags(SimEntity *entity, u32 flags);
inline bool is_set(SimEntity *entity, u32 flag);

// Entity stored in world
struct Entity {
    SimEntity sim;
    // Stores data of chunk entity is in
    WorldPosition world_pos;
};

struct EntityBlock {
    u32 entity_count;
    EntityID entity_storage_indices[16];
    EntityBlock *next;
};  

// World is split into chunks by entity position
// Enitites in chunk are stored in EntityBlock
// It is linked list with 16 entities in block
struct Chunk {
    EntityBlock entity_block;
    Vec2i coord;
    Chunk *next_in_hash;
};

#define WORLD_CHUNK_HASH_SIZE 4096
CT_ASSERT(IS_POW2(WORLD_CHUNK_HASH_SIZE));
// @TODO make world inners inaccessable, use api calls for all
// because it starts to become a mess 
// World is a structure that is responsible for storing entities
// It can be though as entity list, that additionally has spatial partition and uses ids for access
struct World {
    MemoryArena *world_arena;
    MemoryArena *frame_arena;
    // Linked list free entry
    EntityBlock *first_free;
    // Chunks hash table. Implemented with external collision resolving (other chunks are allocated separately)
    Chunk chunk_hash[WORLD_CHUNK_HASH_SIZE];
    
    size_t max_entity_count;
    size_t entity_count;
    Entity *entities;
};  

EntityID add_world_entity(World *world, WorldPosition pos);
Entity *get_world_entity(World *world, EntityID id);
Chunk *get_world_chunk(World *world, Vec2i coord);
bool remove_entity_from_chunk(World *world, Chunk *chunk, EntityID id);
void add_entity_to_chunk(World *world, Chunk *chunk, EntityID id);
// Remove entity from old chunk and move to new position
void move_entity(World *world, EntityID id, WorldPosition to, WorldPosition from);

struct SimCamera {
    f32 pitch;
    f32 yaw;
    f32 distance_from_player;
};

struct SimEntityHash {
    SimEntity *ptr;
    EntityID id;
};

#define SIM_REGION_ENTITY_COUNT 4096
CT_ASSERT(IS_POW2(SIM_REGION_ENTITY_COUNT));
// Represents part of the world that can be updated
// While world is used for storing and accessing entities in different parts of the world,
// sim region collates entities for similar postions, which is neccessary for updating due to floating point mistakes
// Also it provides space partitioning
// This is going to be very computationally heavy piece of data, so we better do copies here instead of using pointers to world
struct SimRegion {
    MemoryArena *frame_arena;
    // World *world;
    struct GameState *game_state;
    // So in case player positions are some huge float numbers,
    // map entities from world position to sim position and back after sim end
    WorldPosition origin;
    Vec2i min_chunk;
    Vec2i max_chunk;
    // Local space camera
    SimCamera cam;
    Vec3 cam_p;
    Mat4x4 cam_mvp;
    
    size_t max_entity_count;
    size_t entity_count;
    SimEntity *entities;
    // Mapping from EntityID, which is storage index in world to entity in sim region
    // get_entity_by_id can be used to get sim entity by world id
    SimEntityHash entity_hash[SIM_REGION_ENTITY_COUNT];
};  

// Create sim region in given world part
// Puts all entities from selected world chunks into sim 
SimRegion *begin_sim(struct GameState *game_state, Vec2i min_chunk, Vec2i max_chunk);
// Puts all entities from sim region to their world storage2
void end_sim(SimRegion *region);
// Create new entity 
SimEntity *create_entity(SimRegion *sim);
// Add entity from world
SimEntityHash *get_hash_from_storage_index(SimRegion *sim, EntityID entity_id);
SimEntity *add_entity(SimRegion *sim, EntityID entity_id);
SimEntity *get_entity_by_id(SimRegion *sim, EntityID entity_id);

// Iterate all not deleted entities
struct EntityIterator {
    SimRegion *sim;
    size_t     idx;
    SimEntity *ptr;
};

EntityIterator iterate_all_entities(SimRegion *sim);
void advance(EntityIterator *iter);

// void load_entity_reference(SimRegion *sim, EntityReference *ref);
// void store_entity_reference(EntityReference *ref);

enum BuildingKind {
    BuildingKind_None,
    BuildingKind1,
    BuildingKind2
};

// All game-related data is stored here. Like player resources, debug thigs etc.
struct GameState {
    MemoryArena arena;
    MemoryArena frame_arena;
    
    SimCamera camera;
    EntityID camera_followed_entity_id;
    // Generated world size
    Vec2i min_chunk;
    Vec2i max_chunk;
    
    World *world;
    u32 wood_count;
    u32 gold_count;
    bool is_player_interacting;
    EntityID interactable;
    
    BuildingKind selected_builing_kind;
};

void game_state_init(GameState *game_state);
void update_and_render(GameState *game_state, Input *input, Renderer *renderer, Assets *assets);

#define WORLD_HH 1
#endif
