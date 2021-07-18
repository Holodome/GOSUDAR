#if !defined(SIM_REGION_HH)

#include "world.hh"

// All values inside spatial occupancy are in range 0-15 
// So each of its values can be represented as half a byte
// And this structure could take twice less size
// But it seems we don't care, since differency in even big (game-sence) sim region will be a couple of
// kilobytes
struct SpatialOccupancy {
    u8 x;
    u8 y;
    u8 r;
};  

struct SimRegionChunkEntityBlock {
    u8 entity_count;
    // @TODO we may want to choose bigger number here,
    // since sim region is rather dense then sparse
    EntityID ids[ENTITIES_IN_BLOCK];  
    SimRegionChunkEntityBlock *next;
    // @TODO here we can place all kinds of acceleration structures - like
    // storing entity flags, their locations and whatever else game may need
    // to access entity for strictly reading
};

struct SimRegionChunk {
    // Sim space chunk position
    i32 chunk_x;
    i32 chunk_y;
    // During simulation game makes a lot of calls to check if 
    // given cell in the world is considred empty - so we store 
    // occupancy data additionally to entities
    // u16 spatial_occupancy_count;
    // SpatialOccupancy spatial_occupancy[CELLS_IN_CHUNK * CELLS_IN_CHUNK / 2];
    SimRegionChunkEntityBlock first_block;  
};  

struct SimRegionEntityHash {
    Entity *ptr;
    EntityID id;
};

// World is split in simulation regions during updating
// This game wants ot update all of its regions with equal percision - 
// so AI plays seem natural and alike real player 
//
// Sim region can be viewed as a joining layer between the simulation step and world storage
// It does no gameplay-related stuff, only collects entities that game requests
//
// Sim region is defined by anchor - in each frame some entities in
// the world are picked to define sim region - this can be player, 
// enemy boss or something else
// If sim regions of different anchors overlap, they are joined to do single sim region
// @TODO for now if two circles touch each other they are made to be single big circle 
// which is actually a huge wast of memory on parts of the world that were not designed to sim (up to 8 times)
// we need a way to have different sim regions be joined in other way
//
// Each different sim region is independent from others - and they can be simulated on different threads
//
// There are problems with how sim regions work
// Basically, we can't properly simulate the borders of the sim - because we know nothing about entities 
// that are located outside of it
// So entities should avoid getting closer to sim borders - or become anchors if 
// their job is important
// For player-simulated region we need to choose size big enough so player doesn't notice stupid AI 
// around borders
//
// Data stored in sim region should be sufficient for simulation - meaning there is no 
// need for memore allocations and world access during the frame
//
// All world changes happen inside sim region
// @TODO So generation will happen here too, and when we do world generation it needs to cpecify 
// generated part. But it is complicated to divide world, since sim regions are not rectangular based
struct SimRegion {
    MemoryArena *arena;
    // Needed to create ids for new entities
    World *world;
    // Sim region is defined as circle - by center and radius
    // Actually this is rhombus - there is no need in full circle
    i32 center_chunk_x;
    i32 center_chunk_y;
    u32 chunk_radius;
    // In contrast to the world, where entities are stored in per-chunk basis, 
    // in sim region they are stored in single array 
    //
    // Max entity count can be defined from sim region radius, if we know in general 
    // how many entities can be it single chunk
    // But it is better not to push this number too low, since we don't want to 
    // meet some memory limitation during the game process
    u64 max_entity_count;
    u64 entity_count;
    Entity *entities;
    // Hash table used to retrieve entity in sim region from its id
    // size of max_entity_count
    SimRegionEntityHash *entity_hash;
    // Entities in sim are still spatially partotioned - 
    // even sim regions are a spatial partition itself, it is not enough for 
    // game needs
    // Chunks are a tool to iterate over entities in certain radius rather than 
    // primary entity storage key
    SimRegionChunk *chunks;
    u32 chunks_count;
    
    u32 entity_blocks_allocated;
    SimRegionChunkEntityBlock *first_free_entity_block;
    
    u32 missing_entity_space;
};

#define MAX_ANCHORS 32
// Anchor is some object in world that has its own simulation region
// So due to game limitations we want to have different distance parts of the world simulated,
// but we want only simulate parts that interest us
// After simulation is ended, all anchor entities are written in world, 
// so in next simulation begin we know what parts of the world to simulate
struct Anchor {
    i32 chunk_x;
    i32 chunk_y;
    u32 radius;
};

