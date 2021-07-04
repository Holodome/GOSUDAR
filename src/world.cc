#include "world.hh"
#include "game.hh"

// @TODO do we want to introduce threshold here to avoid pointlessly moving entities when they are at chunk edge
static bool is_canonical(f32 chunk_rel) {
    bool result = ((chunk_rel >= -0.001f) && (chunk_rel <= CHUNK_SIZE + 0.001f));
    return result;
}

static bool is_canonical(Vec2 offset) {
    return is_canonical(offset.x) && is_canonical(offset.y);
}

static void recanonicalize_coord(i32 *chunk, f32 *chunk_rel) {
    i32 offset = (i32)floorf(*chunk_rel / CHUNK_SIZE);
    *chunk += offset;
    *chunk_rel -= offset * CHUNK_SIZE;
    assert(is_canonical(*chunk_rel));
}

WorldPosition pos_add(WorldPosition base_pos, Vec2 offset) {
    WorldPosition result = base_pos;
    result.offset += offset;
    recanonicalize_coord(&result.chunk.x, &result.offset.x);
    recanonicalize_coord(&result.chunk.y, &result.offset.y);
    return result;
}

Vec2 distance_between_pos(WorldPosition a, WorldPosition b) {
    Vec2 dcoord = Vec2(a.chunk - b.chunk);
    Vec2 result = dcoord * CHUNK_SIZE + (a.offset - b.offset);
    return result;
}

bool is_same_chunk(WorldPosition a, WorldPosition b) {
    assert(is_canonical(a.offset) && is_canonical(b.offset));
    return a.chunk == b.chunk;
}

WorldPosition world_position_from_tile_position(Vec2i tile_position) {
    // Tile center, so tiles are placed in chunk_hash correctly
    WorldPosition base_pos = {};
    Vec2 global_pos = (Vec2(tile_position) + Vec2(0.5f)) * TILE_SIZE;
    WorldPosition result = pos_add(base_pos, global_pos);
    return result;
}

Vec2 floor_to_cell(Vec2 pos) {
    pos.x = floorf(pos.x / CELL_SIZE) * CELL_SIZE;
    pos.y = floorf(pos.y / CELL_SIZE) * CELL_SIZE;
    return pos;
}

Vec2 DEBUG_world_pos_to_p(WorldPosition pos) {
    WorldPosition base_pos = {};
    return distance_between_pos(pos, base_pos);    
}

inline EntityID null_id() {
    return {};    
}

inline bool is_same(EntityID a, EntityID b) {
    return a.value == b.value;
}

inline bool is_not_null(EntityID a) {
    return !is_same(a, null_id());
}

inline EntityID entity_id_from_storage_index(u32 index) {
    EntityID result;
    result.value = index;
    return result;    
}

ChunkIterator iterate_chunk_entities(Chunk *chunk) {
    ChunkIterator iter;
    iter.block = &chunk->entity_block;
    iter.entity_index = 0;
    iter.id = iter.block->entity_count ? iter.block->entity_storage_indices : 0;
    return iter;
}

bool is_valid(ChunkIterator *iter) {
    return iter->id != 0;    
}

void advance(ChunkIterator *iter) {
    if (iter->entity_index + 1 < iter->block->entity_count) {
        iter->id = iter->block->entity_storage_indices + ++iter->entity_index;
    } else if (iter->block->next) {
        iter->entity_index = 0;
        iter->block = iter->block->next;
        iter->id = iter->block->entity_count ? iter->block->entity_storage_indices : 0;
    } else {
        iter->id = 0;
    }
}

