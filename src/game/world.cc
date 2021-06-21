#include "game/world.hh"
#include "game/game.hh"

static void get_billboard_positions(Vec3 mid_bottom, Vec3 right, Vec3 up, f32 width, f32 height, Vec3 out[4]) {
    Vec3 top_left = mid_bottom - right * width * 0.5f + up * height;
    Vec3 bottom_left = top_left - up * height;
    Vec3 top_right = top_left + right * width;
    Vec3 bottom_right = top_right - up * height;
    out[0] = top_left;
    out[1] = bottom_left;
    out[2] = top_right;
    out[3] = bottom_right;
}

bool is_canonical(f32 tile_rel) {
    bool result = ((tile_rel >= -0.5f * CHUNK_SIZE - 0.001f) && (tile_rel <= 0.5f * CHUNK_SIZE + 0.001f));
    return result;
}

bool is_canonical(Vec2 offset) {
    return is_canonical(offset.x) && is_canonical(offset.y);
}

void recanonicalize_coord(i32 *chunk, f32 *chunk_rel) {
    i32 offset = (i32)roundf(*chunk_rel / CHUNK_SIZE);
    *chunk += offset;
    *chunk_rel -= offset * CHUNK_SIZE;
    assert(is_canonical(*chunk_rel));
}

WorldPosition map_into_chunk_space(WorldPosition base_pos, Vec2 offset) {
    WorldPosition result = base_pos;
    result.offset += offset;
    recanonicalize_coord(&result.chunk.x, &result.offset.x);
    recanonicalize_coord(&result.chunk.y, &result.offset.y);
    return result;
}

Vec2 world_difference(WorldPosition *a, WorldPosition *b) {
    Vec2 dcoord = Vec2(a->chunk - b->chunk);
    Vec2 result = dcoord * CHUNK_SIZE + (a->offset - b->offset);
    return result;
}

Vec2 global_position_from_world_position(WorldPosition p) {
    WorldPosition origin = {};
    return world_difference(&p, &origin);
}

bool is_same_chunk(WorldPosition a, WorldPosition b) {
    assert(is_canonical(a.offset) && is_canonical(b.offset));
    return a.chunk == b.chunk;
}

EntityID add_entity(World *world, EntityKind kind, WorldPosition pos) {
    assert(world->entity_count < world->max_entity_count);
    EntityID id = world->entity_count++;
    
    world->entities[id] = {};
    world->entities[id].sim.id = id;
    world->entities[id].sim.kind = kind;
    change_entity_position(world, id, 0, &pos);
    return id;
}

Entity *get_entity(World *world, EntityID id) {
    assert(id < world->entity_count);
    return world->entities + id;
}

static void add_flag(SimEntity *entity, u32 flag) {
    entity->flags |= flag;
}

static void remove_flag(SimEntity *entity, u32 flag) {
    entity->flags &= ~flag;
}

EntityID add_player(World *world) {
    EntityID id = add_entity(world, EntityKind::Player, {});
    Entity *entity = get_entity(world, id);
    add_flag(&entity->sim, EntityFlags_IsBillboard);
    entity->sim.texture_id = Asset_Dude;
    entity->sim.size = Vec2(0.5f);
    return id;
}

EntityID add_tree(World *world, WorldPosition pos) {
    EntityID id = add_entity(world, EntityKind::Tree, pos);
    Entity *entity = get_entity(world, id);
    add_flag(&entity->sim, EntityFlags_IsBillboard);
    entity->sim.texture_id = Asset_Tree;
    entity->sim.size = Vec2(0.5f);
    return id;
}

WorldPosition world_position_from_tile_position(Vec2i tile_position) {
    // Tile center, so tiles are placed in chunks correctly
    Vec2 global_pos = (Vec2(tile_position) + Vec2(0.5f)) * TILE_SIZE;
    WorldPosition result = map_into_chunk_space({}, global_pos);
    return result;
}

EntityID add_tile(World *world, Vec2i tile_position) {
    WorldPosition tile_world_position = world_position_from_tile_position(tile_position);
    EntityID id = add_entity(world, EntityKind::Ground, tile_world_position);
    Entity *entity = get_entity(world, id);
    entity->sim.tile_pos = tile_position;
    entity->sim.texture_id = Asset_Grass;
    return id;
}

