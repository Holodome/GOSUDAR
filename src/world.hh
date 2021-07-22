#if !defined(WORLD_HH)

#include "lib.hh"

#include "entity.hh"

//
// Game world storage
// Game world is designed to be huge in size - infinite for game purposes
// Of course simulation complexity will not allow processing infinitely-sized world,
// but we want to be able to be contrained only by machine computing abilities and
// not artifitially in game architecture
//
// Game world is split into chunks for internal storage, but chunks could also be a measure of distance
// World allows (1 << 32 - 1) chunks in x and y dimension, where each chunk consists of 16 cells, 
// each of which is roughly 0.5 meters in size
// So dimensions of square world are (4294967295 * 16 * 0.5)m = 34359738.36 km 
// And surface area of the world is 1_180_591_620_167_655.5 square kilometers = 1 * 10^15 kilometers - which is 
// thousand times more than surface area of the sun
// Of course no game will last long enough to be even reach the border of the world this big, and we have no limitations in design

// Define to 16 so we can use less space on cell occupancy checks - see SpatialOccupancyBlock
// Basically we have sparse matrix that we need to contruct each time we do a
// check on placing object in the world or collision detection
// Each chunk defines its own matrix part, where each entry can have size in cells less than size of the chunk
// that way when we want to do test on space aviability we can look only in nearby chunks, decompress matrix 
// data and do check on objects intersection
//
// Objects in the world must have one cell in between them and can have sizes from 1 to CELLS_IN_CHUNK - 1
#define CELLS_IN_CHUNK 16 
// Hash of world chunk storage
// This hash table has external collision resolving and we have no idea how many chunks of 
// its storage are actually used - so from time to time game needs to do a safe and unload
// unused chunks into file for storing
// This number should be big enough to support several sim regions across the world
// to act with decent perfomance
#define WORLD_CHUNK_HASH_SIZE 1024
// Entities in world chunks are stored in linked lists by 16 entries
// basically we want to do as small number of this list iterations as possible 
// so this number should be more than usual number of entities in chunk
// When we actually do profiling, we can decide that several entries of different size in list 
// act the same in terms of perfomance and consume less memory - we can switch this number
#define ENTITIES_IN_BLOCK 16
// Entities in the world are stored compresed - and uncompressed when we need to use it
// Each entity block contains compressed data for its entities, but since compressed entity
// sizes can differ, we need to choose number small enough to fit reasonable amount of medium-sized entities
// 
// Later we will actually do compression on entities when writing to world - 
// data size will grow smaller
#define WORLD_CHUNK_ENTITY_DATA_SIZE (sizeof(Entity) * ENTITIES_IN_BLOCK)
// Cell size in arbitrary game units - basically cell is half a meter, but we define 
// it to one so it is easy to do operations on spatial partitions and cell arithmetic
#define CELL_SIZE 1.0f
#define CHUNK_SIZE (CELL_SIZE * CELLS_IN_CHUNK)

struct WorldChunkEntityBlock {
    u8 entity_count;
    EntityID ids[ENTITIES_IN_BLOCK];
    // @TODO u32 offset[ENTITIES_IN_BLOCK]; - when we do compression step
    // Compressed entity data
    u32 entity_data_size;
    u8 entity_data[WORLD_CHUNK_ENTITY_DATA_SIZE];
    
    WorldChunkEntityBlock *next;
};

struct WorldChunk {
    i32 chunk_x;
    i32 chunk_y;
    
    WorldChunkEntityBlock *first_entity_block;
    
    WorldChunk *next;
};

struct WorldIDListEntry {
    EntityID id;
    WorldIDListEntry *next;
};

//
// @TODO there should be separate notion about world structure
// When we do generation, primary key iterest objects needs to be stored somehow
// So for example if enemy AI wants to get something that does not exist in
// its near environment, it nees to look in some greater radius in world
// So there should be another partition unit - something like WorldPart 
// that has chunks and some additional info
// But chunks in world part can be sparse or dence - in is unclear 
// what is the optimal way of storing chunks when using world parts
//
struct World {
    MemoryArena arena;
    // This is actually what sim regions need pointer to world - 
    // they want to be able to create unique ids even when multithreading
    // So when one thread want to get new ids it needs to lock world one
    u32 max_entity_id;
    // Stores free list for entity ids
    // We don't know how many entities there will be in the world and how many of them are going to 
    // be deleted, so this may go away some time
    WorldIDListEntry *first_id;
    
    WorldChunkEntityBlock *first_free_entity_block;
    WorldChunk *first_free_chunk;
    WorldIDListEntry *first_free_id;
    
    WorldChunk *chunk_hash[WORLD_CHUNK_HASH_SIZE];
    // In future we may want to do hot chunks - entity data will not be decomprssed and 
    // stored if it is considered hot 
    //
    // Statistics
    //
    u32 entity_blocks_allocated;
    u32 chunks_allocated;
    u32 entity_ids_allocated;
};

World *world_init();

WorldChunk *get_world_chunk(World *world, i32 chunk_x, i32 chunk_y);
// Returns world chunk and removes it from storage - since all 
// entities are written in sime region we don't need this chunk for anything else
// until entities are written again
WorldChunk *remove_world_chunk(World *world, i32 chunk_x, i32 chunk_y);
// Adds entity into world chunk
void pack_entity_into_chunk(World *world, WorldChunk *chunk, Entity *src);
void pack_entity_into_world(World *world, i32 chunk_x, i32 chunk_y, Entity *src);
WorldChunk *get_new_chunk(World *world);
WorldChunkEntityBlock *get_new_entity_block(World *world);
void add_chunk_to_free_list(World *world, WorldChunk *chunk);
void add_entity_block_to_free_list(World *world, WorldChunkEntityBlock *block);

EntityID get_new_id(World *world);
void add_id_to_free_list(World *world, EntityID id);

#define WORLD_HH 1
#endif
