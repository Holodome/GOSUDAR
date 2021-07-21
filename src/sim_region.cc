#include "sim_region.hh"
#include "world_state.hh"

void p_to_chunk_coord(vec2 p, i32 *chunk_x_dst, i32 *chunk_y_dst, vec2 *chunk_p_dst) {
    i32 chunk_x = Floor(p.x / CHUNK_SIZE);
    i32 chunk_y = Floor(p.y / CHUNK_SIZE);
    *chunk_x_dst = chunk_x;
    *chunk_y_dst = chunk_y;
    if (chunk_p_dst) {
        *chunk_p_dst = p - Vec2(chunk_x, chunk_y) * CHUNK_SIZE;
    }
}

vec2 get_sim_space_p(SimRegion *sim, i32 chunk_x, i32 chunk_y, vec2 chunk_p) {
    i32 dchx = chunk_x - sim->center_chunk_x;
    i32 dchy = chunk_y - sim->center_chunk_y;
    vec2 result = Vec2(dchx, dchy) * CHUNK_SIZE + chunk_p;   
    return result;
}

void get_global_space_p(SimRegion *sim, vec2 p, i32 *chunk_x_dst, i32 *chunk_y_dst, vec2 *chunk_p_dst) {
    i32 local_chunk_x, local_chunk_y;
    p_to_chunk_coord(p, &local_chunk_x, &local_chunk_y, chunk_p_dst);
    *chunk_x_dst = sim->center_chunk_x + local_chunk_x;
    *chunk_y_dst = sim->center_chunk_y + local_chunk_y;
}

void get_chunk_coord_from_cell_coord(i32 cell_x, i32 cell_y, i32 *chunk_x_dst, i32 *chunk_y_dst) {
    *chunk_x_dst = Floor((f32)cell_x / CELLS_IN_CHUNK);
    *chunk_y_dst = Floor((f32)cell_y / CELLS_IN_CHUNK);
}

u32 get_chunk_count_for_radius(u32 radius) {
    return 1 + 4 * radius + 4 * (((1 + (radius - 1)) * (radius - 1)) >> 1);
}

bool chunk_array_index_to_coord(u32 radius, u32 idx, i32 *dx, i32 *dy) {
    bool result = false;
    
    u32 horiz_offset = 0;
    u32 vert_offset = 2 * radius + 1;
    u32 corner_offset = 4 * radius + 1;
    u32 corner_size = ((1 + (radius - 1)) * (radius - 1)) >> 1;
    if (idx < vert_offset) {
        *dy = 0;
        *dx = (i32)idx - radius;
        result = true;
    } else if (idx < corner_offset) {
        *dx = 0;
        u32 local_idx = idx - vert_offset;
        if (local_idx < radius) {
            *dy = local_idx - radius;
        } else {
            *dy = local_idx - radius + 1;
        }
        result = true;
    } else {
        u32 local_idx = idx - corner_offset;
        u32 quarter = local_idx / corner_size;
        if (quarter < 4) {
            u32 quarter_dx, quarter_dy;
            u32 quarter_idx = local_idx % corner_size;
            for (u32 row_length = radius - 1; row_length > 0; --row_length) {
                if (quarter_idx < row_length) {
                    quarter_dx = quarter_idx % row_length;
                    quarter_dy = radius - 1 - row_length;
                    break;
                }
                quarter_idx -= row_length;
            }
            
            if (quarter == 0) {
                *dx = quarter_dx + 1;
                *dy = quarter_dy + 1;
            } else if (quarter == 1) {
                *dx = -(i32)(quarter_dx + 1);
                *dy = quarter_dy + 1;
            } else if (quarter == 2) {
                *dx = -(i32)(quarter_dx + 1);
                *dy = -(i32)(quarter_dy + 1);
            } else {
                *dx = quarter_dx + 1;
                *dy = -(i32)(quarter_dy + 1);
            }
        }
    }
    return result;
}