void world_init(World *world) {
    world->first_free_entity_block = 0;
    world->free_id_list_free_entry = 0;
    world->free_id_list = 0;
    world->_entity_count = 1; // !!!
    world->max_entity_count = 16384;
    world->entities = (Entity *)arena_alloc(world->world_arena, 
											sizeof(Entity) * world->max_entity_count);
    memset(world->chunk_hash, 0, sizeof(world->chunk_hash));
    for (size_t i = 0; i < ARRAY_SIZE(world->chunk_hash); ++i) {
        world->chunk_hash[i].coord = Vec2i(CHUNK_COORD_UNINITIALIZED, 0);
    }
    for (size_t i = 0; i < world->max_entity_count; ++i) {
        world->entities[i].world_pos.chunk.x = CHUNK_COORD_UNINITIALIZED;
    }
}

EntityID get_new_id(World *world) {
    EntityID result;
    if (world->free_id_list) {
        IDListEntry *next_id = world->free_id_list->next;
        result = world->free_id_list->id;
        world->free_id_list->next = world->free_id_list_free_entry;
        world->free_id_list_free_entry = world->free_id_list;
        world->free_id_list = next_id;
    } else {
        result = entity_id_from_storage_index(world->_entity_count++);
    }
    return result;
}

void free_id(World *world, EntityID id) {
    IDListEntry *new_entry;
    if (world->free_id_list_free_entry) {
        new_entry = world->free_id_list_free_entry;
        world->free_id_list_free_entry = world->free_id_list_free_entry->next;
    } else {
        ++world->DEBUG_id_list_entries_allocated;
        new_entry = alloc_struct(world->world_arena, IDListEntry);
    }
    new_entry->id = id;
    new_entry->next = world->free_id_list;
    world->free_id_list = new_entry;
}

Entity *get_world_entity(World *world, EntityID id) {
    // @TODO clean, make 0 id invalid
    assert(id.value);
    assert(id.value < world->_entity_count);
    return world->entities + id.value;    
}

Chunk *get_world_chunk(World *world, Vec2i coord) {
    TIMED_FUNCTION();
    if (!is_chunk_coord_initialized(coord)) {
        return 0;
    }
    u32 hash_value = coord.x * 123123 + coord.y * 1891289 + 121290;    
#if 1 
    xorshift32(&hash_value);
#endif 
    
#if 0
    u32 hash_slot = hash_value & (ARRAY_SIZE(world->chunk_hash) - 1);
    
    Chunk *chunk = world->chunk_hash + hash_slot;
    for (;;) {
        if (coord == chunk->coord) {
            break;
        } 
        
		if (!is_chunk_coord_initialized(chunk->coord)) {
            chunk->coord = coord;
            break;
        }
        if (!chunk->next_in_hash) {
            ++world->DEBUG_external_chunks_allocated;
            chunk->next_in_hash = (Chunk *)arena_alloc(world->world_arena, sizeof(Chunk));
        }
        if (chunk->next_in_hash) {
            chunk = chunk->next_in_hash;
        }
    }
    return chunk;    
#else 
    Chunk *chunk = 0;
    u32 hash_mask = ARRAY_SIZE(world->chunk_hash) - 1;
    u32 hash_slot = hash_value & hash_mask;
    for (size_t offset = 0; offset < ARRAY_SIZE(world->chunk_hash); ++offset) {
        u32 hash_index = ((hash_value + offset) & hash_mask);
        Chunk *test = world->chunk_hash + hash_index;
        if (!is_chunk_coord_initialized(test->coord)) {
            test->coord = coord;
            chunk = test;
            break;
        } else if (test->coord == coord) {
            chunk = test;
            break;
        }
    }
    return chunk;
#endif 
}

bool remove_entity_from_chunk(World *world, Chunk *chunk, EntityID id) {
    EntityBlock *first_block = &chunk->entity_block;
    bool not_found = true; 
    for (EntityBlock *entity_block = first_block;
		 entity_block && not_found;
		 entity_block = entity_block->next) {
        for (size_t entity_idx = 0; entity_idx < entity_block->entity_count && not_found; ++entity_idx) {
            if (is_same(entity_block->entity_storage_indices[entity_idx], id)) {
                assert(first_block->entity_count > 0);
                entity_block->entity_storage_indices[entity_idx] = first_block->entity_storage_indices[--first_block->entity_count];  
                if (first_block->entity_count == 0) {
                    if (first_block->next) {
                        EntityBlock *free_block = first_block->next;
                        *first_block = *free_block;
                        
                        entity_block->next = world->first_free_entity_block;
                        world->first_free_entity_block = free_block;
                    }
                } 
                
                not_found = false;   
            }
        }
    }    
    
    return !not_found;
}

