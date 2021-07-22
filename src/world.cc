#include "world.hh"

u32 hash_chunk_location(u32 chunk_x, u32 chunk_y) {
    return 123 * chunk_x + 456 + chunk_y + 789;
}

static WorldChunk **get_world_chunk_internal(World *world, i32 chunk_x, i32 chunk_y) {
    u32 hash_value = hash_chunk_location(chunk_x, chunk_y);
    CT_ASSERT(IS_POW2(ARRAY_SIZE(world->chunk_hash)));
    u32 hash_slot = hash_value & (ARRAY_SIZE(world->chunk_hash) - 1);
    
    WorldChunk **chunk = &world->chunk_hash[hash_slot];
    while (*chunk && !((chunk_x == (*chunk)->chunk_x && 
                        chunk_y == (*chunk)->chunk_y))) {
        chunk = &(*chunk)->next;                       
    }
    
    return chunk;
}

WorldChunk *get_world_chunk(World *world, i32 chunk_x, i32 chunk_y) {
    WorldChunk **chunk_ptr = get_world_chunk_internal(world, chunk_x, chunk_y);
    WorldChunk *result = *chunk_ptr;
    if (!result) {
        result = get_new_chunk(world);
        result->chunk_x = chunk_x;
        result->chunk_y = chunk_y;
        
        LLIST_ADD(*chunk_ptr, result);
    }
    return result;
}

WorldChunk *remove_world_chunk(World *world, i32 chunk_x, i32 chunk_y) {
    WorldChunk *result = 0;
    WorldChunk **chunk_ptr = get_world_chunk_internal(world, chunk_x, chunk_y);
    if (*chunk_ptr) {
        result = *chunk_ptr;
        *chunk_ptr = (*chunk_ptr)->next;
    }
    return result;
}

static bool entity_block_has_enough_space_for(WorldChunkEntityBlock *entity_block, u32 size) {
    bool result = ((entity_block->entity_data_size + size) <= sizeof(entity_block->entity_data));
    return result;
}

static void clear_entity_block(WorldChunkEntityBlock *entity_block) {
    memset(entity_block->entity_data, 0, sizeof(entity_block->entity_data));
    entity_block->entity_count = 0;
    entity_block->entity_data_size = 0;
}

void pack_entity_into_chunk(World *world, WorldChunk *chunk, Entity *src) {
    u32 pack_size = sizeof(*src);
    if (!chunk->first_entity_block || !entity_block_has_enough_space_for(chunk->first_entity_block, pack_size)) {
        WorldChunkEntityBlock *entity_block = get_new_entity_block(world);
        LLIST_ADD(chunk->first_entity_block, entity_block);
        clear_entity_block(entity_block);
    }
    
    WorldChunkEntityBlock *block = chunk->first_entity_block;
    assert(entity_block_has_enough_space_for(block, pack_size));
    u8 *dst = block->entity_data + block->entity_data_size;
    
    *(Entity *)dst = *src;
    block->entity_data_size += pack_size;
    block->ids[block->entity_count++] = src->id;
}

void pack_entity_into_world(World *world, i32 chunk_x, i32 chunk_y, Entity *src) {
    WorldChunk *chunk = get_world_chunk(world, chunk_x, chunk_y);
    pack_entity_into_chunk(world, chunk, src);
}

EntityID get_new_id(World *world) {
    EntityID result;
    if (world->first_id) {
        WorldIDListEntry *entry = world->first_id;
        result = entry->id;
        LLIST_POP(world->first_id);
        LLIST_ADD(world->first_free_id, entry);
    } else {
        result.value = world->max_entity_id++;
    }
    return result;
}

WorldChunk *get_new_chunk(World *world) {
    WorldChunk *chunk = world->first_free_chunk;
    if (!chunk) {
        ++world->chunks_allocated;
        chunk = alloc_struct(&world->arena, WorldChunk);
    } else {
        LLIST_POP(world->first_free_chunk);
    }
    memset(chunk, 0, sizeof(*chunk));
    return chunk;
}

WorldChunkEntityBlock *get_new_entity_block(World *world) {
    WorldChunkEntityBlock *entity_block = world->first_free_entity_block;
    if (!entity_block) {
        ++world->entity_blocks_allocated;
        entity_block = alloc_struct(&world->arena, WorldChunkEntityBlock);
    } else {
        LLIST_POP(world->first_free_entity_block);
    }
    memset(entity_block, 0, sizeof(*entity_block));
    return entity_block;
}

void add_chunk_to_free_list(World *world, WorldChunk *chunk) {
    LLIST_ADD(world->first_free_chunk, chunk);    
}

void add_entity_block_to_free_list(World *world, WorldChunkEntityBlock *block) {
    LLIST_ADD(world->first_free_entity_block, block);
}

void add_id_to_free_list(World *world, EntityID id) {
    WorldIDListEntry *entry = world->first_free_id;
    if (!entry) {
        ++world->entity_ids_allocated;
        entry = alloc_struct(&world->arena, WorldIDListEntry);
    } else {
        LLIST_POP(world->first_free_id);
    }
    
    entry->id = id;
    LLIST_ADD(world->first_id, entry);
}

World *world_init() {
    World *result = bootstrap_alloc_struct(World, arena);
    result->max_entity_id = 1;
    DEBUG_ARENA_NAME(&result->arena, "World");
    return result;
}