u32 chunk_array_index(u32 radius, i32 dx, i32 dy) {
    u32 horiz_offset = 0;
    u32 vert_offset = 2 * radius + 1;
    u32 corner_offset = 4 * radius + 1;
    u32 corner_size = ((1 + (radius - 1)) * (radius - 1)) >> 1;
    u32 result = (u32)-1;
    if (radius && Abs(dx) + Abs(dy) <= radius) {
        if (dy == 0) {
            result = dx + radius;
        } else if (dx == 0) {
            if (dy < 0) {
                result = vert_offset + dy + radius;
            } else {
                result = vert_offset + dy + radius - 1;
            }
        } else {
            u32 quarter = 0;
            if (dx > 0 && dy > 0) {
                quarter = 0;
            } else if (dx < 0 && dy > 0) {
                quarter = 1;
            } else if (dx < 0 && dy < 0) {
                quarter = 2;
            } else {
                quarter = 3;
            }
            u32 current_corner_offset = corner_offset + quarter * corner_size;
            
            u32 local_dx = Abs(dx) - 1;
            u32 local_dy = Abs(dy) - 1;
            if (local_dy < radius - 1) {
                u32 row_offset = ((radius - 1) + (radius - 1 - local_dy + 1)) * (local_dy) / 2;
                if (local_dx < (radius - 1 - local_dy)) {
                    result = current_corner_offset + row_offset + local_dx;
                }
            }
        }
    }
    return result;
}


SimRegionChunk *get_chunk(SimRegion *sim, i32 chunk_x, i32 chunk_y) {
    SimRegionChunk *result = 0; 
    
    u32 index = chunk_array_index(sim->chunk_radius, chunk_x, chunk_y);
    if (index != (u32)-1) {
        assert(index < sim->chunks_count);
        result = sim->chunks + index;
    }
    return result;
}

bool remove_entity_from_chunk(SimRegion *sim, SimRegionChunk *chunk, EntityID id) {
    SimRegionChunkEntityBlock *first_block = &chunk->first_block;
    bool not_found = true;
    for (SimRegionChunkEntityBlock *block = first_block;
         block && not_found;
         block = block->next) {
        for (size_t entity_idx = 0; entity_idx < block->entity_count; ++entity_idx) {
            if (IS_SAME(block->ids[entity_idx], id)) {
                assert(first_block->entity_count);
                block->ids[entity_idx] = first_block->ids[--first_block->entity_count];
                if (first_block->entity_count == 0) {
                    if (first_block->next) {
                        SimRegionChunkEntityBlock *free_block = first_block->next;
                        *first_block = *free_block;
                        LLIST_ADD(sim->first_free_entity_block, free_block);
                    }
                }
            }
        }        
    }
    return !not_found;
}

void add_entity_to_chunk(SimRegion *sim, SimRegionChunk *chunk, EntityID id) {
    SimRegionChunkEntityBlock *first_block = &chunk->first_block;
    if (first_block->entity_count == ARRAY_SIZE(first_block->ids)) {
        SimRegionChunkEntityBlock *new_block = sim->first_free_entity_block;
        if (!new_block) {
            ++sim->entity_blocks_allocated;
            new_block = alloc_struct(sim->arena, SimRegionChunkEntityBlock);
        } else {
            LLIST_POP(sim->first_free_entity_block);
        }
        
        *new_block = *first_block;
        first_block->next = new_block;
        first_block->entity_count = 0;
    }
    
    assert(first_block->entity_count < ARRAY_SIZE(first_block->ids));
    first_block->ids[first_block->entity_count++] = id;
}

SimRegionEntityHash *get_entity_hash(SimRegion *sim, EntityID id) {
    SimRegionEntityHash *result = 0;
    u32 hash_value = id.value;
    u32 hash_mask = sim->max_entity_count - 1;
    assert(is_power_of_two(hash_mask + 1));
    for (size_t offset = 0; offset < sim->entity_count; ++offset) {
        u32 hash_idx = ((hash_value + offset) & hash_mask);
        SimRegionEntityHash *entry = sim->entity_hash + hash_idx;
        if (IS_NULL(entry->id) || IS_SAME(entry->id, id)) {
            ++sim->entity_hash_hits;
            sim->entity_hash_misses += offset;
            
            result = entry;
            break;
        }
    }
    
    return result;
}

Entity *get_entity_by_id(SimRegion *sim, EntityID id) {
    Entity *result = 0;
    SimRegionEntityHash *hash = get_entity_hash(sim, id);
    if (hash) {
        result = hash->ptr;
    }
    
    return result;
}

Entity *create_new_entity_internal(SimRegion *sim, EntityID id) {
    Entity *result = 0;
    if (sim->entity_count + 1 < sim->max_entity_count) {
        Entity *entity = sim->entities + sim->entity_count++;
        entity->id = id;
    }
    return result;
}

