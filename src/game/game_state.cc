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
    entity->resource_interactions_left = 1;
    return entity->id;
}

static EntityID add_gold_vein(SimRegion *sim, Vec2 pos) {
    pos = floor_to_cell(pos);
    
    SimEntity *entity = create_entity(sim);
    entity->p = pos;
    entity->kind = ENTITY_KIND_WORLD_OBJECT;
    entity->world_object_kind = WORLD_OBJECT_KIND_GOLD_DEPOSIT;
    entity->resource_interactions_left = 5;
    return entity->id;
}

static void world_gen(GameState *game_state) {
    Entropy entropy;
    entropy.state = 12345678;
    SimRegion *gen_sim = begin_sim(&game_state->frame_arena, game_state->world, Vec2i(0), Vec2i(0));
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

static WorldObjectSettings tree_settings(u32 resource_gain) {
    WorldObjectSettings result = {};
    result.resource_kind = RESOURCE_KIND_WOOD;
    result.resource_gain = resource_gain;
    result.flags = WORLD_OBJECT_SETTINGS_FLAG_IS_RESOURCE;
    return result;
}

static WorldObjectSettings gold_settings(u32 resource_gain) {
    WorldObjectSettings result = {};
    result.resource_kind = RESOURCE_KIND_GOLD;
    result.resource_gain = resource_gain;
    result.flags = WORLD_OBJECT_SETTINGS_FLAG_IS_RESOURCE;
    return result;
}

static WorldObjectSettings building_settings() {
    WorldObjectSettings result = {};
    result.flags = WORLD_OBJECT_SETTINGS_FLAG_IS_BUILDING;
    return result;
}

void game_state_init(GameState *game_state) {
    game_state->world_object_settings[WORLD_OBJECT_KIND_TREE_DESERT] = tree_settings(1);
    game_state->world_object_settings[WORLD_OBJECT_KIND_TREE_FOREST] = tree_settings(4);
    game_state->world_object_settings[WORLD_OBJECT_KIND_TREE_JUNGLE] = tree_settings(2);
    game_state->world_object_settings[WORLD_OBJECT_KIND_GOLD_DEPOSIT] = gold_settings(10);
    game_state->world_object_settings[WORLD_OBJECT_KIND_BUILDING1] = building_settings();
    game_state->world_object_settings[WORLD_OBJECT_KIND_BUILDING2] = building_settings();
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
    world_init(game_state->world);
    world_gen(game_state);
    
    init_interface_for_game_state(&game_state->arena, &game_state->interface, Vec2(1264, 681));
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
#define REGION_CHUNK_RADIUS 10
    *min = Vec2i(camera_coord.chunk.x - REGION_CHUNK_RADIUS, camera_coord.chunk.y - REGION_CHUNK_RADIUS);
    *max = Vec2i(camera_coord.chunk.x + REGION_CHUNK_RADIUS, camera_coord.chunk.y + REGION_CHUNK_RADIUS);
}

static void update_interactions(GameState *game_state, FrameData *frame, InputManager *input) {
    if (game_state->is_in_building_mode) {
        game_state->interactable = null_id();
        game_state->interaction_kind = PLAYER_INTERACTION_KIND_NONE;
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
        WorldObjectSettings *settings = game_state->world_object_settings + ent->world_object_kind;

        assert(ent->kind == ENTITY_KIND_WORLD_OBJECT);
        if (game_state->interaction_kind) {
            bool continue_interaction = false;
            switch (game_state->interaction_kind) {
                case PLAYER_INTERACTION_KIND_MINE_RESOURCE: {
                    continue_interaction = is_key_held(input, Key::MouseLeft, INPUT_ACCESS_TOKEN_NO_LOCK);
                } break;
                case PLAYER_INTERACTION_KIND_BUILD: {
                    continue_interaction = is_key_held(input, Key::MouseRight, INPUT_ACCESS_TOKEN_NO_LOCK);
                } break;
                INVALID_DEFAULT_CASE;
            }
            
            if (continue_interaction) {
                game_state->interaction_current_time += get_dt(input);
                if (game_state->interaction_current_time >= game_state->interaction_time) {
                    // finalize interaction
                    switch (game_state->interaction_kind) {
                        case PLAYER_INTERACTION_KIND_MINE_RESOURCE: {
                            assert(settings->flags & WORLD_OBJECT_SETTINGS_FLAG_IS_RESOURCE);
                            assert(ent->resource_interactions_left);
                            ent->resource_interactions_left -= 1;
                            
                            switch (settings->resource_kind) {
                                case RESOURCE_KIND_WOOD: {
                                    game_state->wood_count += settings->resource_gain;
                                } break;
                                case RESOURCE_KIND_GOLD: {
                                    game_state->gold_count += settings->resource_gain;
                                } break;
                                INVALID_DEFAULT_CASE;
                            }
                            if (ent->resource_interactions_left == 0) {
                                ent->flags |= ENTITY_FLAG_IS_DELETED;
                                game_state->interactable = null_id();
                            }
                        } break;
                        case PLAYER_INTERACTION_KIND_BUILD: {
                            assert(settings->flags & WORLD_OBJECT_SETTINGS_FLAG_IS_BUILDING);
#define BUILD_SPEED 0.1
                            ent->build_progress += BUILD_SPEED;
                            if (ent->build_progress > 1.0f) {
                                ent->build_progress = 1.0f;
                                ent->world_object_flags |= WORLD_OBJECT_FLAG_IS_BUILT;
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
            if (settings->flags & WORLD_OBJECT_SETTINGS_FLAG_IS_RESOURCE) {
                if (is_key_held(input, Key::MouseLeft, INPUT_ACCESS_TOKEN_NO_LOCK)) {
                    interaction_kind = PLAYER_INTERACTION_KIND_MINE_RESOURCE;  
                } 
            } else if (settings->flags & WORLD_OBJECT_SETTINGS_FLAG_IS_BUILDING) {
                if (is_key_held(input, Key::MouseRight, INPUT_ACCESS_TOKEN_NO_LOCK)) {
                    interaction_kind = PLAYER_INTERACTION_KIND_BUILD;
                }
            }
            
            if (interaction_kind) {
                f32 interaction_time = 0.0f;
                bool cancel_interaction = false;
                if (settings->flags & WORLD_OBJECT_SETTINGS_FLAG_IS_RESOURCE) {
                    assert(ent->resource_interactions_left > 0);
                    // switch on type...
                    interaction_time = 1.0f;
                } else if (settings->flags & WORLD_OBJECT_SETTINGS_FLAG_IS_BUILDING) {
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

static void update_interface(GameState *game_state, FrameData *frame) {
    InputManager *input = frame->input;
    if (is_key_pressed(input, Key::Z, INPUT_ACCESS_TOKEN_NO_LOCK)) {
        game_state->allow_camera_controls = !game_state->allow_camera_controls;
    }
    if (is_key_pressed(input, Key::B, INPUT_ACCESS_TOKEN_NO_LOCK)) {
        game_state->is_in_building_mode = !game_state->is_in_building_mode;
    }
    if (is_key_pressed(input, Key::X, INPUT_ACCESS_TOKEN_NO_LOCK)) {
        game_state->show_grid = !game_state->show_grid;
    }
    
    InterfaceStats stats = interface_update(&game_state->interface.interface, input);
    
    // Update camera input
    if (game_state->allow_camera_controls && input->access_token == INPUT_ACCESS_TOKEN_NO_LOCK) {
        f32 x_view_coef = 1.0f * get_dt(input);
        f32 y_view_coef = 0.6f * get_dt(input);
        f32 x_angle_change = mouse_d(input).x * x_view_coef;
        f32 y_angle_change = mouse_d(input).y * y_view_coef;
        game_state->cam.yaw += x_angle_change;
        game_state->cam.yaw = Math::unwind_rad(game_state->cam.yaw);
        game_state->cam.pitch += y_angle_change;
#define MIN_CAM_PITCH (Math::HALF_PI * 0.1f)
#define MAX_CAM_PITCH (Math::HALF_PI * 0.9f)
        game_state->cam.pitch = Math::clamp(game_state->cam.pitch, MIN_CAM_PITCH, MAX_CAM_PITCH);
        f32 delta_zoom = get_mwheel(input);
        // printf("%f\n", delta_zoom);
        game_state->cam.distance_from_player -= delta_zoom;
        game_state->cam.distance_from_player = Math::clamp(game_state->cam.distance_from_player, 0.5f, 10);
    }
}

static void update_world_simulation(GameState *game_state, FrameData *frame) {
    TIMED_FUNCTION();
    SimRegion *sim = frame->sim;
    InputManager *input = frame->input;

    SimEntity *camera_followed_entity = get_entity_by_id(sim, game_state->camera_followed_entity_id);
    frame->camera_followed_entity = camera_followed_entity;
    if (!game_state->interaction_kind) {
        // Calculate player movement
        Vec2 player_delta = Vec2(0);
        f32 move_coef = 4.0f * get_dt(input);
        f32 z_speed = 0;
        if (is_key_held(input, Key::W, INPUT_ACCESS_TOKEN_NO_LOCK)) {
            z_speed = move_coef;
        } else if (is_key_held(input, Key::S, INPUT_ACCESS_TOKEN_NO_LOCK)) {
            z_speed = -move_coef;
        }
        player_delta.x += z_speed *  Math::sin(game_state->cam.yaw);
        player_delta.y += z_speed * -Math::cos(game_state->cam.yaw);
        
        f32 x_speed = 0;
        if (is_key_held(input, Key::D, INPUT_ACCESS_TOKEN_NO_LOCK)) {
            x_speed = move_coef;
        } else if (is_key_held(input, Key::A, INPUT_ACCESS_TOKEN_NO_LOCK)) {
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
    frame->projection = Mat4x4::perspective(CAMERA_FOV, window_size(input).aspect_ratio(), CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);
    frame->view = Mat4x4::identity() * Mat4x4::rotation(game_state->cam.pitch, Vec3(1, 0, 0)) * Mat4x4::rotation(game_state->cam.yaw, Vec3(0, 1, 0))
        * Mat4x4::translate(-sim->cam_p);
    sim->cam_mvp = frame->projection * frame->view;
    // Get mouse point projected on plane
    Vec3 ray_dir = uv_to_world(frame->projection, frame->view, Vec2((2.0f * mouse_p(input).x) / window_size(input).x - 1.0f,
													           1.0f - (2.0f * mouse_p(input).y) / window_size(input).y));
    f32 t = 0;
    ray_intersect_plane(Vec3(0, 1, 0), 0, sim->cam_p, ray_dir, &t);
    Vec3 mouse_point_xyz = sim->cam_p + ray_dir * t;
    Vec2 mouse_point = Vec2(mouse_point_xyz.x, mouse_point_xyz.z);
    frame->mouse_projection = mouse_point;
    
    update_interactions(game_state, frame, input);
    
    if (game_state->is_in_building_mode) {
        Vec2 p = floor_to_cell(mouse_point);
        if (!is_cell_occupied(sim, p)) {
            SimEntity *building = create_entity(sim);
            building->p = p;
            building->kind = ENTITY_KIND_WORLD_OBJECT;
            building->world_object_kind = WORLD_OBJECT_KIND_BUILDING1 + game_state->selected_building;
            if (is_key_held(input, Key::MouseRight, INPUT_ACCESS_TOKEN_NO_LOCK)) {
                game_state->is_in_building_mode = false;
            } else {
                building->flags |= ENTITY_FLAG_SINGLE_FRAME_LIFESPAN;
                building->world_object_flags |= WORLD_OBJECT_FLAG_IS_BLUEPRINT;
            }
        }
    }
}

static void get_sprite_settings_for_entity(SimEntity *entity, AssetID *id_dest, Vec2 *size_dest) {
    AssetID texture_id = INVALID_ASSET_ID;
    Vec2 size = Vec2(0, 0);
    switch(entity->kind) {
        case ENTITY_KIND_PLAYER: {
            texture_id = Asset_Dude;
            size = Vec2(0.5f);
        } break;
        case ENTITY_KIND_WORLD_OBJECT: {
            size = Vec2(0.5f);
            switch(entity->world_object_kind) {
                case WORLD_OBJECT_KIND_TREE_FOREST: {
                    texture_id = Asset_TreeForest;
                } break;
                case WORLD_OBJECT_KIND_TREE_JUNGLE: {
                    size = Vec2(0.6f);
                    texture_id = Asset_TreeJungle;
                } break;
                case WORLD_OBJECT_KIND_TREE_DESERT: {
                    size = Vec2(0.4f);
                    texture_id = Asset_TreeDesert;
                } break;
                case WORLD_OBJECT_KIND_GOLD_DEPOSIT: {
                    texture_id = Asset_GoldVein;
                } break;
                case WORLD_OBJECT_KIND_BUILDING1: {
                    if (entity->world_object_flags & WORLD_OBJECT_FLAG_IS_BUILT) {
                        texture_id = Asset_Building;
                    } else {
                        texture_id = Asset_Building1;
                    }
                } break;
                case WORLD_OBJECT_KIND_BUILDING2: {
                    texture_id = Asset_Building;
                } break;
                INVALID_DEFAULT_CASE;
            }
        } break;
        INVALID_DEFAULT_CASE;
    }
        
    *id_dest = texture_id;
    *size_dest = size;
}

static void render_world(GameState *game_state, FrameData *frame, RendererCommands *commands, Assets *assets) {
    TIMED_FUNCTION();
    SimRegion *sim = frame->sim;
    RenderGroup world_render_group = render_group_begin(commands, assets, setup_3d(frame->view, frame->projection));
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
    END_BLOCK();
    // Draw grid near mouse cursor
    BEGIN_BLOCK("Show grid");
    if (game_state->show_grid) {
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
    }
    END_BLOCK();
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
        AssetID texture_id;
        Vec2 size;
        get_sprite_settings_for_entity(entity, &texture_id, &size);
        Vec3 billboard[4];
        Vec3 pos = Vec3(entity->p.x, 0, entity->p.y);
        Vec4 color = Colors::white;
        if (entity->kind == ENTITY_KIND_WORLD_OBJECT && entity->world_object_flags & WORLD_OBJECT_FLAG_IS_BLUEPRINT) {
            color.a = 0.5f;
        }
        get_billboard_positions(pos, sim->cam_mvp.get_x(), sim->cam_mvp.get_y(), size.x, size.y, billboard);
        push_quad(&world_render_group, billboard, color, texture_id);
    }
    END_BLOCK();
    
    render_group_end(&world_render_group); 
}

static void render_interface(GameState *game_state, FrameData *frame, RendererCommands *commands, Assets *assets) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Wood: %u", game_state->wood_count);
    game_state->interface.text_for_wood_count->text = alloc_string(&game_state->frame_arena, buffer);
    snprintf(buffer, sizeof(buffer), "Gold: %u", game_state->gold_count);
    game_state->interface.text_for_gold_count->text = alloc_string(&game_state->frame_arena, buffer);
    
    RenderGroup interface_render_group = render_group_begin(commands, assets,
        setup_2d(Mat4x4::ortographic_2d(0, window_size(frame->input).x, window_size(frame->input).y, 0)));
    interface_render(&game_state->interface.interface, &interface_render_group);
    render_group_end(&interface_render_group);
}

void update_and_render(GameState *game_state, InputManager *input, RendererCommands *commands, Assets *assets) {
    TIMED_FUNCTION();
    arena_clear(&game_state->frame_arena);
    FrameData frame = {};
    frame.input = input;
    // Since camera follows some entity and is never too far from it, we can assume that camera position is
    // the position of followed entity
    WorldPosition camera_position = get_world_entity(game_state->world, game_state->camera_followed_entity_id)->world_pos;
    Vec2i min_chunk_coord, max_chunk_coord;
    get_sim_region_bounds(camera_position, &min_chunk_coord, &max_chunk_coord);
    frame.sim = begin_sim(&game_state->frame_arena, game_state->world, min_chunk_coord, max_chunk_coord);
    update_interface(game_state, &frame);
    update_world_simulation(game_state, &frame);
    render_world(game_state, &frame, commands, assets);
    render_interface(game_state, &frame, commands, assets);
    end_sim(frame.sim);
    
    Entity *player = get_world_entity(game_state->world, game_state->camera_followed_entity_id);
    Vec2 player_pos = DEBUG_world_pos_to_p(player->world_pos);
    DEBUG_VALUE(player_pos);
    DEBUG_VALUE(player->world_pos.offset);
    DEBUG_VALUE(player->world_pos.chunk);
}


WorldObjectSettings *get_object_settings(GameState *game_state, u32 world_object_kind) {
    assert(world_object_kind);
    assert(world_object_kind < ARRAY_SIZE(game_state->world_object_settings));
    return game_state->world_object_settings + world_object_kind;
}


void manage_input(InputManager *manager) {
    
}

void lock_input(InputManager *manager, u32 access_token) {
    assert(!manager->access_token);
    manager->access_token = access_token;
}

void unlock_input(InputManager *manager) {
    assert(manager->access_token);
    manager->access_token = INPUT_ACCESS_TOKEN_NO_LOCK;
}

bool is_key_pressed(InputManager *manager, Key key, u32 access_token) {
    bool result = false;
    if (manager->access_token == access_token || access_token == INPUT_ACCESS_TOKEN_ALL) {
        result = manager->input->is_key_pressed(key);
    }
    return result;
}

bool is_key_released(InputManager *manager, Key key, u32 access_token) {
    bool result = false;
    if (manager->access_token == access_token || access_token == INPUT_ACCESS_TOKEN_ALL) {
        // result = manager->input->is_key_released(key);
    }
    return result;
}

bool is_key_held(InputManager *manager, Key key, u32 access_token) {
    bool result = false;
    if (manager->access_token == access_token || access_token == INPUT_ACCESS_TOKEN_ALL) {
        result = manager->input->is_key_held(key);
    }
    return result;
}

f32 get_mwheel(InputManager *manager) {
    return manager->input->mwheel;
}

Vec2 mouse_p(InputManager *manager) {
    return manager->input->mpos;
}

Vec2 mouse_d(InputManager *manager) {
    return manager->input->mdelta;
}

Vec2 window_size(InputManager *manager) {
    return manager->input->winsize;   
}

f32 get_dt(InputManager *manager) {
    return manager->input->dt;
}
