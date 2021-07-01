#if !defined(WORLD_HH)

#include "lib/lib.hh"

#define WORLD_EPSILON 0.001f

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

// @TODO clean
#define CHUNK_COORD_UNINITIALIZED 0x7FFFFFFF
static bool is_chunk_coord_initialized(Vec2i coord) {
    return coord.x != CHUNK_COORD_UNINITIALIZED;
}

// Add offset to base_pos and return new position
inline WorldPosition pos_add(WorldPosition base_pos, Vec2 offset);
inline Vec2 distance_between_pos(WorldPosition a, WorldPosition b);
inline bool is_same_chunk(WorldPosition a, WorldPosition b);
inline Vec2 DEBUG_world_pos_to_p(WorldPosition pos);
inline WorldPosition world_position_from_tile_position(Vec2i tile_position);
inline Vec2 floor_to_cell(Vec2 pos);

struct EntityID {
    u32 value;    
};  

inline EntityID null_id();
inline bool is_same(EntityID a, EntityID b);
inline bool is_not_null(EntityID a);
inline EntityID entity_id_from_storage_index(u32 index);

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

// @TODO if we want to be fancy, we can use depth kind system
// So there will be several kind levels, primary being pawn, world object
// And pawn can be human or animal
// And human can be hostile or friendly or whatever
// This way we can save space on repeating kind fields
// But really this is just a couple of bytes, so should we care?

struct SimEntity {
    // General data
    Vec2 p;
    EntityID id;
    u8 flags;
    u8 kind;
    // Per-kind values
    u8 world_object_flags;
    u8 world_object_kind;
    u8 resource_interactions_left;
    // For building
    f32 build_progress; // [0-1]
};  

CT_ASSERT(ENUM_FITS_IN_VARIABLE(ENTITY_FLAG_SENTINEL, STRUCT_FIELD(SimEntity, flags)));
CT_ASSERT(ENUM_FITS_IN_VARIABLE(ENTITY_KIND_SENTINEL, STRUCT_FIELD(SimEntity, kind)));
CT_ASSERT(ENUM_FITS_IN_VARIABLE(WORLD_OBJECT_KIND_SENTINEL, STRUCT_FIELD(SimEntity, world_object_kind)));

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

struct ChunkIterator {
    EntityBlock *block;
    u32 entity_index;
    EntityID *id;
};  

ChunkIterator iterate_chunk_entities(Chunk *chunk);
bool is_valid(ChunkIterator *iter);
void advance(ChunkIterator *iter);

struct IDListEntry {
    EntityID id;
    IDListEntry *next;  
};

#define WORLD_CHUNK_HASH_SIZE 4096
CT_ASSERT(IS_POW2(WORLD_CHUNK_HASH_SIZE));
// @TODO make world inners inaccessable, use api calls for all
// because it starts to become a mess 
// World is a structure that is responsible for storing entities
// It can be though as entity list, that additionally has spatial partition and uses ids for access
struct World {
    MemoryArena *world_arena;
    // Linked list free entry
    EntityBlock *first_free_entity_block;
    // Chunks hash table. Implemented with external collision resolving (other chunks are allocated separately)
    Chunk chunk_hash[WORLD_CHUNK_HASH_SIZE];
    
    size_t max_entity_count;
    size_t _entity_count;
    Entity *entities;

    IDListEntry *free_id_list;
    IDListEntry *free_id_list_free_entry;
    
    u64 DEBUG_id_list_entries_allocated;
};  

void world_init(World *world);
EntityID get_new_id(World *world);
void free_id(World *world, EntityID id);
// EntityID add_world_entity(World *world, WorldPosition pos);
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
    World *world;
    // So in case player positions are some huge float numbers,
    // map entities from world position to sim position and back after sim end
    WorldPosition origin;
    Vec2i min_chunk;
    Vec2i max_chunk;
    // @TODO this is here only because we couldn't do zsort ourselves!
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
SimRegion *begin_sim(MemoryArena *arena, World *world, Vec2i min_chunk, Vec2i max_chunk);
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
bool is_valid(EntityIterator *iter);
void advance(EntityIterator *iter);

#define WORLD_HH 1
#endif