Entity *create_new_entity(SimRegion *sim, vec2 p, Entity *src) {
    Entity *result = 0;
    if (sim->entity_count + 1 < sim->max_entity_count) {
        Entity *entity = sim->entities + sim->entity_count++;
        result = entity;
        if (src) {
            *entity = *src;
        } else {
            EntityID id = get_new_id(sim->world);
            entity->id = id;
        }
        
        entity->p = p;
        // Attempt to add to chunk
        i32 chunk_x, chunk_y; 
        p_to_chunk_coord(p, &chunk_x, &chunk_y);
        SimRegionChunk *chunk = get_chunk(sim, chunk_x, chunk_y);
        if (chunk) {
            add_entity_to_chunk(sim, chunk, entity->id);  
        }
        // Add to hash
        SimRegionEntityHash *hash = get_entity_hash(sim, entity->id);
        if (hash) {
            assert(!hash->ptr);
            hash->id = entity->id;
            hash->ptr = entity;
        }
    } 
    return result;
}

void change_entity_position(SimRegion *sim, Entity *entity, vec2 p) {
    TIMED_FUNCTION();
    i32 old_chunk_x, old_chunk_y;
    p_to_chunk_coord(entity->p, &old_chunk_x, &old_chunk_y);
    i32 new_chunk_x, new_chunk_y;
    p_to_chunk_coord(p, &new_chunk_x, &new_chunk_y);
    if (old_chunk_x != new_chunk_x || old_chunk_y != new_chunk_y) {
        SimRegionChunk *old_chunk = get_chunk(sim, old_chunk_x, old_chunk_y);
        if (old_chunk) {
            remove_entity_from_chunk(sim, old_chunk, entity->id);    
        }
        SimRegionChunk *new_chunk = get_chunk(sim, new_chunk_x, new_chunk_y);
        if (new_chunk) {
            add_entity_to_chunk(sim, new_chunk, entity->id);
        }
    }
    
    entity->p = p;
}

bool is_cell_occupied(SimRegion *sim, i32 cell_x, i32 cell_y) {
    TIMED_FUNCTION();
    i32 min_cell_chunk_x, min_cell_chunk_y;
    get_chunk_coord_from_cell_coord(cell_x - 15, cell_y - 15, &min_cell_chunk_x, &min_cell_chunk_y);
    i32 max_cell_chunk_x, max_cell_chunk_y;
    get_chunk_coord_from_cell_coord(cell_x + 15, cell_y + 15, &max_cell_chunk_x, &max_cell_chunk_y);
    bool is_occupied = false;
    ITERATE(chunk_iter, iterate_sim_chunks(sim, min_cell_chunk_x, min_cell_chunk_y, 
                                           max_cell_chunk_x, max_cell_chunk_y)) {
        SimRegionChunk *chunk = chunk_iter.ptr;
        ITERATE(iter, iterate_chunk_entities(chunk)) {
            Entity *entity = get_entity_by_id(sim, *iter.ptr);
            if (entity && entity->flags & ENTITY_FLAG_HAS_WORLD_PLACEMENT) {
                i32 min_occupancy_cell_x = Floor(entity->p.x);
                i32 min_occupancy_cell_y = Floor(entity->p.y);
                i32 max_occupancy_cell_x = min_occupancy_cell_x;
                i32 max_occupancy_cell_y = min_occupancy_cell_y;
                if ((min_occupancy_cell_x <= cell_x && cell_x <= max_occupancy_cell_x) && 
                    (min_occupancy_cell_y <= cell_y && cell_y <= max_occupancy_cell_y)) {
                    is_occupied = true;
                    goto end;
                }
            }
        }
    }
    end: 
    return is_occupied;
}

bool check_spatial_placement(SimRegion *sim, i32 cell_x, i32 cell_y, u32 width, u32 height) {
    // TIMED_FUNCTION();
    // i32 min_occupant_x = cell_x;
    // i32 min_occupant_y = cell_y;
    // i32 max_occupant_x = cell_x + width;
    // i32 max_occupant_y = cell_y + height;
    
    // i32 cell_chunk_x, cell_chunk_y;
    // get_chunk_coord_from_cell_coord(cell_x, cell_y, &cell_chunk_x, &cell_chunk_y);
    // bool is_occupied = false;
    // for (SimChunkIterator iter = iterate_sim_chunks(sim, cell_chunk_x - 1, cell_chunk_y - 1, 
    //         cell_chunk_x + 1, cell_chunk_y + 1);
    //      is_valid(&iter) && !is_occupied;
    //      advance(&iter)) {
    //     SimRegionChunk *chunk = iter.ptr;
    //     i32 chunk_pos_in_cells_x = chunk->chunk_x * CELLS_IN_CHUNK;
    //     i32 chunk_pos_in_cells_y = chunk->chunk_y * CELLS_IN_CHUNK;
    //     for (u32 spatial_data_idx = 0;
    //          spatial_data_idx < chunk->spatial_occupancy_count && !is_occupied;
    //          ++spatial_data_idx) {
    //         SpatialOccupancy occupancy = chunk->spatial_occupancy[spatial_data_idx];
    //         i32 min_occupancy_cell_x = chunk_pos_in_cells_x - 1;
    //         i32 min_occupancy_cell_y = chunk_pos_in_cells_y - 1;
    //         i32 max_occupancy_cell_x = chunk_pos_in_cells_x + occupancy.x + occupancy.w + 1;
    //         i32 max_occupancy_cell_y = chunk_pos_in_cells_y + occupancy.y + occupancy.y + 1;
    //         if (min_occupant_x < max_occupancy_cell_x && max_occupant_x > min_occupancy_cell_x
    //             && min_occupant_y < max_occupancy_cell_y && max_occupant_y > min_occupancy_cell_y) {
    //             is_occupied = true;
    //             break;       
    //         }
    //     }
    // }
    // return is_occupied;
    return false;
}

