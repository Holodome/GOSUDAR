#if !defined(WORLD_HH)

#include "lib/lib.hh"

#include "game/camera.hh"

typedef u32 EntityID;

#define TILE_SIZE 1.0f
#define TILES_IN_CHUNK 4
#define CHUNK_SIZE (1.0f * TILES_IN_CHUNK)

struct WorldPosition {
    Vec2i chunk; // Chunk coordinate
    Vec2 offset; // Offset in chunk, world coordinate units in [0; CHUNK_SIZE]
};  

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

struct SimEntity {
    // ID in world storage
    EntityID id;
    EntityKind kind;
    u32 flags; // EntityFlags
    // For sim
    Vec2 p;
    AssetID texture_id;
    Vec2 size; // texture size multiplier
    Vec2i tile_pos;
};  

// Entity stored in world
struct Entity {
    SimEntity sim;
    // Stores data of chunk entity is in
    WorldPosition world_pos;
};

struct ChunkEntityBlock {
    u32 entity_count;
    EntityID entity_ids[16];
    ChunkEntityBlock *next;
};  

// World is split into chunks by entity position
// Enitites in chunk are stored in ChunkEntityBlock
// It is linked list with 16 entities in block
struct Chunk {
    ChunkEntityBlock entity_block;
    Vec2i coord;
    
    bool is_initialized;
    Chunk *next_in_hash;
};

struct SimCamera {
    f32 pitch;
    f32 yaw;
    f32 distance_from_player;
};

struct World {
    MemoryArena world_arena;
    MemoryArena *frame_arena;
    // Linked list free entry
    ChunkEntityBlock *first_free;
    // Chunks hash table. Implemented with external collision resolving (other chunks are allocated separately)
    Chunk chunk_hash[128];
    
    size_t max_entity_count;
    size_t entity_count;
    Entity *entities;
    
    SimCamera camera;
    EntityID camera_followed_entity_id;
    
    // Camera camera;
};  

void world_init(World *world);
void world_update(World *world, Input *input);
void world_render(World *world, Renderer *renderer, Assets *assets);
Entity *get_entity(World *world, EntityID id);
Chunk *get_world_chunk(World *world, Vec2i coord);
void change_entity_position(World *world, EntityID id, WorldPosition *old_p, WorldPosition *new_p);

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
    
    SimCamera cam;
    Vec3 cam_p;
    Mat4x4 cam_mvp;
    
    size_t max_entity_count;
    size_t entity_count;
    SimEntity *entities;
};  

SimRegion *begin_sim(MemoryArena *sim_arena, World *world);
void do_sim(SimRegion *sim, Input *input, Renderer *renderer, Assets *assets) ;
void end_sim(SimRegion *region, Input *input);

#define WORLD_HH 1
#endif