void world_init(World *world) {
    world->first_free = 0;
    world->wood_count = 0;
    world->entity_count = 0;
    world->max_entity_count = 4096;
    world->entities = (Entity *)arena_alloc(&world->world_arena, sizeof(Entity) * world->max_entity_count);
    memset(world->chunks, 0, sizeof(world->chunks));
    
    world->camera.init();
    world->player_id = add_player(world);
    for (size_t i = 0; i < 100; ++i) {
        WorldPosition tree_p;
        tree_p.chunk.x = rand() % 10;
        tree_p.chunk.y = rand() % 10;
        tree_p.offset.x = ((rand() / (f32)RAND_MAX) * 2.0f - 1.0f) * CHUNK_SIZE;
        tree_p.offset.y = ((rand() / (f32)RAND_MAX) * 2.0f - 1.0f) * CHUNK_SIZE;
        // add_tree(world, tree_p);
    }
    for (size_t i = 0; i < TILES_IN_CHUNK; ++i) {
        for (size_t j = 0; j < TILES_IN_CHUNK; ++j) {
            Vec2i tile_pos = Vec2i(j, i);
            add_tile(world, tile_pos);
        }
    }
}

void get_tile_v(Vec2i coord, Vec3 out[4]) {
    i32 x = coord.x;
    i32 y = coord.y;
    out[0] = Vec3(x, 0, y) * CHUNK_SIZE;
    out[1] = Vec3(x, 0, y + 1) * CHUNK_SIZE;
    out[2] = Vec3(x + 1, 0, y) * CHUNK_SIZE;
    out[3] = Vec3(x + 1, 0, y + 1) * CHUNK_SIZE;
}

Chunk *get_world_chunk(World *world, Vec2i coord) {
    u32 hash_value = coord.x * 123123 + coord.y * 1891289 + 121290;    
    u32 hash_slot = hash_value % ARRAY_SIZE(world->chunks);
    
    Chunk *chunk = world->chunks + hash_slot;
    do {
        if (chunk->is_initialized && coord == chunk->coord) {
            break;
        }
        
        if (chunk->is_initialized && !chunk->next_in_hash) {
            chunk->next_in_hash = (Chunk *)arena_alloc(&world->world_arena, sizeof(Chunk));
        }
        
        if (chunk->next_in_hash) {
            assert(chunk->is_initialized);
            chunk = chunk->next_in_hash;
        }
        
        if (!chunk->is_initialized) {
            chunk->coord = coord;
            chunk->is_initialized = true;
            break;
        }
    } while(chunk);
    return chunk;
}

void change_entity_position(World *world, EntityID id, WorldPosition *old_p, WorldPosition *new_p) {
    if (old_p && is_same_chunk(*old_p, *new_p)) {
        
    } else {
        if (old_p) {
            Chunk *chunk = get_world_chunk(world, old_p->chunk);
            ChunkEntityBlock *first_block = &chunk->entity_block;
            bool not_found = true; 
            for (ChunkEntityBlock *entity_block = first_block;
                 entity_block && not_found;
                 entity_block = entity_block->next) {
                for (size_t entity_idx = 0; entity_idx < entity_block->entity_count && not_found; ++entity_idx) {
                    if (entity_block->entity_ids[entity_idx] == id) {
                        assert(first_block->entity_count > 0);
                        entity_block->entity_ids[entity_idx] = first_block->entity_ids[--first_block->entity_count];  
                        if (first_block->entity_count == 0) {
                            if (first_block->next) {
                                ChunkEntityBlock *entity_block = first_block->next;
                                *first_block = *entity_block;
                                
                                entity_block->next = world->first_free;
                                world->first_free = entity_block;
                            }
                        } 
                     
                        not_found = false;   
                    }
                }
            }
            
            assert(!not_found);
        }
        
        Chunk *chunk = get_world_chunk(world, new_p->chunk);
        printf("%u %u\n", chunk->coord.x, chunk->coord.y);
        ChunkEntityBlock *entity_block = &chunk->entity_block;
        if (entity_block->entity_count == ARRAY_SIZE(entity_block->entity_ids)) {
            ChunkEntityBlock *new_block = world->first_free;
            if (new_block) {
                world->first_free = world->first_free->next;
            } else {        
                new_block = (ChunkEntityBlock *)arena_alloc(&world->world_arena, sizeof(ChunkEntityBlock));
            }
            *new_block = *entity_block;
            entity_block->next = new_block;
            entity_block->entity_count = 0;
        }
        assert(entity_block->entity_count < ARRAY_SIZE(entity_block->entity_ids));
        entity_block->entity_ids[entity_block->entity_count++] = id;
    }
}

