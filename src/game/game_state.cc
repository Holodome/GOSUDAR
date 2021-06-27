#include "game/game_state.hh"

#include "game/ray_casting.hh"

static bool is_in_same_cell(Vec2 a, Vec2 b) {
    Vec2 a_floor = floor_to_cell(a);
    Vec2 b_floor = floor_to_cell(b);
    return a_floor == b_floor;
}


static bool is_cell_occupied(SimRegion *sim, Vec2 p) {
    TIMED_FUNCTION();
    bool occupied = false;
    
    for (EntityIterator iter = iterate_all_entities(sim);
         is_valid(&iter);
         advance(&iter)) {
        SimEntity *entity = iter.ptr;
        if (entity->kind & ENTITY_KIND_WORLD_OBJECT) {
            if (is_in_same_cell(entity->p, p)) {
                occupied = true;
                break;
            }
        }        
    }
    return occupied;
}

static EntityID add_player(SimRegion *sim) {
    SimEntity *entity = create_entity(sim);
    entity->p = {};
    entity->kind = ENTITY_KIND_PLAYER;
    return entity->id;
}

static EntityID add_tree(SimRegion *sim, Vec2 pos) {
    pos = floor_to_cell(pos);
    
    SimEntity *entity = create_entity(sim);
    entity->p = pos;
    entity->kind = ENTITY_KIND_WORLD_OBJECT;
    entity->world_object_kind = (rand() % 3 + WORLD_OBJECT_KIND_TREE_FOREST);
    entity->world_object_flags = WORLD_OBJECT_FLAG_IS_RESOURCE;
    entity->resource_kind = RESOURCE_KIND_WOOD;
    entity->resource_interactions_left = 1;
    entity->resource_gain = 2;
    return entity->id;
}

static EntityID add_gold_vein(SimRegion *sim, Vec2 pos) {
    pos = floor_to_cell(pos);
    
    SimEntity *entity = create_entity(sim);
    entity->p = pos;
    entity->kind = ENTITY_KIND_WORLD_OBJECT;
    entity->world_object_kind = WORLD_OBJECT_KIND_GOLD_DEPOSIT;
    entity->world_object_flags = WORLD_OBJECT_FLAG_IS_RESOURCE;
    entity->resource_kind = RESOURCE_KIND_GOLD;
    entity->resource_interactions_left = 5;
    entity->resource_gain = 10;
    return entity->id;
}

static void world_gen(GameState *game_state) {
    Entropy entropy;
    entropy.state = 1233437824;
    SimRegion *gen_sim = begin_sim(game_state, Vec2i(0), Vec2i(0));
    // Initialize game_state 
    game_state->camera_followed_entity_id = add_player(gen_sim);
    for (size_t i = 0; i < 1000; ++i) {
        do {
            Vec2 p;
            p.x = random(&entropy) * CHUNK_SIZE * 10;
            p.y = random(&entropy) * CHUNK_SIZE * 10;
            if (!is_cell_occupied(gen_sim, p)) {
                add_tree(gen_sim, p);
                break;
            }
        } while (true);
    }
    
    for (size_t i = 0; i < 50; ++i) {
        do {
            Vec2 p;
            p.x = random(&entropy) * CHUNK_SIZE * 10;
            p.y = random(&entropy) * CHUNK_SIZE * 10;
            if (!is_cell_occupied(gen_sim, p)) {
                add_gold_vein(gen_sim, p);
                break;
            }
        } while (0);
    }
    end_sim(gen_sim);
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
    game_state->interaction_kind = PLAYER_INTERACTION_KIND_NONE;
    game_state->allow_camera_controls = false;
    // Initialize world struct
    game_state->world = alloc_struct(&game_state->arena, World);
    game_state->world->world_arena = &game_state->arena;
    // Initialize world
    game_state->world->first_free = 0;
    game_state->world->entity_count = 1; // !!!
    game_state->world->max_entity_count = 16384;
    game_state->world->entities = (Entity *)arena_alloc(game_state->world->world_arena, 
														sizeof(Entity) * game_state->world->max_entity_count);
    memset(game_state->world->chunk_hash, 0, sizeof(game_state->world->chunk_hash));
    for (size_t i = 0; i < ARRAY_SIZE(game_state->world->chunk_hash); ++i) {
        game_state->world->chunk_hash[i].coord = Vec2i(CHUNK_COORD_UNINITIALIZED, 0);
    }
    for (size_t i = 0; i < game_state->world->max_entity_count; ++i) {
        game_state->world->entities[i].world_pos.chunk.x = CHUNK_COORD_UNINITIALIZED;
    }
    
    world_gen(game_state);
}

