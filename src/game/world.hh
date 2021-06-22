#if !defined(WORLD_HH)

#include "lib/lib.hh"

#include "game/camera.hh"

#define TILE_SIZE 1.0f
#define TILES_IN_CHUNK 4
#define CHUNK_SIZE (1.0f * TILES_IN_CHUNK)

struct WorldPosition {
    Vec2i chunk; // Chunk coordinate
    Vec2 offset; // Offset in chunk, world coordinate units in [0; CHUNK_SIZE]
};  

// Add offset to base_pos and return new position
inline WorldPosition pos_add(WorldPosition base_pos, Vec2 offset);
inline Vec2 distance_between_pos(WorldPosition a, WorldPosition b);
inline bool is_same_chunk(WorldPosition a, WorldPosition b);
inline WorldPosition world_position_from_tile_position(Vec2i tile_position);

enum struct EntityKind {
    None = 0x0,
    Player,
    Tree,
    GroundTile
};  

enum EntityFlags {
    EntityFlags_IsBillboard = 0x1,
    EntityFlags_IsDeleted = 0x2,
};

struct EntityID {
    u32 value;    
};  

inline EntityID null_id();
inline bool is_same(EntityID a, EntityID b);
inline EntityID entity_id_from_storage_index(u32 index);

// struct EntityReference {
//     EntityID entity_id;
//     struct SimEntity *ptr;
// };  

struct SimEntity {
    EntityID entity_id;
    EntityKind kind;
    u32 flags; // EntityFlags
    // For sim
    // EntityReference ref;
    Vec2 p;
    AssetID texture_id;
    Vec2 size; // texture size multiplier
    Vec2i tile_pos;
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

struct ChunkEntityBlock {
    u32 entity_count;
    EntityID entity_storage_indices[16];
    ChunkEntityBlock *next;
};  

// World is split into chunks by entity position
// Enitites in chunk are stored in ChunkEntityBlock
// It is linked list with 16 entities in block
struct Chunk {
    ChunkEntityBlock entity_block;
    Vec2i coord;
    Chunk *next_in_hash;
};

struct SimCamera {
    f32 pitch;
    f32 yaw;
    f32 distance_from_player;
};

struct World {
    MemoryArena *world_arena;
    MemoryArena *frame_arena;
    // Linked list free entry
    ChunkEntityBlock *first_free;
    // Chunks hash table. Implemented with external collision resolving (other chunks are allocated separately)
    Chunk chunk_hash[4096];
    
    size_t max_entity_count;
    size_t entity_count;
    Entity *entities;
    
    SimCamera camera;
    EntityID camera_followed_entity_idx;
};  

EntityID add_entity(World *world, EntityKind kind, WorldPosition pos);
Entity *get_entity_by_id(World *world, EntityID id);
Chunk *get_world_chunk(World *world, Vec2i coord);
void remove_entity_from_chunk(World *world, Chunk *chunk, EntityID id);
// Place entity in new chunk and remove from previous if needed
void change_entity_position_raw(World *world, EntityID entity_id, WorldPosition new_p, WorldPosition *old_p = 0);
void change_entity_position(World *world, Entity *entity, WorldPosition new_p);

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
    World *world;
    // So in case player positions are some huge float numbers,
    // map entities from world position to sim position and back after sim end
    WorldPosition origin;
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

SimRegion *begin_sim(MemoryArena *sim_arena, World *world);
void end_sim(SimRegion *region);
SimEntityHash *get_hash_from_storage_index(SimRegion *sim, EntityID entity_id);
SimEntity *get_entity_by_id(SimRegion *sim, EntityID entity_id);
SimEntity *add_entity_raw(SimRegion *sim, EntityID entity_id, Entity *source = 0);
SimEntity *add_entity(SimRegion *sim, EntityID entity_id, Entity *source);

struct EntityIterator {
    size_t idx;
    SimEntity *ptr;
};

// Iterate all not deleted entities
EntityIterator iterate_all_entities(SimRegion *region);
void advance(SimRegion *sim, EntityIterator *iter);

// void load_entity_reference(SimRegion *sim, EntityReference *ref);
// void store_entity_reference(EntityReference *ref);

// All game-related data is stored here. Like player resources, debug thigs etc.
struct GameState {
    MemoryArena arena;
    MemoryArena frame_arena;
    
    World *world;
    u32 wood_count;
};

void world_init(World *world);
void update_and_render_world(GameState *game_state, Input *input, Renderer *renderer, Assets *assets);

#define WORLD_HH 1
#endif
