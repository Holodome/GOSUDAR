#include "game/world.hh"
#include "game/game.hh"

#include "game/ray_casting.hh"

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

#define CHUNK_COORD_UNINITIALIZED 0x7FFFFFFF

static bool is_chunk_coord_initialized(Vec2i coord) {
    return coord.x != CHUNK_COORD_UNINITIALIZED;
}

static Vec2i chunk_coord_uninitialized() {
    return Vec2i(CHUNK_COORD_UNINITIALIZED, 0);
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

WorldPosition world_position_floor_to_cell(WorldPosition pos) {
    pos.offset.x = floorf(pos.offset.x / CELL_SIZE) * CELL_SIZE + CELL_SIZE * 0.5f;
    pos.offset.y = floorf(pos.offset.y / CELL_SIZE) * CELL_SIZE + CELL_SIZE * 0.5f;
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

EntityID add_world_entity(World *world, WorldPosition pos) {
    assert(world->entity_count < world->max_entity_count);
    EntityID id;
    id.value = world->entity_count++; 
    Entity *entity = world->entities + id.value;
    memset(entity, 0, sizeof(*entity));
    entity->world_pos = pos;
    entity->sim.id = id;
    
    Chunk *chunk = get_world_chunk(world, pos.chunk);
    add_entity_to_chunk(world, chunk, id);
    return id;    
}

Entity *get_world_entity(World *world, EntityID id) {
    assert(id.value < world->entity_count);
    return world->entities + id.value;    
}

Chunk *get_world_chunk(World *world, Vec2i coord) {
    u32 hash_value = coord.x * 123123 + coord.y * 1891289 + 121290;    
    u32 hash_slot = hash_value & (ARRAY_SIZE(world->chunk_hash) - 1);
    
    Chunk *chunk = world->chunk_hash + hash_slot;
    for (;;) {
        if (coord == chunk->coord) {
            break;
        } 
        
        if (is_chunk_coord_initialized(chunk->coord) && !chunk->next_in_hash) {
            chunk->next_in_hash = (Chunk *)arena_alloc(world->world_arena, sizeof(Chunk));
        }
        if (chunk->next_in_hash) {
            chunk = chunk->next_in_hash;
        }
        if (!is_chunk_coord_initialized(chunk->coord)) {
            chunk->coord = coord;
            break;
        }
    }
    return chunk;    
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
                        EntityBlock *entity_block = first_block->next;
                        *first_block = *entity_block;
                        
                        entity_block->next = world->first_free;
                        world->first_free = entity_block;
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
        EntityBlock *new_block = world->first_free;
        if (new_block) {
            world->first_free = world->first_free->next;
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
        remove_entity_from_chunk(world, old_chunk, id);
        Chunk *new_chunk = get_world_chunk(world, to.chunk);
        add_entity_to_chunk(world, new_chunk, id);
    }
    Entity *entity = get_world_entity(world, id);
    entity->world_pos = to;
}

SimRegion *begin_sim(struct GameState *game_state, Vec2i min_chunk, Vec2i max_chunk) {
    SimRegion *sim = alloc_struct(&game_state->frame_arena, SimRegion);
    sim->game_state = game_state;
    sim->max_entity_count = 4096;
    sim->entity_count = 0;
    sim->entities = alloc_arr(&game_state->frame_arena, sim->max_entity_count, SimEntity);
    
    sim->min_chunk = min_chunk;
    sim->max_chunk = max_chunk;
    sim->origin.chunk = sim->min_chunk;
    sim->origin.offset = Vec2(0);
    
    for (i32 chunk_y = min_chunk.y; chunk_y <= max_chunk.y; ++chunk_y) {
        for (i32 chunk_x = min_chunk.x; chunk_x <= max_chunk.x; ++chunk_x) {
            // if (chunk_x < game_state->min_chunk.x || chunk_x > game_state->max_chunk.x || 
            //     chunk_y < game_state->min_chunk.y || chunk_y > game_state->max_chunk.y) {
            //     continue;       
            // }
            Vec2i chunk_coord = Vec2i(chunk_x, chunk_y);
            // Maybe if chunk has zero entities we delete it, this way we can dont worry about chunk regions 
            // in sim region. Let the update code handle where entity can go.
            // So this way we could make portals or whatever in far away chunks
            Chunk *chunk = get_world_chunk(game_state->world, chunk_coord);
            for (EntityBlock *block = &chunk->entity_block;
                 block;
                 block = block->next) {
                for (size_t entity_idx = 0; entity_idx < block->entity_count; ++entity_idx) {
                    EntityID entity_id = block->entity_storage_indices[entity_idx];
                    add_entity(sim, entity_id);
                }        
            }
        }
    }
    
    return sim;
}

void end_sim(SimRegion *sim) {
    for (size_t entity_idx = 0; entity_idx < sim->entity_count; ++entity_idx) {
        SimEntity *entity = sim->entities + entity_idx;
        // Place entity in world 
        WorldPosition new_position = pos_add(sim->origin, entity->p);
        if (is_same(entity->id, null_id()) && !(entity->flags & ENTITY_FLAG_IS_DELETED)) {
            entity->id = add_world_entity(sim->game_state->world, new_position);
            Entity *world_ent = get_world_entity(sim->game_state->world, entity->id);
            world_ent->sim = *entity;
        } else {
            Entity *world_ent = get_world_entity(sim->game_state->world, entity->id);
            if (entity->flags & ENTITY_FLAG_IS_DELETED) {
                Chunk *old_chunk = get_world_chunk(sim->game_state->world, world_ent->world_pos.chunk);
                remove_entity_from_chunk(sim->game_state->world, old_chunk, entity->id);
            } else {
                world_ent->sim = *entity;
				move_entity(sim->game_state->world, entity->id, new_position, world_ent->world_pos);
            }
        }
    }
}

SimEntityHash *get_hash_from_storage_index(SimRegion *sim, EntityID entity_id) {
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
    Entity *world_ent = get_world_entity(sim->game_state->world, entity_id);
    
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

SimEntity *create_entity(SimRegion *sim) {
    assert(sim->entity_count < sim->max_entity_count);
    SimEntity *entity = sim->entities + sim->entity_count++;
    entity->id = null_id();
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

void advance(EntityIterator *iter) {
    ++iter->idx;
    set_entity_to_next_not_deleted(iter);
}

static EntityID add_player(World *world) {
    EntityID entity_id = add_world_entity(world, {});
    Entity *entity = get_world_entity(world, entity_id);
    entity->sim.kind = ENTITY_KIND_PLAYER;
    return entity_id;
}

static EntityID add_tree(World *world, WorldPosition pos) {
    pos = world_position_floor_to_cell(pos);
    
    EntityID entity_id = add_world_entity(world, pos);
    Entity *entity = get_world_entity(world, entity_id);
    entity->sim.kind = ENTITY_KIND_WORLD_OBJECT;
    entity->sim.world_object_kind = (rand() % 3 + WORLD_OBJECT_KIND_TREE_FOREST);
    entity->sim.world_object_flags = WORLD_OBJECT_FLAG_IS_RESOURCE;
    entity->sim.resource_kind = RESOURCE_KIND_WOOD;
    entity->sim.resource_interactions_left = 1;
    entity->sim.resource_gain = 2;
    return entity_id;
}

static EntityID add_gold_vein(World *world, WorldPosition pos) {
    pos = world_position_floor_to_cell(pos);
    
    EntityID entity_id = add_world_entity(world, pos);
    Entity *entity = get_world_entity(world, entity_id);
    entity->sim.kind = ENTITY_KIND_WORLD_OBJECT;
    entity->sim.world_object_kind = WORLD_OBJECT_KIND_GOLD_DEPOSIT;
    entity->sim.world_object_flags = WORLD_OBJECT_FLAG_IS_RESOURCE;
    entity->sim.resource_kind = RESOURCE_KIND_GOLD;
    entity->sim.resource_interactions_left = 5;
    entity->sim.resource_gain = 10;
    return entity_id;
}

void game_state_init(GameState *game_state) {
    // Initialize game_state params
    game_state->min_chunk = Vec2i(0);
    game_state->max_chunk = Vec2i(9);
    game_state->cam.distance_from_player = 3.0f;
    game_state->cam.pitch = Math::HALF_PI * 0.5f;
    game_state->cam.yaw = Math::HALF_PI * 1.5f;
    game_state->wood_count = 0;
    game_state->gold_count = 0;
    game_state->interactable = null_id();
    // Initialize world struct
    game_state->world = alloc_struct(&game_state->arena, World);
    game_state->world->world_arena = &game_state->arena;
    game_state->world->frame_arena = &game_state->frame_arena;
    // Initialize world
    game_state->world->first_free = 0;
    game_state->world->entity_count = 1; // !!!
    game_state->world->max_entity_count = 16384;
    game_state->world->entities = (Entity *)arena_alloc(game_state->world->world_arena, 
														sizeof(Entity) * game_state->world->max_entity_count);
    memset(game_state->world->chunk_hash, 0, sizeof(game_state->world->chunk_hash));
    for (size_t i = 0; i < ARRAY_SIZE(game_state->world->chunk_hash); ++i) {
        game_state->world->chunk_hash[i].coord = chunk_coord_uninitialized();
    }
    // Initialize game_state 
    game_state->camera_followed_entity_id = add_player(game_state->world);
    for (size_t i = 0; i < 1000; ++i) {
        WorldPosition tree_p;
        tree_p.chunk.x = rand() % 10;
        tree_p.chunk.y = rand() % 10;
        tree_p.offset.x = ((rand() / (f32)RAND_MAX)) * CHUNK_SIZE;
        tree_p.offset.y = ((rand() / (f32)RAND_MAX)) * CHUNK_SIZE;
        add_tree(game_state->world, tree_p);
    }
    
    for (size_t i = 0; i < 50; ++i) {
        WorldPosition p;
        p.chunk.x = rand() % 10;
        p.chunk.y = rand() % 10;
        p.offset.x = ((rand() / (f32)RAND_MAX)) * CHUNK_SIZE;
        p.offset.y = ((rand() / (f32)RAND_MAX)) * CHUNK_SIZE;
        add_gold_vein(game_state->world, p);
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

static void get_billboard_positions(Vec3 mstorage_index_bottom, Vec3 right, Vec3 up, f32 wstorage_indexth, f32 height, Vec3 out[4]) {
    Vec3 top_left = mstorage_index_bottom - right * wstorage_indexth * 0.5f + up * height;
    Vec3 bottom_left = top_left - up * height;
    Vec3 top_right = top_left + right * wstorage_indexth;
    Vec3 bottom_right = top_right - up * height;
    out[0] = top_left;
    out[1] = bottom_left;
    out[2] = top_right;
    out[3] = bottom_right;
}

static int z_camera_sort(void *ctx, const void *a, const void *b){
    SimRegion *sim = (SimRegion *)ctx;
    u32 storage_index1 = *((u32 *)a);
    u32 storage_index2 = *((u32 *)b);
    SimEntity *ae = &sim->entities[storage_index1];
    SimEntity *be = &sim->entities[storage_index2];
    int result = 0;
    Vec3 a_pos = Vec3(ae->p.x, 0, ae->p.y);
    Vec3 b_pos = Vec3(be->p.x, 0, be->p.y);
    f32 a_v = Math::dot(sim->cam_mvp.get_z(), a_pos - sim->cam_p);
    f32 b_v = Math::dot(sim->cam_mvp.get_z(), b_pos - sim->cam_p);
    if (fabsf(a_v - b_v) > 0.001f) {
        result = (int)(a_v < b_v ? 1 : -1);
    } else {
        // @TODO fix sorting by x
        Vec3 middle = (a_pos + b_pos) * 0.5f;
        a_v = Math::dot(sim->cam_mvp.get_x(), a_pos - middle);
        b_v = Math::dot(sim->cam_mvp.get_x(), b_pos - middle);
        result = (int)(a_v > b_v ? 1 : -1);
    }
    return (int)(result);
};

static Vec3 uv_to_world(Mat4x4 projection, Mat4x4 view, Vec2 uv) {
    f32 x = uv.x;
    f32 y = uv.y;
    Vec3 ray_dc = Vec3(x, y, 1.0f);
    Vec4 ray_clip = Vec4(ray_dc.xy, -1.0f, 1.0f);
    Vec4 ray_eye = Mat4x4::inverse(projection) * ray_clip;
    ray_eye.z = -1.0f;
    ray_eye.w = 0.0f;
    Vec3 ray_world = Math::normalize((Mat4x4::inverse(view) * ray_eye).xyz);
    return ray_world;
}

static void get_sim_region_bounds(WorldPosition camera_coord, Vec2i *min, Vec2i *max) {
#define REGION_CHUNK_RADIUS 3
    *min = Vec2i(camera_coord.chunk.x - REGION_CHUNK_RADIUS, camera_coord.chunk.y - REGION_CHUNK_RADIUS);
    *max = Vec2i(camera_coord.chunk.x + REGION_CHUNK_RADIUS, camera_coord.chunk.y + REGION_CHUNK_RADIUS);
}

void update_and_render(GameState *game_state, Input *input, RendererCommands *commands, Assets *assets) {
    arena_clear(&game_state->frame_arena);
    // Since camera follows some entity and is never too far from it, we can assume that camera position is
    // the position of followed entity
    WorldPosition camera_position = get_world_entity(game_state->world, game_state->camera_followed_entity_id)->world_pos;
    Vec2i min_chunk_coord, max_chunk_coord;
    get_sim_region_bounds(camera_position, &min_chunk_coord, &max_chunk_coord);
    SimRegion *sim = begin_sim(game_state, min_chunk_coord, max_chunk_coord);
    // @TODO This is kinda stupstorage_index to do update here - what if we want to do sim twice
    // Calculate player movement
    Vec2 player_delta = Vec2(0);
    f32 move_coef = 4.0f * input->dt;
    f32 z_speed = 0;
    if (input->is_key_held(Key::W)) {
        z_speed = move_coef;
    } else if (input->is_key_held(Key::S)) {
        z_speed = -move_coef;
    }
    player_delta.x += z_speed *  Math::sin(game_state->cam.yaw);
    player_delta.y += z_speed * -Math::cos(game_state->cam.yaw);
    
    f32 x_speed = 0;
    if (input->is_key_held(Key::D)) {
        x_speed = move_coef;
    } else if (input->is_key_held(Key::A)) {
        x_speed = -move_coef;
    }
    player_delta.x += x_speed * Math::cos(game_state->cam.yaw);
    player_delta.y += x_speed * Math::sin(game_state->cam.yaw);     
    
    // Update camera input
    if (input->is_key_held(Key::MouseRight)) {
        f32 x_view_coef = 1.0f * input->dt;
        f32 y_view_coef = 0.6f * input->dt;
        f32 x_angle_change = input->mdelta.x * x_view_coef;
        f32 y_angle_change = input->mdelta.y * y_view_coef;
        game_state->cam.yaw += x_angle_change;
        game_state->cam.yaw = Math::unwind_rad(game_state->cam.yaw);
        game_state->cam.pitch += y_angle_change;
#define MIN_CAM_PITCH (Math::HALF_PI * 0.1f)
#define MAX_CAM_PITCH (Math::HALF_PI * 0.9f)
        game_state->cam.pitch = Math::clamp(game_state->cam.pitch, MIN_CAM_PITCH, MAX_CAM_PITCH);
        game_state->cam.distance_from_player -= input->mwheel;
    }
  
    f32 interact_distance = 0.25f;
    f32 interact_distance_sq = interact_distance * interact_distance;
    for (EntityIterator iter = iterate_all_entities(sim);
         iter.ptr;
         advance(&iter)) {
        SimEntity *entity = iter.ptr;
        
        switch (entity->kind) {
            case ENTITY_KIND_PLAYER: {
                if (!game_state->is_player_interacting) {
                    entity->p += player_delta;
                    // Find closest interactable
                    game_state->interactable = null_id();
                    f32 closest_so_far = INFINITY;
                    for (EntityIterator tree_iter = iterate_all_entities(sim);
                        tree_iter.ptr;
                        advance(&tree_iter)) {
                        SimEntity *test_entity = tree_iter.ptr;
                        if (test_entity->kind == ENTITY_KIND_WORLD_OBJECT) {
                            f32 d_sq = Math::length_sq(test_entity->p - entity->p);
                            if (d_sq < interact_distance_sq && d_sq < closest_so_far) {
                                closest_so_far = d_sq;
                                game_state->interactable = test_entity->id;
                            }
                        }
                    }
                }   
            } break;
        }
    }
   
    if (is_not_null(game_state->interactable)) {
        SimEntity *ent = get_entity_by_id(sim, game_state->interactable);
        if (input->is_key_held(Key::MouseLeft)) {
            if (game_state->is_player_interacting) {
                game_state->interaction_current_time += input->dt;
                if (game_state->interaction_current_time >= game_state->interaction_time) {
                    game_state->is_player_interacting = false;
                    if (ent->world_object_flags & WORLD_OBJECT_FLAG_IS_RESOURCE) {
                        ent->resource_interactions_left -= 1;
                        switch (ent->resource_kind) {
                            case RESOURCE_KIND_WOOD: {
                                game_state->wood_count += ent->resource_gain;
                            } break;
                            case RESOURCE_KIND_GOLD: {
                                game_state->gold_count += ent->resource_gain;
                            } break;
                            INVALID_DEFAULT_CASE;
                        }
                        if (ent->resource_interactions_left == 0) {
                            ent->flags |= ENTITY_FLAG_IS_DELETED;
                            game_state->interactable = null_id();
                        }
                    } else {
                        assert(false);
                    }
                }
            } else {
                f32 interaction_time;
                assert(ent->resource_interactions_left > 0);
                if (ent->world_object_flags & WORLD_OBJECT_FLAG_IS_RESOURCE) {
                    interaction_time = 1.0f;
                } else {
                    assert(false);
                }
                game_state->interaction_time = interaction_time;
                game_state->interaction_current_time = 0;
                game_state->is_player_interacting = true;
            }
        } else {
            game_state->is_player_interacting = false;
        }
    } 
    // Update camera movement
    SimEntity *camera_followed_entity = get_entity_by_id(sim, game_state->camera_followed_entity_id);
    Vec3 center_pos = Vec3(camera_followed_entity->p.x, 0, camera_followed_entity->p.y);
    f32 horiz_distance = game_state->cam.distance_from_player * Math::cos(game_state->cam.pitch);
    f32 vert_distance = game_state->cam.distance_from_player * Math::sin(game_state->cam.pitch);
    f32 offsetx = horiz_distance * Math::sin(-game_state->cam.yaw);
    f32 offsetz = horiz_distance * Math::cos(-game_state->cam.yaw);
    sim->cam_p.x = offsetx + center_pos.x;
    sim->cam_p.z = offsetz + center_pos.z;
    sim->cam_p.y = vert_distance;
    // set camera matrix
    Mat4x4 projection = Mat4x4::perspective(Math::rad(60), input->winsize.aspect_ratio(), 0.001f, 100.0f);
    Mat4x4 view = Mat4x4::identity() * Mat4x4::rotation(game_state->cam.pitch, Vec3(1, 0, 0)) * Mat4x4::rotation(game_state->cam.yaw, Vec3(0, 1, 0))
        * Mat4x4::translate(-sim->cam_p);
    sim->cam_mvp = projection * view;
    // Get mouse point projected on plane
    Vec3 ray_dir = uv_to_world(projection, view, Vec2((2.0f * input->mpos.x) / input->winsize.x - 1.0f,
													  1.0f - (2.0f * input->mpos.y) / input->winsize.y));
    f32 t = 0;
    bool intersect = ray_intersect_plane(Vec3(0, 1, 0), 0, sim->cam_p, ray_dir, &t);
    Vec3 mouse_point_xyz = sim->cam_p + ray_dir * t;
    Vec2 mouse_point = Vec2(mouse_point_xyz.x, mouse_point_xyz.z);
    
    RenderGroup world_render_group = render_group_begin(commands, assets, setup_3d(sim->cam_mvp));
    // Draw ground
    for (i32 chunk_x = sim->min_chunk.x; chunk_x <= sim->max_chunk.x; ++chunk_x) {
        for (i32 chunk_y = sim->min_chunk.y; chunk_y <= sim->max_chunk.y; ++chunk_y) {
            if (chunk_x < game_state->min_chunk.x || chunk_x > game_state->max_chunk.x || 
                chunk_y < game_state->min_chunk.y || chunk_y > game_state->max_chunk.y) {
                continue;       
            }
            for (size_t tile_x = 0; tile_x < TILES_IN_CHUNK; ++tile_x) {
                for (size_t tile_y = 0; tile_y < TILES_IN_CHUNK; ++tile_y) {
                    Vec2i tile_pos = (Vec2i(chunk_x, chunk_y) - sim->min_chunk) * TILES_IN_CHUNK + Vec2i(tile_x, tile_y);
                    Vec3 tile_v[4];
                    get_ground_tile_positions(tile_pos, tile_v);
                    push_quad(&world_render_group, tile_v, Asset_Grass);
                }
            }
        }
    }
    // Draw grid near mouse cursor
    Vec2i mouse_cell_coord = Vec2i(floorf(mouse_point.x / CELL_SIZE), floorf(mouse_point.y / CELL_SIZE));
    Vec2 mouse_cell_pos = Vec2(mouse_cell_coord) * CELL_SIZE;
#define MOUSE_CELL_RAD 5
    for (i32 dy = -MOUSE_CELL_RAD / 2; dy <= MOUSE_CELL_RAD / 2; ++dy) {
        for (i32 dx = -MOUSE_CELL_RAD / 2; dx <= MOUSE_CELL_RAD / 2; ++dx) {
            Vec3 v0 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE, WORLD_EPSILON, mouse_cell_pos.y + dy * CELL_SIZE);
            Vec3 v1 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE, WORLD_EPSILON, mouse_cell_pos.y + dy * CELL_SIZE + CELL_SIZE);
            Vec3 v2 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE + CELL_SIZE, WORLD_EPSILON, mouse_cell_pos.y + dy * CELL_SIZE);
            Vec3 v3 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE + CELL_SIZE, WORLD_EPSILON, mouse_cell_pos.y + dy * CELL_SIZE + CELL_SIZE);
            push_quad_outline(&world_render_group, v0, v1, v2, v3, Colors::black, WORLD_EPSILON);
        }
    }
    // Highlight selected entity    
    if (is_not_null(game_state->interactable)) {
        SimEntity *interactable = get_entity_by_id(sim, game_state->interactable);
        Vec2 entity_p = interactable->p;
        Vec2 half_size = Vec2(0.5f, 0.5f) * 0.5f;
        Vec3 v[4];
        v[0] = xz(entity_p + Vec2(-half_size.x, -half_size.y), WORLD_EPSILON);
        v[1] = xz(entity_p + Vec2(-half_size.x, half_size.y), WORLD_EPSILON);
        v[2] = xz(entity_p + Vec2(half_size.x, -half_size.y), WORLD_EPSILON);
        v[3] = xz(entity_p + Vec2(half_size.x, half_size.y), WORLD_EPSILON);
        push_quad(&world_render_group, v, Asset_SelectCircle);
    }
    size_t max_drawable_count = sim->entity_count;
    // Collecting entities for drawing is better done after updating in case some of them are deleted...
    TempMemory zsort = temp_memory_begin(&game_state->frame_arena);
    size_t drawable_entity_storage_index_count = 0;
    u32 *drawable_entity_storage_indexs = alloc_arr(&game_state->frame_arena, max_drawable_count, u32);
    for (EntityIterator iter = iterate_all_entities(sim);
         iter.ptr;
         advance(&iter)) {
        SimEntity *entity = iter.ptr;
        drawable_entity_storage_indexs[drawable_entity_storage_index_count++] = iter.idx;
    }
    
    // Sort by distance to camera
    qsort_s(drawable_entity_storage_indexs, drawable_entity_storage_index_count, sizeof(u32), z_camera_sort, sim);
    for (size_t drawable_idx = 0; drawable_idx < drawable_entity_storage_index_count; ++drawable_idx) {
        SimEntity *entity = &sim->entities[drawable_entity_storage_indexs[drawable_idx]];
        AssetID texture_id;
        Vec2 size;
        switch(entity->kind) {
            case ENTITY_KIND_PLAYER: {
                texture_id = Asset_Dude;
                size = Vec2(0.5f);
            } break;
            case ENTITY_KIND_WORLD_OBJECT: {
                switch(entity->world_object_kind) {
                    case WORLD_OBJECT_KIND_TREE_FOREST: {
                        texture_id = Asset_TreeForest;
                    } break;
                    case WORLD_OBJECT_KIND_TREE_JUNGLE: {
                        texture_id = Asset_TreeJungle;
                    } break;
                    case WORLD_OBJECT_KIND_TREE_DESERT: {
                        texture_id = Asset_TreeDesert;
                    } break;
                    case WORLD_OBJECT_KIND_GOLD_DEPOSIT: {
                        texture_id = Asset_GoldVein;
                    } break;
                    case WORLD_OBJECT_KIND_BUILDING1: {
                        texture_id = Asset_Building;
                    } break;
                    case WORLD_OBJECT_KIND_BUILDING2: {
                        texture_id = Asset_Building1;
                    } break;
                    INVALID_DEFAULT_CASE;
                }
                size = Vec2(0.5f);
            } break;
            INVALID_DEFAULT_CASE;
        }
        
        Vec3 billboard[4];
        Vec3 pos = Vec3(entity->p.x, 0, entity->p.y);
        get_billboard_positions(pos, sim->cam_mvp.get_x(), sim->cam_mvp.get_y(), size.x, size.y, billboard);
        push_quad(&world_render_group, billboard, texture_id);
    }
    render_group_end(&world_render_group); 
    game_state->DEBUG_last_frame_sim_region_entity_count = sim->entity_count;
    end_sim(sim);
}
