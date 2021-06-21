#if !defined(WORLD_HH)

#include "lib/lib.hh"

#include "game/camera.hh"

typedef u32 EntityID;

#define CHUNK_SIZE 16.0f
#define TILES_IN_CHUNK 16
#define TILE_SIZE (CHUNK_SIZE / (f32)TILES_IN_CHUNK)

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
    AssetID texture_id;
    Vec2 size; // texture size multiplier
    Vec2 p;    // 
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
    
    EntityID player_id;
    Camera camera;
};  

void world_init(World *world);
void world_update(World *world, Input *input);
void world_render(World *world, Renderer *renderer, Assets *assets);
Entity *get_entity(World *world, EntityID id);
Chunk *get_world_chunk(World *world, Vec2i coord);
void change_entity_position(World *world, EntityID id, WorldPosition *old_p, WorldPosition *new_p);

struct SimRegion {
    MemoryArena *frame_arena;
    World *world;
    
    size_t max_entity_count;
    size_t entity_count;
    SimEntity *entities;
};  

SimRegion *begin_sim(MemoryArena *sim_arena, World *world);
void do_sim(SimRegion *sim, Input *input, Renderer *renderer, Assets *assets) ;
void end_sim(SimRegion *region);

#define WORLD_HH 1
#endif
