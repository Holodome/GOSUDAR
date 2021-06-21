#include "game/world.hh"
#include "game/game.hh"

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

static WorldPosition map_into_chunk_space(WorldPosition base_pos, Vec2 offset) {
    WorldPosition result = base_pos;
    result.offset += offset;
    recanonicalize_coord(&result.chunk.x, &result.offset.x);
    recanonicalize_coord(&result.chunk.y, &result.offset.y);
    return result;
}

static WorldPosition map_into_chunk_space(Vec2 offset) {
    return map_into_chunk_space({}, offset);
}

static WorldPosition chunk_origin(WorldPosition pos) {
    pos.offset = Vec2(0);
    return pos;
}

static Vec2 world_difference(WorldPosition a, WorldPosition b) {
    Vec2 dcoord = Vec2(a.chunk - b.chunk);
    Vec2 result = dcoord * CHUNK_SIZE + (a.offset - b.offset);
    return result;
}

static Vec2 global_position_from_world_position(WorldPosition p) {
    WorldPosition origin = {};
    return world_difference(p, origin);
}

static bool is_same_chunk(WorldPosition a, WorldPosition b) {
    assert(is_canonical(a.offset) && is_canonical(b.offset));
    return a.chunk == b.chunk;
}

static void change_entity_position_internal(World *world, EntityID id, WorldPosition *old_p, WorldPosition new_p) {
    if (old_p && is_same_chunk(*old_p, new_p)) {
           
    } else {
        // Remove entity from old chunk
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
        
        // Place entity in new chunk
        Chunk *chunk = get_world_chunk(world, new_p.chunk);
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

static void change_entity_position(World *world, Entity *entity, WorldPosition new_p) {
    change_entity_position_internal(world, entity->sim.id, &entity->world_pos, new_p);
    entity->world_pos = new_p;
}

EntityID add_entity(World *world, EntityKind kind, WorldPosition pos) {
    assert(world->entity_count < world->max_entity_count);
    EntityID id = world->entity_count++;
    Entity *entity = world->entities + id;
    memset(entity, 0, sizeof(*entity));
    change_entity_position_internal(world, id, 0, pos);
    entity->world_pos = pos;
    entity->sim.id = id;
    entity->sim.kind = kind;
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

static WorldPosition world_position_from_tile_position(Vec2i tile_position) {
    // Tile center, so tiles are placed in chunk_hash correctly
    Vec2 global_pos = (Vec2(tile_position) + Vec2(0.5f)) * TILE_SIZE;
    WorldPosition result = map_into_chunk_space({}, global_pos);
    return result;
}

EntityID add_tile(World *world, Vec2i tile_position) {
    WorldPosition tile_world_position = world_position_from_tile_position(tile_position);
    EntityID id = add_entity(world, EntityKind::GroundTile, tile_world_position);
    Entity *entity = get_entity(world, id);
    entity->sim.tile_pos = tile_position;
    entity->sim.texture_id = Asset_Grass;
    return id;
}

void world_init(World *world) {
    world->first_free = 0;
    world->entity_count = 0;
    world->max_entity_count = 4096;
    world->entities = (Entity *)arena_alloc(&world->world_arena, sizeof(Entity) * world->max_entity_count);
    memset(world->chunk_hash, 0, sizeof(world->chunk_hash));
    
    // world->camera.init();
    world->camera.distance_from_player = 5.0f;
    world->camera.pitch = 0.0f;
    world->camera.yaw = 0.0f;
    world->camera_followed_entity_id = add_player(world);
    for (size_t i = 0; i < 1000; ++i) {
        WorldPosition tree_p;
        tree_p.chunk.x = rand() % 10;
        tree_p.chunk.y = rand() % 10;
        tree_p.offset.x = ((rand() / (f32)RAND_MAX)) * CHUNK_SIZE;
        tree_p.offset.y = ((rand() / (f32)RAND_MAX)) * CHUNK_SIZE;
        add_tree(world, tree_p);
    }
    for (size_t i = 0; i < TILES_IN_CHUNK * 10; ++i) {
        for (size_t j = 0; j < TILES_IN_CHUNK * 10; ++j) {
            Vec2i tile_pos = Vec2i(j, i);
            add_tile(world, tile_pos);
        }
    }
}

Chunk *get_world_chunk(World *world, Vec2i coord) {
    u32 hash_value = coord.x * 123123 + coord.y * 1891289 + 121290;    
    u32 hash_slot = hash_value % ARRAY_SIZE(world->chunk_hash);
    
    Chunk *chunk = world->chunk_hash + hash_slot;
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

static Vec2 world_position_to_camera_position(WorldPosition pos) {
    WorldPosition camera_pos = {};
    return world_difference(pos, camera_pos);
}

static void get_sim_region_bounds(WorldPosition camera_coord, Vec2i *min, Vec2i *max) {
    *min = Vec2i(camera_coord.chunk.x - 10, camera_coord.chunk.y - 10);
    *max = Vec2i(camera_coord.chunk.x + 10, camera_coord.chunk.y + 10);
}

SimRegion *begin_sim(MemoryArena *sim_arena, World *world) {
    SimRegion *sim = alloc_struct(sim_arena, SimRegion);
    sim->world = world;
    sim->max_entity_count = 4096;
    sim->entity_count = 0;
    sim->entities = alloc_arr(sim_arena, sim->max_entity_count, SimEntity);
    // Since camera follows some entity and is never too far from it, we can assume that camera position is
    // the position of followed entity
    WorldPosition camera_position = get_entity(world, world->camera_followed_entity_id)->world_pos;
    Vec2i min_chunk_coord, max_chunk_coord;
    get_sim_region_bounds(camera_position, &min_chunk_coord, &max_chunk_coord);
    sim->cam = world->camera;
    
    for (i32 chunk_y = min_chunk_coord.y; chunk_y <= max_chunk_coord.y; ++chunk_y) {
        for (i32 chunk_x = min_chunk_coord.x; chunk_x <= max_chunk_coord.x; ++chunk_x) {
            Vec2i chunk_coord = Vec2i(chunk_x, chunk_y);
            
            Chunk *chunk = get_world_chunk(world, chunk_coord);
            for (ChunkEntityBlock *block = &chunk->entity_block;
                 block;
                 block = block->next) {
                for (size_t entity_idx = 0; entity_idx < block->entity_count; ++entity_idx) {
                    EntityID id = block->entity_ids[entity_idx];
                    Entity *entity = get_entity(world, id);
                    
                    assert(sim->entity_count < sim->max_entity_count);
                    SimEntity *dest = sim->entities + sim->entity_count++;
                    *dest = entity->sim;
                    dest->p = world_difference(entity->world_pos, sim->origin);
                }        
            }
        }
    }
    
    return sim;
}

void end_sim(SimRegion *sim, Input *input) {
    sim->world->camera = sim->cam;
    for (size_t entity_idx = 0; entity_idx < sim->entity_count; ++entity_idx) {
        SimEntity *entity = sim->entities + entity_idx;
        WorldPosition new_position = map_into_chunk_space(sim->origin, entity->p);
        
        Entity *world_ent = get_entity(sim->world, entity->id);
        change_entity_position(sim->world, world_ent, new_position);
        world_ent->sim = *entity;
    }
}

static void get_ground_tile_positions(Vec2i tile_pos, Vec3 out[4]) {
    f32 x = tile_pos.x;
    f32 y = tile_pos.y;
    out[0] = Vec3(x, 0, y) * TILE_SIZE;
    out[1] = Vec3(x, 0, y + 1) * TILE_SIZE;
    out[2] = Vec3(x + 1, 0, y) * TILE_SIZE;
    out[3] = Vec3(x + 1, 0, y + 1) * TILE_SIZE;
}

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

int z_camera_sort(void *ctx, const void *a, const void *b){
    SimRegion *sim = (SimRegion *)ctx;
    EntityID id1 = *((EntityID *)a);
    EntityID id2 = *((EntityID *)b);
    SimEntity *ae = &sim->entities[id1];
    SimEntity *be = &sim->entities[id2];
    int result = 0;
    Vec3 a_pos = Vec3(ae->p.x, 0, ae->p.y);
    Vec3 b_pos = Vec3(be->p.x, 0, be->p.y);
    f32 a_v = Math::dot(sim->cam_mvp.get_z(), a_pos - sim->cam_p);
    f32 b_v = Math::dot(sim->cam_mvp.get_z(), b_pos - sim->cam_p);
    result = (int)(a_v < b_v ? -1 : 1);
    return (int)(-result);
};

void do_sim(SimRegion *sim, Input *input, Renderer *renderer, Assets *assets) {
    // @TODO This is kinda stupid to do update here - what if we want to do sim twice
    // Calculate player movement
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
    
    // Update camera input
    f32 x_view_coef = 1.0f * input->dt;
    f32 y_view_coef = 0.6f * input->dt;
    f32 x_angle_change = input->mdelta.x * x_view_coef;
    f32 y_angle_change = input->mdelta.y * y_view_coef;
    sim->cam.yaw += x_angle_change;
    sim->cam.yaw = Math::unwind_rad(sim->cam.yaw);
    sim->cam.pitch += y_angle_change;
    sim->cam.pitch = Math::clamp(sim->cam.pitch, 0.01f, Math::HALF_PI - 0.01f);
    sim->cam.distance_from_player -= input->mwheel;
    
    for (size_t entity_idx = 0; entity_idx < sim->entity_count; ++entity_idx) {
        SimEntity *entity = &sim->entities[entity_idx];
        
        switch (entity->kind) {
            case EntityKind::Player: {
                entity->p += player_delta;
            } break;
        }
        
        // Update camera movement
        if (entity->id == sim->world->camera_followed_entity_id) {
            Vec3 center_pos = Vec3(entity->p.x, 0, entity->p.y);
            f32 horiz_distance = sim->cam.distance_from_player * Math::cos(sim->cam.pitch);
            f32 vert_distance = sim->cam.distance_from_player * Math::sin(sim->cam.pitch);
            f32 offsetx = horiz_distance * Math::sin(-sim->cam.yaw);
            f32 offsetz = horiz_distance * Math::cos(-sim->cam.yaw);
            sim->cam_p.x = offsetx + center_pos.x;
            sim->cam_p.z = offsetz + center_pos.z;
            sim->cam_p.y = vert_distance;
        }
    }
   
    // set camera matrix
    Mat4x4 projection = Mat4x4::perspective(Math::rad(60), input->winsize.aspect_ratio(), 0.001f, 100.0f);
    Mat4x4 view = Mat4x4::identity() * Mat4x4::rotation(sim->cam.pitch, Vec3(1, 0, 0)) * Mat4x4::rotation(sim->cam.yaw, Vec3(0, 1, 0))
        * Mat4x4::translate(-sim->cam_p);
    sim->cam_mvp = projection * view;
    
    RenderGroup world_render_group = render_group_begin(renderer, assets, sim->cam_mvp);
    world_render_group.has_depth = true;
    for (size_t entity_idx = 0; entity_idx < sim->entity_count; ++entity_idx) {
        SimEntity *entity = &sim->entities[entity_idx];
        if (!(entity->flags & EntityFlags_IsBillboard)) {
            Vec2i tile_pos = entity->tile_pos - sim->origin.chunk * TILES_IN_CHUNK;
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
    
    // Sort by distance to camera
    qsort_s(drawable_entity_ids, drawable_entity_id_count, sizeof(u32), z_camera_sort, sim);
    for (size_t drawable_idx = 0; drawable_idx < drawable_entity_id_count; ++drawable_idx) {
        SimEntity *entity = &sim->entities[drawable_entity_ids[drawable_idx]];
        Vec3 billboard[4];
        Vec3 pos = Vec3(entity->p.x, 0, entity->p.y);
        get_billboard_positions(pos, sim->cam_mvp.get_x(), sim->cam_mvp.get_y(), entity->size.x, entity->size.y, billboard);
        imm_draw_quad(&world_render_group, billboard, entity->texture_id);
    }
    render_group_end(&world_render_group);    
}