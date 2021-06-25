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

// Add offset to base_pos and return new position
inline WorldPosition pos_add(WorldPosition base_pos, Vec2 offset);
inline Vec2 distance_between_pos(WorldPosition a, WorldPosition b);
inline bool is_same_chunk(WorldPosition a, WorldPosition b);
inline Vec2 DEBUG_world_pos_to_p(WorldPosition pos);
inline WorldPosition world_position_from_tile_position(Vec2i tile_position);
inline WorldPosition world_position_floor_to_cell(WorldPosition pos);

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
// World object flags
enum {
    WORLD_OBJECT_FLAG_IS_RESOURCE = 0x1,
    WORLD_OJBECT_FLAG_IS_BUILDING = 0x2,
    WORLD_OJBECT_FLAG_SENTINEL
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

struct SimEntity {
    // General data
    Vec2 p;
    EntityID id;
    u8 flags;
    u8 kind;
    // Per-kind values
    u8 world_object_flags;
    u8 world_object_kind;
    u8 resource_kind;
    u8 resource_interactions_left;
    u8 resource_gain;
    // For building
    f32 build_progress; // in work units
    f32 hit_points;
};  

CT_ASSERT(ENUM_FITS_IN_VARIABLE(ENTITY_FLAG_SENTINEL, STRUCT_FIELD(SimEntity, flags)));
CT_ASSERT(ENUM_FITS_IN_VARIABLE(ENTITY_KIND_SENTINEL, STRUCT_FIELD(SimEntity, kind)));
CT_ASSERT(ENUM_FITS_IN_VARIABLE(WORLD_OBJECT_KIND_SENTINEL, STRUCT_FIELD(SimEntity, world_object_kind)));
CT_ASSERT(ENUM_FITS_IN_VARIABLE(WORLD_OJBECT_FLAG_SENTINEL, STRUCT_FIELD(SimEntity, world_object_flags)));
CT_ASSERT(ENUM_FITS_IN_VARIABLE(RESOURCE_KIND_SENTINEL, STRUCT_FIELD(SimEntity, resource_kind)));

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
    // @TODO think if we need this back-pointer
    struct GameState *game_state;
    // So in case player positions are some huge float numbers,
    // map entities from world position to sim position and back after sim end
    WorldPosition origin;
    Vec2i min_chunk;
    Vec2i max_chunk;
    // @CLEAN this is here only because we couldn't do zsort ourselves!
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

enum {
    PLAYER_INTERACTION_KIND_NONE,
    // Make this separate from destroy not to overcomplicate things trying to use less names
    PLAYER_INTERACTION_KIND_MINE_RESOURCE,
    PLAYER_INTERACTION_KIND_DESTROY,
    PLAYER_INTERACTION_KIND_BUILD,
    PLAYER_INTERACTION_KIND_SENTINEL  
};

// All game-related data is stored here. Like player resources, debug thigs etc.
struct GameState {
    MemoryArena arena;
    MemoryArena frame_arena;
    
    SimCamera cam;
    EntityID camera_followed_entity_id;
    // Generated world size
    Vec2i min_chunk;
    Vec2i max_chunk;
    
    World *world;
    u32 wood_count;
    u32 gold_count;
    
    u32 interaction_kind;
    bool is_player_interacting;
    EntityID interactable;
    f32 interaction_time;
    f32 interaction_current_time;
    
    u8 selected_building;
    bool is_planning_building;
    
    size_t DEBUG_last_frame_sim_region_entity_count;
};

void game_state_init(GameState *game_state);
void update_and_render(GameState *game_state, Input *input, Renderer *renderer, Assets *assets);

#define WORLD_HH 1
#endif