void begin_sim(SimRegion *sim, MemoryArena *arena, World *world,
               i32 center_x, i32 center_y, u32 chunk_radius) {
    TIMED_FUNCTION();
    sim->arena = arena;
    sim->world = world;
    sim->center_chunk_x = center_x;
    sim->center_chunk_y = center_y;
    sim->chunk_radius = chunk_radius;
    
    u32 chunk_count = get_chunk_count_for_radius(chunk_radius);
#define MAX_ENTITIES_PER_CHUNK 512
    sim->max_entity_count = align_forward_pow2(chunk_count * MAX_ENTITIES_PER_CHUNK);
    sim->entity_count = 0;
    sim->chunks = alloc_arr(arena, chunk_count, SimRegionChunk);
    sim->entities = alloc_arr(arena, sim->max_entity_count, Entity);
    sim->entity_hash = alloc_arr(arena, sim->max_entity_count, SimRegionEntityHash);
    sim->chunks_count = chunk_count;
    for (u32 chunk_idx = 0; chunk_idx < chunk_count; ++chunk_idx) {
        SimRegionChunk *sim_chunk = sim->chunks + chunk_idx;
        sim_chunk->first_block = {};
        
        i32 chx, chy;
        chunk_array_index_to_coord(chunk_radius, chunk_idx, &chx, &chy);
        sim_chunk->chunk_x = chx;
        sim_chunk->chunk_y = chy;
        i32 world_chunk_x = sim->center_chunk_x + chx;
        i32 world_chunk_y = sim->center_chunk_y + chy;
        WorldChunk *world_chunk = remove_world_chunk(world, world_chunk_x, world_chunk_y);
        if (world_chunk) {
            WorldChunkEntityBlock *block = world_chunk->first_entity_block;
            while (block) {
                for (u32 entity_idx = 0; entity_idx < block->entity_count; ++entity_idx) {
                    Entity *src = (Entity *)block->entity_data  + entity_idx;
                    vec2 sim_space_p = get_sim_space_p(sim, world_chunk_x, world_chunk_y, src->p);
                    Entity *dst = create_new_entity(sim, sim_space_p, src);
                }
                
                WorldChunkEntityBlock *next_block = block->next;
                add_entity_block_to_free_list(world, block);
                block = next_block;
            }
            add_chunk_to_free_list(world, world_chunk);
        }
    }
}

void end_sim(SimRegion *sim, struct WorldState *world_state) {
    TIMED_FUNCTION();
    for (size_t entity_idx = 0; entity_idx < sim->entity_count; ++entity_idx) {
        Entity *src = sim->entities + entity_idx;
        if (!(src->flags & ENTITY_FLAG_IS_DELETED)) {
            i32 chunk_x, chunk_y;
            vec2 chunk_p;
            get_global_space_p(sim, src->p, &chunk_x, &chunk_y, &chunk_p);
            src->p = chunk_p;
            pack_entity_into_world(sim->world, chunk_x, chunk_y, src);   
            
            if (src->flags & ENTITY_FLAG_IS_ANCHOR) {
                Anchor anchor = {};
                anchor.chunk_x = chunk_x;
                anchor.chunk_y = chunk_y;
                anchor.radius = 5;
                world_state->anchors[world_state->anchor_count++] = anchor;
            }
        }
    }
    DEBUG_VALUE(sim->entity_hash_hits, "Entity hash hits");
    DEBUG_VALUE(sim->entity_hash_misses, "Entit hash misses");
}