static void get_ground_tile_positions(Vec2i tile_pos, Vec3 out[4]) {
    f32 x = (f32)tile_pos.x;
    f32 y = (f32)tile_pos.y;
    out[0] = Vec3(x, 0, y) * TILE_SIZE;
    out[1] = Vec3(x, 0, y + 1) * TILE_SIZE;
    out[2] = Vec3(x + 1, 0, y) * TILE_SIZE;
    out[3] = Vec3(x + 1, 0, y + 1) * TILE_SIZE;
}

static void get_billboard_positions(Vec3 mstorage_index_bottom, Vec3 right, Vec3 up, f32 width, f32 height, Vec3 out[4]) {
    Vec3 top_left = mstorage_index_bottom - right * width * 0.5f + up * height;
    Vec3 bottom_left = top_left - up * height;
    Vec3 top_right = top_left + right * width;
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
#define REGION_CHUNK_RADIUS 2
    *min = Vec2i(camera_coord.chunk.x - REGION_CHUNK_RADIUS, camera_coord.chunk.y - REGION_CHUNK_RADIUS);
    *max = Vec2i(camera_coord.chunk.x + REGION_CHUNK_RADIUS, camera_coord.chunk.y + REGION_CHUNK_RADIUS);
}

static void update_interactions(GameState *game_state, FrameData *frame, Input *input) {
    if (game_state->is_in_building_mode) {
        return;
    }
    
    if (!game_state->interaction_kind) {
        game_state->interactable = null_id();
        f32 closest_so_far = INFINITY;
        for (EntityIterator tree_iter = iterate_all_entities(frame->sim);
            is_valid(&tree_iter);
            advance(&tree_iter)) {
            SimEntity *test_entity = tree_iter.ptr;
            if (test_entity->kind == ENTITY_KIND_WORLD_OBJECT) {
                f32 d_sq = Math::length_sq(test_entity->p - frame->camera_followed_entity->p);
#define INTERACT_DISTANCE 0.25f
#define INTERACT_DISTANCE_SQ (INTERACT_DISTANCE * INTERACT_DISTANCE)
                if (d_sq < INTERACT_DISTANCE_SQ && d_sq < closest_so_far) {
                    closest_so_far = d_sq;
                    game_state->interactable = test_entity->id;
                }
            }
        }
    }
    
    if (is_not_null(game_state->interactable)) {
        SimEntity *ent = get_entity_by_id(frame->sim, game_state->interactable);
        // printf("%hhu %u\n", ent->world_object_flags, ent->id.value);

        assert(ent->kind == ENTITY_KIND_WORLD_OBJECT);
        if (game_state->interaction_kind) {
            bool continue_interaction = false;
            switch (game_state->interaction_kind) {
                case PLAYER_INTERACTION_KIND_MINE_RESOURCE: {
                    continue_interaction = input->is_key_held(Key::MouseLeft);
                } break;
                case PLAYER_INTERACTION_KIND_BUILD: {
                    continue_interaction = input->is_key_held(Key::MouseRight);
                } break;
                INVALID_DEFAULT_CASE;
            }
            
            if (continue_interaction) {
                game_state->interaction_current_time += input->dt;
                if (game_state->interaction_current_time >= game_state->interaction_time) {
                    // finalize interaction
                    switch (game_state->interaction_kind) {
                        case PLAYER_INTERACTION_KIND_MINE_RESOURCE: {
                            assert(ent->world_object_flags & WORLD_OBJECT_FLAG_IS_RESOURCE);
                            assert(ent->resource_interactions_left);
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
                        } break;
                        case PLAYER_INTERACTION_KIND_BUILD: {
                            assert(ent->world_object_flags & WORLD_OBJECT_FLAG_IS_BUILDING);
#define BUILD_SPEED 0.1
                            ent->build_progress += BUILD_SPEED;
                            ent->build_progress = Math::min(ent->build_progress, 1.0f);
                            if (ent->build_progress == 1.0f) {
                                
                            }
                        } break;
                        INVALID_DEFAULT_CASE;
                    }
                    game_state->interaction_kind = PLAYER_INTERACTION_KIND_NONE;
                } 
            } else {
                
                game_state->interaction_kind = PLAYER_INTERACTION_KIND_NONE;
            }
        } else {
            u8 interaction_kind = PLAYER_INTERACTION_KIND_NONE;
            if (ent->world_object_flags & WORLD_OBJECT_FLAG_IS_RESOURCE) {
                if (input->is_key_held(Key::MouseLeft)) {
                    interaction_kind = PLAYER_INTERACTION_KIND_MINE_RESOURCE;  
                } 
            } else if (ent->world_object_flags & WORLD_OBJECT_FLAG_IS_BUILDING) {
                if (input->is_key_held(Key::MouseRight)) {
                    interaction_kind = PLAYER_INTERACTION_KIND_BUILD;
                }
            }
            
            if (interaction_kind) {
                f32 interaction_time = 0.0f;
                bool cancel_interaction = false;
                if (ent->world_object_flags & WORLD_OBJECT_FLAG_IS_RESOURCE) {
                    assert(ent->resource_interactions_left > 0);
                    // switch on type...
                    interaction_time = 1.0f;
                } else if (ent->world_object_flags & WORLD_OBJECT_FLAG_IS_BUILDING) {
                    if (ent->build_progress < 1.0f) {
                        interaction_time = 1.0f;
                    } else {
                        cancel_interaction = true;
                    }
                } else {
                    assert(false);
                }
                
                if (!cancel_interaction) {
                    game_state->interaction_time = interaction_time;
                    game_state->interaction_current_time = 0;
                    game_state->interaction_kind = interaction_kind;
                }
            }
        }
    } 
}