void add_entity_to_chunk(World *world, Chunk *chunk, EntityID id) {
    EntityBlock *entity_block = &chunk->entity_block;
    if (entity_block->entity_count == ARRAY_SIZE(entity_block->entity_storage_indices)) {
        EntityBlock *new_block = world->first_free_entity_block;
        if (new_block) {
            world->first_free_entity_block = world->first_free_entity_block->next;
        } else {        
            new_block = (EntityBlock *)arena_alloc(world->world_arena, sizeof(EntityBlock));
        }
        *new_block = *entity_block;
        entity_block->next = new_block;
        entity_block->entity_count = 0;
    }
    assert(entity_block->entity_count < ARRAY_SIZE(entity_block->entity_storage_indices));
    entity_block->entity_storage_indices[entity_block->entity_count++] = id;
}
void move_entity(World *world, EntityID id, WorldPosition to, WorldPosition from) {
    if (to.chunk != from.chunk) {
        Chunk *old_chunk = get_world_chunk(world, from.chunk);
        if (old_chunk) {
            remove_entity_from_chunk(world, old_chunk, id);
        }
        Chunk *new_chunk = get_world_chunk(world, to.chunk);
        add_entity_to_chunk(world, new_chunk, id);
    }
    Entity *entity = get_world_entity(world, id);
    entity->world_pos = to;
}

SimRegion *begin_sim(MemoryArena *arena, World *world, Vec2i min_chunk, Vec2i max_chunk) {
    TIMED_FUNCTION();
    SimRegion *sim = alloc_struct(arena, SimRegion);
    sim->world = world;
    sim->max_entity_count = 4096;
    sim->entity_count = 0;
    sim->entities = alloc_arr(arena, sim->max_entity_count, SimEntity);
    
    sim->min_chunk = min_chunk;
    sim->max_chunk = max_chunk;
    sim->origin.chunk = sim->min_chunk;
    sim->origin.offset = Vec2(0);
    
    for (i32 chunk_y = min_chunk.y; chunk_y <= max_chunk.y; ++chunk_y) {
        for (i32 chunk_x = min_chunk.x; chunk_x <= max_chunk.x; ++chunk_x) {
            Vec2i chunk_coord = Vec2i(chunk_x, chunk_y);
            // Maybe if chunk has zero entities we delete it, this way we can dont worry about chunk regions 
            // in sim region. Let the update code handle where entity can go.
            // So this way we could make portals or whatever in far away chunks
            Chunk *chunk = get_world_chunk(world, chunk_coord);
            for (ChunkIterator iter = iterate_chunk_entities(chunk);
                 is_valid(&iter);
                 advance(&iter)) {
                EntityID entity_id = *iter.id;
                add_entity(sim, entity_id);
            }        
        }
    }
    DEBUG_VALUE(sim->entity_count, "Sim entity count");
    DEBUG_VALUE(sim->world->_entity_count, "World entity count");
    DEBUG_VALUE(sim->world->DEBUG_id_list_entries_allocated, "Unused id list entries");
    
    return sim;
}