static Vec2 world_position_to_camera_position(WorldPosition pos) {
    WorldPosition camera_pos = {};
    return world_difference(&pos, &camera_pos);
}

static void add_entity(SimRegion *sim, Entity *entity) {
    assert(sim->entity_count < sim->max_entity_count);
    SimEntity *dest = sim->entities +sim->entity_count++;
    *dest = entity->sim;
    dest->p = world_position_to_camera_position(entity->world_pos);
}

SimRegion *begin_sim(MemoryArena *sim_arena, World *world) {
    SimRegion *sim = alloc_struct(sim_arena, SimRegion);
    sim->world = world;
    sim->max_entity_count = 4096;
    sim->entity_count = 0;
    sim->entities = alloc_arr(sim_arena, sim->max_entity_count, SimEntity);
    
    for (size_t chunk_y = 0; chunk_y < 10; ++chunk_y) {
        for (size_t chunk_x = 0; chunk_x < 10; ++chunk_x) {
            Vec2i chunk_coord = Vec2i(chunk_x, chunk_y);
            Chunk *chunk = get_world_chunk(world, chunk_coord);
            for (ChunkEntityBlock *block = &chunk->entity_block;
                 block;
                 block = block->next) {
                for (size_t entity_idx = 0; entity_idx < block->entity_count; ++entity_idx) {
                    EntityID id = block->entity_ids[entity_idx];
                    Entity *entity = get_entity(world, id);
                    add_entity(sim, entity);
                }        
            }
        }
    }
    
    return sim;
}

void end_sim(SimRegion *sim) {
    for (size_t entity_idx = 0; entity_idx < sim->entity_count; ++entity_idx) {
        SimEntity *entity = sim->entities + entity_idx;
        WorldPosition new_position = map_into_chunk_space({}, entity->p);
        change_entity_position(sim->world, entity->id, &get_entity(sim->world, entity->id)->world_pos, &new_position);
        Entity *world_ent = get_entity(sim->world, entity->id);
        world_ent->world_pos = new_position;
        world_ent->sim = *entity;
    }
}

static void get_ground_tile_positions(Vec2i tile_pos, Vec3 out[4]) {
    Rect tile_rect = Rect(Vec2(tile_pos) * TILE_SIZE, Vec2(TILE_SIZE));
    out[0].x = tile_rect.top_left().x;
    out[1].x = tile_rect.bottom_left().x;
    out[2].x = tile_rect.top_right().x;
    out[3].x = tile_rect.bottom_right().x;
    out[0].y = 0;
    out[1].y = 0;
    out[2].y = 0;
    out[3].y = 0;
    out[0].z = tile_rect.top_left().y;
    out[1].z = tile_rect.bottom_left().y;
    out[2].z = tile_rect.top_right().y;
    out[3].z = tile_rect.bottom_right().y;
}