static void update_interface(GameState *game_state, Input *input) {
    if (input->is_key_pressed(Key::Z)) {
        game_state->allow_camera_controls = !game_state->allow_camera_controls;
    }
    if (input->is_key_pressed(Key::B)) {
        game_state->is_in_building_mode = !game_state->is_in_building_mode;
    }
    
    // Update camera input
    if (game_state->allow_camera_controls) {
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
}

static void update_world_simulation(GameState *game_state, FrameData *frame, Input *input) {
    TIMED_FUNCTION();
    SimRegion *sim = frame->sim;

    SimEntity *camera_followed_entity = get_entity_by_id(sim, game_state->camera_followed_entity_id);
    frame->camera_followed_entity = camera_followed_entity;
    if (!game_state->interaction_kind) {
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
        camera_followed_entity->p += player_delta;
    }
    Vec3 center_pos = xz(camera_followed_entity->p);
    f32 horiz_distance = game_state->cam.distance_from_player * Math::cos(game_state->cam.pitch);
    f32 vert_distance = game_state->cam.distance_from_player * Math::sin(game_state->cam.pitch);
    f32 offsetx = horiz_distance * Math::sin(-game_state->cam.yaw);
    f32 offsetz = horiz_distance * Math::cos(-game_state->cam.yaw);
    sim->cam_p.x = offsetx + center_pos.x;
    sim->cam_p.z = offsetz + center_pos.z;
    sim->cam_p.y = vert_distance;
    // set camera matrix
#define CAMERA_FOV Math::rad(60)
#define CAMERA_NEAR_PLANE 0.001f
#define CAMERA_FAR_PLANE  100.0f
    Mat4x4 projection = Mat4x4::perspective(CAMERA_FOV, input->winsize.aspect_ratio(), CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);
    Mat4x4 view = Mat4x4::identity() * Mat4x4::rotation(game_state->cam.pitch, Vec3(1, 0, 0)) * Mat4x4::rotation(game_state->cam.yaw, Vec3(0, 1, 0))
        * Mat4x4::translate(-sim->cam_p);
    sim->cam_mvp = projection * view;
    // Get mouse point projected on plane
    Vec3 ray_dir = uv_to_world(projection, view, Vec2((2.0f * input->mpos.x) / input->winsize.x - 1.0f,
													  1.0f - (2.0f * input->mpos.y) / input->winsize.y));
    f32 t = 0;
    ray_intersect_plane(Vec3(0, 1, 0), 0, sim->cam_p, ray_dir, &t);
    Vec3 mouse_point_xyz = sim->cam_p + ray_dir * t;
    Vec2 mouse_point = Vec2(mouse_point_xyz.x, mouse_point_xyz.z);
    frame->mouse_projection = mouse_point;
    
    update_interactions(game_state, frame, input);
    
    if (game_state->is_in_building_mode) {
        if (input->is_key_pressed(Key::MouseRight)) {
            Vec2 p = floor_to_cell(mouse_point);
            if (!is_cell_occupied(sim, p)) {
                SimEntity *building = create_entity(sim);
                building->p = p;
                building->kind = ENTITY_KIND_WORLD_OBJECT;
                building->world_object_flags = WORLD_OBJECT_FLAG_IS_BUILDING;
                building->world_object_kind = WORLD_OBJECT_KIND_BUILDING1;
                game_state->is_in_building_mode = false;
            }
        }
    }
}

static void render_world(GameState *game_state, FrameData *frame, RendererCommands *commands, Assets *assets) {
    TIMED_FUNCTION();
    SimRegion *sim = frame->sim;
    RenderGroup world_render_group = render_group_begin(commands, assets, setup_3d(sim->cam_mvp));
    // Draw ground
    BEGIN_BLOCK("Render_ground");
    for (i32 chunk_x = sim->min_chunk.x; chunk_x <= sim->max_chunk.x; ++chunk_x) {
        for (i32 chunk_y = sim->min_chunk.y; chunk_y <= sim->max_chunk.y; ++chunk_y) {
            if (chunk_x < game_state->min_chunk.x || chunk_x > game_state->max_chunk.x || 
                chunk_y < game_state->min_chunk.y || chunk_y > game_state->max_chunk.y) {
                continue;       
            }
            for (i32 tile_x = 0; tile_x < TILES_IN_CHUNK; ++tile_x) {
                for (i32 tile_y = 0; tile_y < TILES_IN_CHUNK; ++tile_y) {
                    Vec2i tile_pos = (Vec2i(chunk_x, chunk_y) - sim->min_chunk) * TILES_IN_CHUNK + Vec2i(tile_x, tile_y);
                    Vec3 tile_v[4];
                    get_ground_tile_positions(tile_pos, tile_v);
                    push_quad(&world_render_group, tile_v, Asset_Grass);
                }
            }
        }
    }
    // Draw grid near mouse cursor
    Vec2 mouse_cell_pos = floor_to_cell(frame->mouse_projection);
#define MOUSE_CELL_RAD 5
    for (i32 dy = -MOUSE_CELL_RAD / 2; dy <= MOUSE_CELL_RAD / 2; ++dy) {
        for (i32 dx = -MOUSE_CELL_RAD / 2; dx <= MOUSE_CELL_RAD / 2; ++dx) {
            Vec2 cell_middle = Vec2(mouse_cell_pos.x + dx * CELL_SIZE + CELL_SIZE * 0.5f, mouse_cell_pos.y + dy * CELL_SIZE + CELL_SIZE * 0.5f);
            bool is_occupied = is_cell_occupied(sim, cell_middle);
            if (!is_occupied) {
                Vec3 v0 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE, WORLD_EPSILON, mouse_cell_pos.y + dy * CELL_SIZE);
                Vec3 v1 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE, WORLD_EPSILON, mouse_cell_pos.y + dy * CELL_SIZE + CELL_SIZE);
                Vec3 v2 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE + CELL_SIZE, WORLD_EPSILON, mouse_cell_pos.y + dy * CELL_SIZE);
                Vec3 v3 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE + CELL_SIZE, WORLD_EPSILON, mouse_cell_pos.y + dy * CELL_SIZE + CELL_SIZE);
                push_quad_outline(&world_render_group, v0, v1, v2, v3, Colors::black, 0.01f);
            } else {
                Vec3 v0 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE, WORLD_EPSILON * 2.0f, mouse_cell_pos.y + dy * CELL_SIZE);
                Vec3 v1 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE, WORLD_EPSILON * 2.0f, mouse_cell_pos.y + dy * CELL_SIZE + CELL_SIZE);
                Vec3 v2 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE + CELL_SIZE, WORLD_EPSILON * 2.0f, mouse_cell_pos.y + dy * CELL_SIZE);
                Vec3 v3 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE + CELL_SIZE, WORLD_EPSILON * 2.0f, mouse_cell_pos.y + dy * CELL_SIZE + CELL_SIZE);
                push_quad_outline(&world_render_group, v0, v1, v2, v3, Colors::red, 0.01f);
            }
        }
    }
    // Highlight selected entity    
    if (is_not_null(game_state->interactable)) {
        SimEntity *interactable = get_entity_by_id(sim, game_state->interactable);
        Vec2 entity_p = interactable->p;
        Vec2 half_size = Vec2(0.5f, 0.5f) * 0.5f;
        Vec3 v[4];
        // @TODO why plain epsilon causes flickering?
        v[0] = xz(entity_p + Vec2(-half_size.x, -half_size.y), 10 * WORLD_EPSILON);
        v[1] = xz(entity_p + Vec2(-half_size.x, half_size.y),  10 * WORLD_EPSILON);
        v[2] = xz(entity_p + Vec2(half_size.x, -half_size.y),  10 * WORLD_EPSILON);
        v[3] = xz(entity_p + Vec2(half_size.x, half_size.y),   10 * WORLD_EPSILON);
        push_quad(&world_render_group, v, Asset_SelectCircle);
    }
    END_BLOCK();
    // Collecting entities for drawing is better done after updating in case some of them are deleted...
    BEGIN_BLOCK("ZSORT");
    TempMemory zsort = temp_memory_begin(&game_state->frame_arena);
    size_t max_drawable_count = sim->entity_count;
    size_t drawable_entity_storage_index_count = 0;
    size_t *drawable_entity_storage_indexs = alloc_arr(&game_state->frame_arena, max_drawable_count, size_t);
    for (EntityIterator iter = iterate_all_entities(sim);
         is_valid(&iter);
         advance(&iter)) {
        drawable_entity_storage_indexs[drawable_entity_storage_index_count++] = iter.idx;
    }
    
    // Sort by distance to camera
    qsort_s(drawable_entity_storage_indexs, drawable_entity_storage_index_count, sizeof(*drawable_entity_storage_indexs), z_camera_sort, sim);
    END_BLOCK();
    BEGIN_BLOCK("Render billboards");
    for (size_t drawable_idx = 0; drawable_idx < drawable_entity_storage_index_count; ++drawable_idx) {
        SimEntity *entity = &sim->entities[drawable_entity_storage_indexs[drawable_idx]];
        AssetID texture_id = INVALID_ASSET_ID;
        Vec2 size = Vec2(0, 0);
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
                        if (entity->build_progress == 1.0f) {
                            texture_id = Asset_Building;
                        } else {
                            texture_id = Asset_Building1;
                        }
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
    END_BLOCK();
    render_group_end(&world_render_group); 
}

void update_and_render(GameState *game_state, Input *input, RendererCommands *commands, Assets *assets) {
    TIMED_FUNCTION();
    FrameData frame = {};
    arena_clear(&game_state->frame_arena);
    // Since camera follows some entity and is never too far from it, we can assume that camera position is
    // the position of followed entity
    WorldPosition camera_position = get_world_entity(game_state->world, game_state->camera_followed_entity_id)->world_pos;
    Vec2i min_chunk_coord, max_chunk_coord;
    get_sim_region_bounds(camera_position, &min_chunk_coord, &max_chunk_coord);
    frame.sim = begin_sim(game_state, min_chunk_coord, max_chunk_coord);
    SimRegion *sim = frame.sim;
    
    update_interface(game_state, input);
    update_world_simulation(game_state, &frame, input);
    render_world(game_state, &frame, commands, assets);
    game_state->DEBUG_last_frame_sim_region_entity_count = sim->entity_count;
    end_sim(sim);
}