void end_sim(SimRegion *sim) {
    TIMED_FUNCTION();
    for (size_t entity_idx = 0; entity_idx < sim->entity_count; ++entity_idx) {
        SimEntity *entity = sim->entities + entity_idx;
        bool do_free_id = false;
        if (entity->flags & ENTITY_FLAG_SINGLE_FRAME_LIFESPAN) {
            do_free_id = true;
        } else {
            // Place entity in world 
            WorldPosition new_position = pos_add(sim->origin, entity->p);
            // if (is_same(entity->id, null_id()) && !(entity->flags & ENTITY_FLAG_IS_DELETED)) {
            //     // entity->id = add_world_entity(sim->world, new_position);
            //     Entity *world_ent = get_world_entity(sim->world, entity->id);
            //     world_ent->sim = *entity;
            // } else {
            Entity *world_ent = get_world_entity(sim->world, entity->id);
            if (entity->flags & ENTITY_FLAG_IS_DELETED) {
                Chunk *old_chunk = get_world_chunk(sim->world, world_ent->world_pos.chunk);
                remove_entity_from_chunk(sim->world, old_chunk, entity->id);
                do_free_id = true;
            } else {
                world_ent->sim = *entity;
                move_entity(sim->world, entity->id, new_position, world_ent->world_pos);
            }
        }
        
        if (do_free_id) {
            free_id(sim->world, entity->id);
        }
    }
}

SimEntityHash *get_hash_from_storage_index(SimRegion *sim, EntityID entity_id) {
    TIMED_FUNCTION();
    SimEntityHash *result = 0;
    u32 hash_value = entity_id.value;
    u32 hash_mask = (ARRAY_SIZE(sim->entity_hash) - 1);
    for (size_t offset = 0; offset < ARRAY_SIZE(sim->entity_hash); ++offset) {
        u32 hash_index = ((hash_value + offset) & hash_mask);
        SimEntityHash *entry = sim->entity_hash + hash_index;
        if (is_same(entry->id, null_id()) || is_same(entry->id, entity_id)) {
            result = entry;
            break;
        }
    }
    return result;
}

SimEntity *get_entity_by_id(SimRegion *sim, EntityID entity_id) {
    SimEntityHash *hash = get_hash_from_storage_index(sim, entity_id);
    SimEntity *result = hash->ptr;
    return result;
}

SimEntity *add_entity(SimRegion *sim, EntityID entity_id) {
    Entity *world_ent = get_world_entity(sim->world, entity_id);
    
    assert(!is_same(entity_id, null_id()));
    SimEntity *entity = 0;
    SimEntityHash *entry = get_hash_from_storage_index(sim, entity_id);
    if (entry->ptr == 0) {
        assert(sim->entity_count < sim->max_entity_count);
        entity = sim->entities + sim->entity_count++;
        
        entry->id = entity_id;
        entry->ptr = entity;
        
        *entity = world_ent->sim;   
        
        entity->id = entity_id;
    } else {
		entity = entry->ptr;
	}
    
    entity->p = distance_between_pos(world_ent->world_pos, sim->origin);
    return entity;
}

// @NOTE fix that we can't get this entity by id until next frame
SimEntity *create_entity(SimRegion *sim) {
    assert(sim->entity_count < sim->max_entity_count);
    SimEntity *entity = sim->entities + sim->entity_count++;
    memset(entity, 0, sizeof(*entity));
    entity->id = get_new_id(sim->world);
    SimEntityHash *hash = get_hash_from_storage_index(sim, entity->id);
    hash->id = entity->id;
    hash->ptr = entity;

    return entity;
}

static void set_entity_to_next_not_deleted(EntityIterator *iter) {
    while ((iter->idx < iter->sim->entity_count) && (iter->sim->entities[iter->idx].flags & ENTITY_FLAG_IS_DELETED)) {
        ++iter->idx;
    }
    
    if (iter->idx < iter->sim->entity_count) {
        iter->ptr = iter->sim->entities + iter->idx;
    } else {
        iter->ptr = 0;
    }
}

EntityIterator iterate_all_entities(SimRegion *sim) {
    EntityIterator iter;
    iter.sim = sim;
    iter.idx = 0;
    set_entity_to_next_not_deleted(&iter);
    return iter;
}

bool is_valid(EntityIterator *iter) {
    return iter->ptr != 0;    
}

void advance(EntityIterator *iter) {
    ++iter->idx;
    set_entity_to_next_not_deleted(iter);
}