void next(SimChunkIterator *iter) {
    for (;;) {
        if (iter->idx < ((iter->max_chunk_y - iter->min_chunk_y + 1) * (iter->max_chunk_x - iter->min_chunk_x + 1))) {
            i32 local_x = iter->idx % (iter->max_chunk_x - iter->min_chunk_x + 1);
            i32 local_y = iter->idx / (iter->max_chunk_x - iter->min_chunk_x + 1);
            i32 chunk_x = iter->min_chunk_x + local_x;
            i32 chunk_y = iter->min_chunk_y + local_y;
            SimRegionChunk *chunk = get_chunk(iter->sim, chunk_x, chunk_y);
            iter->ptr = chunk;
            ++iter->idx;
            if (iter->ptr) {
                break;
            }
        } else {
            iter->ptr = 0;
            break;
        }
    }
}

SimChunkIterator iterate_sim_chunks_in_radius(SimRegion *sim, vec2 p, f32 radius) {
    SimChunkIterator iter = {};
    iter.sim = sim;
    vec2 top_left = p + Vec2(-radius, -radius);
    vec2 bottom_right = p + Vec2(radius, radius);
    p_to_chunk_coord(top_left, &iter.min_chunk_x, &iter.min_chunk_y);
    p_to_chunk_coord(bottom_right, &iter.max_chunk_x, &iter.max_chunk_y);
    assert(iter.min_chunk_x <= iter.max_chunk_x);
    assert(iter.min_chunk_y <= iter.max_chunk_y);
    iter.idx = 0;
    next(&iter);
    return iter;
}

SimChunkIterator iterate_sim_chunks(SimRegion *sim, i32 min_chunk_x, i32 min_chunk_y, i32 max_chunk_x, i32 max_chunk_y) {
    SimChunkIterator iter;
    iter.sim = sim;
    iter.min_chunk_x = min_chunk_x;
    iter.min_chunk_y = min_chunk_y;
    iter.max_chunk_x = max_chunk_x;
    iter.max_chunk_y = max_chunk_y;
    assert(iter.min_chunk_x <= iter.max_chunk_x);
    assert(iter.min_chunk_y <= iter.max_chunk_y);
    iter.idx = 0;
    next(&iter);
    return iter;
}

bool is_valid(SimChunkIterator *iter) {
    return iter->ptr;
}

static void next(SimChunkEntityIterator *iter) {
    for(;;) {
        if (iter->entity_idx < iter->block->entity_count) {
            iter->ptr = iter->block->ids + iter->entity_idx++;
			break;
        } else if (iter->block->next) {
            iter->block = iter->block->next;
            iter->entity_idx = 0;
        } else {
            iter->ptr = 0;
            break;
        }
    }
}

SimChunkEntityIterator iterate_chunk_entities(SimRegionChunk *chunk) {
    SimChunkEntityIterator iter;
    iter.block = &chunk->first_block;
    iter.entity_idx = 0;
    next(&iter);
    return iter;
}

bool is_valid(SimChunkEntityIterator *iter) {
    return iter->ptr != 0;
}

EntityIteratorSettings iter_radius(vec2 origin, f32 radius) {
    EntityIteratorSettings iter = {};
    iter.flags = ENTITY_ITERATOR_DISTANCE_BASED;
    iter.origin = origin;
    iter.radius = radius;
    return iter;
}

void next(EntityIterator *iter) {
    for (;;) {
        if (is_valid(&iter->chunk_entity_iterator)) {
            next(&iter->chunk_entity_iterator);
            iter->ptr = iter->chunk_entity_iterator.ptr;
            if (iter->ptr) {
                break;
            }
        } else if (is_valid(&iter->chunk_iterator)) {
            next(&iter->chunk_iterator);
            if (is_valid(&iter->chunk_iterator)) {
                iter->chunk_entity_iterator = iterate_chunk_entities(iter->chunk_iterator.ptr);
                iter->ptr = iter->chunk_entity_iterator.ptr;
                if (iter->ptr) {
                    break;
                }
            } 
        } else {
            iter->ptr = 0;
            break;
        }
    }
}

EntityIterator iterate_entities(SimRegion *sim, EntityIteratorSettings settings) {
    EntityIterator iter = {};
    iter.sim = sim;
    iter.settings = settings;
    if (settings.flags & ENTITY_ITERATOR_DISTANCE_BASED) {
        iter.chunk_iterator = iterate_sim_chunks_in_radius(sim, settings.origin, settings.radius);
        if (is_valid(&iter.chunk_iterator)) {
            iter.chunk_entity_iterator = iterate_chunk_entities(iter.chunk_iterator.ptr);
			iter.ptr = iter.chunk_entity_iterator.ptr;
        }
    } else {
        INVALID_CODE_PATH;
    }
    return iter;
}

bool is_valid(EntityIterator *iter) {
    return iter->ptr != 0;
}