void p_to_chunk_coord(vec2 p, i32 *chunk_x, i32 *chunk_y, vec2 *chunk_p_dst = 0);
vec2 get_sim_space_p(SimRegion *sim, i32 chunk_x, i32 chunk_y, vec2 chunk_p);
void get_global_space_p(SimRegion *sim, vec2 p, i32 *chunk_x_dst, i32 *chunx_y_dst, vec2 *chunk_p_dst);
void get_chunk_coord_from_cell_coord(i32 cell_x, i32 cell_y, i32 *chunk_x_dst, i32 *chunk_y_dst);
u32 get_chunk_count_for_radius(u32 radius);
// This is related to how we store chunks in sim region
// We store them as rhombus - it uses twice less space than storing rectangle,
// corners of which are so distant from center that they should not be updated
// This functions map from chunk position to its storage index
bool chunk_array_index_to_coord(u32 radius, u32 idx, i32 *dx, i32 *dy);
u32 chunk_array_index(u32 radius, i32 dx, i32 dy);
// Returns chunk if it inside sim region
SimRegionChunk *get_chunk(SimRegion *sim, u32 chunk_x, u32 chunk_y);
bool remove_entity_from_chunk(SimRegion *sim, SimRegionChunk *chunk, EntityID id);
void add_entity_to_chunk(SimRegion *sim, SimRegionChunk *chunk, EntityID id);
// Returns 0 if entity of given id does not exist in sim
SimRegionEntityHash *get_entity_hash(SimRegion *sim, EntityID id);
Entity *get_entity_by_id(SimRegion *sim, EntityID id);
// Allocates storage for new entity and puts it into chunk inside sim
Entity *create_new_entity(SimRegion *sim, vec2 p, Entity *src = 0);
// Sets entity position to p and moves it into new chunk
// This is somewhat slow to modify position in that way, 
// but it is more expensive not to use chunks either way
// So position modifications during frame should be of minimal count
void change_entity_position(SimRegion *sim, Entity *entity, vec2 p);
// All cell coordinates are sim space, basically floored position
// @TODO this is very slow function - we can cache its results or 
// create some structure to accelerate checking
bool is_cell_occupied(SimRegion *sim, i32 cell_x, i32 cell_y);
// Objects must have at least single cell in between them - 
// check if object can be placed
bool check_spatial_placement(SimRegion *sim, i32 cell_x, i32 cell_y, u32 radius);

// We pass sim as argument because its allocation is done in world_state - 
// it ususally allocates sims as number of anchors
// But arena passed here is used to allocate all sim arrays
// Loads world chunks in given radius around center point
// All entities from world are decompressed and are made ready for simulation
void begin_sim(SimRegion *sim, MemoryArena *arena, World *world,
               u32 center_x, u32 center_y, u32 chunk_radius);
// Iterates over entities inside region and packs them back to world
void end_sim(SimRegion *sim, struct WorldState *world_state);

struct SimChunkIterator {
    SimRegion *sim;
    i32 min_chunk_x;
    i32 min_chunk_y;
    i32 max_chunk_x;
    i32 max_chunk_y;
    u32 idx;
    SimRegionChunk *ptr;
};

// Iterates only chnuks that are inside defined circle 
// Internally it does rect-based iteration, calculating points that define outer rect
// of given circle
// This will return some unwanted chunks if radius gets big
SimChunkIterator iterate_sim_chunks_in_radius(SimRegion *sim, vec2 p, f32 radius);
SimChunkIterator iterate_sim_chunks(SimRegion *sim, i32 min_chunk_x, i32 min_chunk_y, i32 max_chunk_x, i32 max_chunk_y);

// @TODO we way want to add chunk-based circle iteration,
// or rectangular iteration
// @TODO this structure can be used in construction of sim regions, where 
bool is_valid(SimChunkIterator *iter);
void advance(SimChunkIterator *iter);

// 
// Iterate entities in chunk
struct SimChunkEntityIterator {
    SimRegionChunkEntityBlock *block;
    u32 entity_idx;
    EntityID *ptr;
};

SimChunkEntityIterator iterate_chunk_entities(SimRegionChunk *chunk);
bool is_valid(SimChunkEntityIterator *iter);
void advance(SimChunkEntityIterator *iter);

// Tool to iterate entities inside sim region
// It is quite easy to create all this (useless) iterators now, so why not do it?
enum {
    ENTITY_ITERATOR_DISTANCE_BASED = 0x1,   
    ENTITY_ITERATOR_FLAG_BASED     = 0x2,   
    ENTITY_ITERATOR_KIND_BASED     = 0x4,  
};

struct EntityIteratorSettings {
    u8 flags;
    
    vec2 origin;
    f32 radius;
    u32 flag_mask;
    i32 kind;
};  

EntityIteratorSettings iter_radius(vec2 origin, f32 radius);

struct EntityIterator {
    SimRegion *sim;
    EntityIteratorSettings settings;
    SimChunkIterator chunk_iterator;
    SimChunkEntityIterator chunk_entity_iterator;
    
    EntityID *ptr;
};

// @TODO iterate using chunks
EntityIterator iterate_entities(SimRegion *sim, EntityIteratorSettings settings);
bool is_valid(EntityIterator *iter);
void advance(EntityIterator *iter);

#define SIM_REGION_HH 1
#endif