void do_sim(SimRegion *sim, Input *input, Renderer *renderer, Assets *assets) {
    Vec2 player_delta = Vec2(0);
    f32 move_coef = 4.0f * input->dt;
    f32 z_speed = 0;
    if (input->is_key_held(Key::W)) {
        z_speed = move_coef;
    } else if (input->is_key_held(Key::S)) {
        z_speed = -move_coef;
    }
    player_delta.x += z_speed *  Math::sin(sim->world->camera.yaw);
    player_delta.y += z_speed * -Math::cos(sim->world->camera.yaw);
    
    f32 x_speed = 0;
    if (input->is_key_held(Key::D)) {
        x_speed = move_coef;
    } else if (input->is_key_held(Key::A)) {
        x_speed = -move_coef;
    }
    player_delta.x += x_speed * Math::cos(sim->world->camera.yaw);
    player_delta.y += x_speed * Math::sin(sim->world->camera.yaw);     
    
    for (size_t entity_idx = 0; entity_idx < sim->entity_count; ++entity_idx) {
        SimEntity *entity = &sim->entities[entity_idx];
        
        switch (entity->kind) {
            case EntityKind::Player: {
                entity->p += player_delta;
                
                if (input->is_key_pressed(Key::MouseRight)) {
                    for (size_t entity_idx = 0; entity_idx < sim->entity_count; ++entity_idx) {
                        SimEntity *tree = &sim->entities[entity_idx];
                        if (tree->kind == EntityKind::Tree) {
                            // f32 distance_sq = Math::length_sq(tree->pos - entity->pos);
                            // f32 chop_distance_sq = 0.2f * 0.2f;
                            // if (distance_sq < chop_distance_sq) {
                            //     tree->chops_left -= 1;
                            //     if (tree->chops_left == 0) {
                            //         tree->is_alive = false;
                            //         ++world->wood_count;
                            //     }
                            // }
                        }
                    }
                }
                if (input->is_key_pressed(Key::MouseLeft)) {
                    // if (world->wood_count >= 10) {
                    //     world->wood_count -= 10;
                    //     add_building_entity(world, get_world_position_from_p(entity->pos));
                    // }
                }

            } break;
        }
    }
    
    
    RenderGroup world_render_group = render_group_begin(renderer, assets, sim->world->camera.mvp);
    world_render_group.has_depth = true;
    for (size_t entity_idx = 0; entity_idx < sim->entity_count; ++entity_idx) {
        SimEntity *entity = &sim->entities[entity_idx];
        if (!(entity->flags & EntityFlags_IsBillboard)) {
            Vec2i tile_pos = entity->tile_pos;
            Vec3 v[4];
            get_ground_tile_positions(tile_pos, v);
            imm_draw_quad(&world_render_group, v, entity->texture_id);
        }
    }
    
    size_t max_drawable_count = sim->entity_count;
    TempMemory zsort = temp_memory_begin(sim->frame_arena);
    size_t drawable_entity_id_count = 0;
    u32 *drawable_entity_ids = alloc_arr(sim->frame_arena, max_drawable_count, EntityID);
    for (size_t entity_idx = 0; entity_idx < sim->entity_count; ++entity_idx) {
        SimEntity *entity = &sim->entities[entity_idx];
        if (entity->flags & EntityFlags_IsBillboard) {
            drawable_entity_ids[drawable_entity_id_count++] = entity_idx;
        }
    }
    
    auto sort_lambda = [](void *ctx, const void *a, const void *b){
        SimRegion *sim = (SimRegion *)ctx;
        EntityID id1 = *((EntityID *)a);
        EntityID id2 = *((EntityID *)b);
        SimEntity *ae = &sim->entities[id1];
        SimEntity *be = &sim->entities[id2];
        int result = 0;
        Vec3 a_pos = Vec3(ae->p.x, 0, ae->p.y);
        Vec3 b_pos = Vec3(be->p.x, 0, be->p.y);
        f32 a_v = Math::dot(sim->world->camera.mvp.get_z(), a_pos - sim->world->camera.pos);
        f32 b_v = Math::dot(sim->world->camera.mvp.get_z(), b_pos - sim->world->camera.pos);
        result = (int)(a_v < b_v ? -1 : 1);
        return (int)(-result);
    };
    // Sort by distance to camera
    qsort_s(drawable_entity_ids, drawable_entity_id_count, sizeof(u32), sort_lambda, sim);
    for (size_t drawable_idx = 0; drawable_idx < drawable_entity_id_count; ++drawable_idx) {
        SimEntity *entity = &sim->entities[drawable_entity_ids[drawable_idx]];
        Vec3 billboard[4];
        Vec3 pos = Vec3(entity->p.x, 0, entity->p.y);
        get_billboard_positions(pos, sim->world->camera.mvp.get_x(), sim->world->camera.mvp.get_y(), entity->size.x, entity->size.y, billboard);
        imm_draw_quad(&world_render_group, billboard, entity->texture_id);
    }
    render_group_end(&world_render_group);    
}