#include "world_state.hh"


inline WorldObjectSpec get_spec_for_type(WorldState *world_state, u32 type) {
    assert(type < WORLD_OBJECT_KIND_SENTINEL);
    return world_state->world_object_specs[type];
}

inline OrderQueueEntry *get_new_order(WorldState *world_state) {
    OrderQueueEntry *entry = world_state->order_free_list;
    if (!entry) {
        ++world_state->orders_allocated;
        entry = alloc_struct(world_state->arena, OrderQueueEntry);
    } else {
        LLIST_POP(world_state->order_free_list);
    }
    CDLIST_ADD(&world_state->order_queue, entry);
    return entry;
}

inline void free_order(WorldState *world_state, OrderQueueEntry *entry) {
    CDLIST_REMOVE(entry);
    LLIST_ADD(world_state->order_free_list, entry);
}

OrderQueueEntry *get_pending_order(WorldState *world_state) {
    OrderQueueEntry *result = 0;
    CDLIST_ITER(&world_state->order_queue, order) {
        result = order;
        break;
    }
    return result;
}

bool is_idle(Entity *entiy) {
    return false;
}

void assign_order(WorldState *world_state, Entity *entity, OrderQueueEntry *order) {
    order->is_assigned = true;
    free_order(world_state, order);
}

static EntityID add_player(SimRegion *sim, Vec2 pos) {
    Entity *entity = create_new_entity(sim, pos);
    entity->kind = ENTITY_KIND_PLAYER;
    entity->flags = ENTITY_FLAG_IS_ANCHOR;
    return entity->id;
}

static EntityID add_tree(SimRegion *sim, Vec2 pos, u32 kind) {
    pos.x = Floor(pos.x) + 0.5f;
    pos.y = Floor(pos.y) + 0.5f;
    Entity *entity = create_new_entity(sim, pos);
    entity->kind = ENTITY_KIND_WORLD_OBJECT;
    entity->flags = ENTITY_FLAG_HAS_WORLD_PLACEMENT;
    entity->world_object_kind = kind;
    entity->resource_interactions_left = 1;
    return entity->id;
}

static EntityID add_pawn(SimRegion *sim, Vec2 pos) {
    Entity *entity = create_new_entity(sim, pos);
    entity->kind = ENTITY_KIND_PAWN;
    return entity->id;
}

inline WorldObjectSpec tree_spec(u32 resource_gain) {
    WorldObjectSpec spec = {};
    spec.type = WORLD_OBJECT_TYPE_RESOURCE;
    spec.resource_kind = RESOURCE_KIND_WOOD;
    spec.default_resource_interactions = resource_gain;
    return spec;
}

inline WorldObjectSpec building_spec() {
    WorldObjectSpec spec = {};
    spec.type = WORLD_OBJECT_TYPE_BUILDING;
    return spec;
}

void world_state_init(WorldState *world_state, MemoryArena *arena, MemoryArena *frame_arena) {    
    world_state->arena = arena;
    world_state->frame_arena = frame_arena;
    // @TODO initialize world properly
    world_state->world = alloc_struct(arena, World);
    world_state->world->arena = arena;
    world_state->world->max_entity_id = 1;
    // Set spec settings
    world_state->world_object_specs[WORLD_OBJECT_KIND_TREE_FOREST] = tree_spec(4);
    world_state->world_object_specs[WORLD_OBJECT_KIND_TREE_JUNGLE] = tree_spec(2);
    world_state->world_object_specs[WORLD_OBJECT_KIND_TREE_DESERT] = tree_spec(1);
    world_state->world_object_specs[WORLD_OBJECT_KIND_BUILDING1] = building_spec();
    world_state->world_object_specs[WORLD_OBJECT_KIND_BUILDING2] = building_spec();
    CDLIST_INIT(&world_state->order_queue);
    
    // Generate world
    Entropy gen_entropy { 123456789 };
    SimRegion *creation_sim = alloc_struct(frame_arena, SimRegion);
    begin_sim(creation_sim, frame_arena, world_state->world, 100, 100, 25);
    Vec2 player_pos = Vec2(0);
    world_state->camera_followed_entity = add_player(creation_sim, player_pos);    
    world_state->pawns[world_state->pawn_count++] = add_pawn(creation_sim, Vec2(5, 5));
    world_state->pawns[world_state->pawn_count++] = add_pawn(creation_sim, Vec2(-5, 5));
    world_state->pawns[world_state->pawn_count++] = add_pawn(creation_sim, Vec2(5, -5));
    world_state->pawns[world_state->pawn_count++] = add_pawn(creation_sim, Vec2(-5, -5));
    world_state->pawns[world_state->pawn_count++] = add_pawn(creation_sim, Vec2(15, 15));
    world_state->pawns[world_state->pawn_count++] = add_pawn(creation_sim, Vec2(-15, 15));
    world_state->pawns[world_state->pawn_count++] = add_pawn(creation_sim, Vec2(15, -15));
    world_state->pawns[world_state->pawn_count++] = add_pawn(creation_sim, Vec2(-15, -15));
    for (u32 i = 0; i < 1000; ++i) {
        for (;;) {
            f32 x = random_bilateral(&gen_entropy) * CHUNK_SIZE * 15;
            f32 y = random_bilateral(&gen_entropy) * CHUNK_SIZE * 15;
            if (!is_cell_occupied(creation_sim, Floor(x), Floor(y))) {
                add_tree(creation_sim, Vec2(x, y), WORLD_OBJECT_KIND_TREE_FOREST);
                break;
            }
        }
    }
    end_sim(creation_sim, world_state);
}

static Vec3 uv_to_world(Mat4x4 projection, Mat4x4 view, Vec2 uv) {
    f32 x = uv.x;
    f32 y = uv.y;
    Vec3 ray_dc = Vec3(x, y, 1.0f);
    Vec4 ray_clip = Vec4(ray_dc.xy, -1.0f, 1.0f);
    Vec4 ray_eye = Mat4x4::inverse(projection) * ray_clip;
    ray_eye.z = -1.0f;
    ray_eye.w = 0.0f;
    Vec3 ray_world = normalize((Mat4x4::inverse(view) * ray_eye).xyz);
    return ray_world;
}
void update_game(WorldState *world_state, SimRegion *sim, InputManager *input) {
    TIMED_FUNCTION();
    if (is_key_held(input, KEY_Z)) {
        f32 x_view_coef = 1.0f * input->platform->frame_dt;
        f32 y_view_coef = 0.6f * input->platform->frame_dt;
        f32 x_angle_change = input->platform->mdelta.x * x_view_coef;
        f32 y_angle_change = input->platform->mdelta.y * y_view_coef;
        world_state->cam.yaw += x_angle_change;
        world_state->cam.pitch += y_angle_change;
#define MIN_CAM_PITCH (HALF_PI * 0.1f)
#define MAX_CAM_PITCH (HALF_PI * 0.9f)
        f32 delta_zoom = input->platform->mwheel;
        world_state->cam.distance_from_player -= delta_zoom;
    }
    world_state->cam.yaw = unwind_rad(world_state->cam.yaw);
    world_state->cam.pitch = Clamp(world_state->cam.pitch, MIN_CAM_PITCH, MAX_CAM_PITCH);
    world_state->cam.distance_from_player = Clamp(world_state->cam.distance_from_player, 0.5f, 1000);
    
    Entity *camera_controlled_entity = get_entity_by_id(sim, world_state->camera_followed_entity);
    assert(camera_controlled_entity);
    // Calculate player movement
    Vec2 player_delta = Vec2(0);
    f32 move_coef = 16.0f * input->platform->frame_dt;
    f32 z_speed = 0;
    if (is_key_held(input, KEY_W)) {
        z_speed = move_coef;
    } else if (is_key_held(input, KEY_S)) {
        z_speed = -move_coef;
    }
    player_delta.x += z_speed *  Sin(world_state->cam.yaw);
    player_delta.y += z_speed * -Cos(world_state->cam.yaw);
    
    f32 x_speed = 0;
    if (is_key_held(input, KEY_D)) {
        x_speed = move_coef;
    } else if (is_key_held(input, KEY_A)) {
        x_speed = -move_coef;
    }
    player_delta.x += x_speed * Cos(world_state->cam.yaw);
    player_delta.y += x_speed * Sin(world_state->cam.yaw);     
    Vec2 new_p = camera_controlled_entity->p + player_delta;
    change_entity_position(sim, camera_controlled_entity, new_p);
    {DEBUG_VALUE_BLOCK("Player")
            DEBUG_VALUE(camera_controlled_entity->p, "Position");
        DEBUG_VALUE(sim->center_chunk_x, "Center chunk x");
        DEBUG_VALUE(sim->center_chunk_y, "Center chunk y");
    }
    
    Vec3 center_pos = xz(camera_controlled_entity->p);
    f32 horiz_distance = world_state->cam.distance_from_player * Cos(world_state->cam.pitch);
    f32 vert_distance = world_state->cam.distance_from_player * Sin(world_state->cam.pitch);
    f32 offsetx = horiz_distance * Sin(-world_state->cam.yaw);
    f32 offsetz = horiz_distance * Cos(-world_state->cam.yaw);
    Vec3 cam_p;
    cam_p.x = offsetx + center_pos.x;
    cam_p.z = offsetz + center_pos.z;
    cam_p.y = vert_distance;
    world_state->cam_p = cam_p;
    
#define CAMERA_FOV rad(60)
#define CAMERA_NEAR_PLANE 0.001f
#define CAMERA_FAR_PLANE  10000.0f
    world_state->projection = Mat4x4::perspective(CAMERA_FOV, input->platform->display_size.aspect_ratio(), CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);
    world_state->view = Mat4x4::identity() * Mat4x4::rotation(world_state->cam.pitch, Vec3(1, 0, 0)) * Mat4x4::rotation(world_state->cam.yaw, Vec3(0, 1, 0))
        * Mat4x4::translate(-cam_p);
    world_state->mvp = world_state->projection * world_state->view;
    
    Vec3 ray_dir = uv_to_world(world_state->projection, world_state->view, Vec2((2.0f * input->platform->mpos.x) / input->platform->display_size.x - 1.0f,
                                                                                1.0f - (2.0f * input->platform->mpos.y) / input->platform->display_size.y));
    f32 t = 0;
    ray_intersect_plane(Vec3(0, 1, 0), 0, cam_p, ray_dir, &t);
    Vec3 mouse_point_xyz = cam_p + ray_dir * t;
    Vec2 mouse_point = Vec2(mouse_point_xyz.x, mouse_point_xyz.z);
    world_state->mouse_projection = mouse_point;
    // Find entity to be selected with mouse
    world_state->mouse_selected_entity = NULL_ENTITY_ID;
    f32 min_distance = INFINITY;
#define DISTANCE_TO_MOUSE_SELECT (1.0f)
#define DISTANCE_TO_MOUSE_SELECT_SQ (DISTANCE_TO_MOUSE_SELECT * DISTANCE_TO_MOUSE_SELECT)
    ITERATE(iter, iterate_entities(sim, iter_radius(world_state->mouse_projection, DISTANCE_TO_MOUSE_SELECT))) {
        Entity *entity = get_entity_by_id(sim, *iter.ptr);
        f32 distance_to_mouse_sq = length_sq(world_state->mouse_projection - entity->p);
        if (distance_to_mouse_sq < DISTANCE_TO_MOUSE_SELECT_SQ && distance_to_mouse_sq < min_distance) {
            world_state->mouse_selected_entity = entity->id;
            min_distance = distance_to_mouse_sq;
        }
    }
    // Add selected entity to job queue if it fits
    if (is_key_pressed(input, KEY_MOUSE_LEFT)) {
        if (is_not_null(world_state->mouse_selected_entity)) {
            Entity *entity = get_entity_by_id(sim, world_state->mouse_selected_entity);
            if (entity->kind == ENTITY_KIND_WORLD_OBJECT) {
                WorldObjectSpec spec = get_spec_for_type(world_state, entity->world_object_kind);
                if (spec.type == WORLD_OBJECT_TYPE_RESOURCE) {
                    OrderQueueEntry *entry = get_new_order(world_state);
                    entry->kind = ORDER_QUEUE_ENTRY_CHOP;
                    entry->destination_id = world_state->mouse_selected_entity;
                }
            }
            
        }
    }
    
    Vec2 player_pos = camera_controlled_entity->p;
    for (u32 pawn_idx = 0; 
         pawn_idx < world_state->pawn_count;
         ++pawn_idx) {
        EntityID pawn_id = world_state->pawns[pawn_idx];
        Entity *entity = get_entity_by_id(sim, pawn_id);
#if 0
        OrderQueueEntry *order = get_pending_order(world_state);
        if (order && is_idle(entity)) {
            assign_order(world_state, entity, order);
        }
#endif
#define PAWN_DISTANCE_TO_PLAYER 3.0f
#define PAWN_DISTANCE_TO_PLAYER_SQ SQ(PAWN_DISTANCE_TO_PLAYER)
        Vec2 delta = player_pos - entity->p;
#define PAWN_SPEED 3.0f
        if (length_sq(delta) > PAWN_DISTANCE_TO_PLAYER_SQ) {
            Vec2 delta_p = normalize(delta) * PAWN_SPEED * input->platform->frame_dt;
            Vec2 new_pawn_p = entity->p + delta_p;
            change_entity_position(sim, entity, new_pawn_p);
        }
    }
    // Assign job to pawn if 
    DEBUG_VALUE(world_state->orders_allocated, "Orders allocated");
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

void render_game(WorldState *world_state, SimRegion *sim, RendererCommands *commands, Assets *assets) {
    RenderGroup world_render_group = render_group_begin(commands, assets,
                                                        setup_3d(RENDERER_FRAMEBUFFER_GAME_WORLD, world_state->view, world_state->projection));
    
    u32 chunk_count = get_chunk_count_for_radius(sim->chunk_radius);
    AssetID ground_tex = assets_get_first_of_type(assets, ASSET_TYPE_GRASS);
    BEGIN_BLOCK("Ground render");
    for (u32 i = 0; i < chunk_count; ++i) {
        SimRegionChunk *chunk = sim->chunks + i;
#define WORLD_EPSILON 0.01f
        Vec3 p[4] = {
            Vec3(chunk->chunk_x, 0, chunk->chunk_y) * CHUNK_SIZE,
            Vec3(chunk->chunk_x, 0, chunk->chunk_y + 1) * CHUNK_SIZE,
            Vec3(chunk->chunk_x + 1, 0, chunk->chunk_y) * CHUNK_SIZE,
            Vec3(chunk->chunk_x + 1, 0, chunk->chunk_y + 1) * CHUNK_SIZE,
        };
        Vec3 p1[4];
        memcpy(p1, p, sizeof(p));
        p1[0].y = WORLD_EPSILON;
        p1[1].y = WORLD_EPSILON;
        p1[2].y = WORLD_EPSILON;
        p1[3].y = WORLD_EPSILON;
        push_quad(&world_render_group, p, ground_tex);
        if (world_state->draw_frames) {
            push_quad_outline(&world_render_group, p1[0], p1[1], p1[2], p1[3], BLACK, 0.05f);
        }
    }
    Vec2 mouse_cell_pos = Vec2(Floor(world_state->mouse_projection.x), Floor(world_state->mouse_projection.y));
    DEBUG_VALUE(mouse_cell_pos, "Selected cell");
    DEBUG_VALUE(is_cell_occupied(sim, mouse_cell_pos.x, mouse_cell_pos.y), "Is occupied");
    if (world_state->draw_frames) {
#define MOUSE_CELL_RAD 10
        for (i32 dy = -MOUSE_CELL_RAD / 2; dy <= MOUSE_CELL_RAD / 2; ++dy) {
            for (i32 dx = -MOUSE_CELL_RAD / 2; dx <= MOUSE_CELL_RAD / 2; ++dx) {
                bool is_occupied = is_cell_occupied(sim, mouse_cell_pos.x + dx, mouse_cell_pos.y + dy);
                if (!is_occupied) {
                    Vec3 v0 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE, WORLD_EPSILON, mouse_cell_pos.y + dy * CELL_SIZE);
                    Vec3 v1 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE, WORLD_EPSILON, mouse_cell_pos.y + dy * CELL_SIZE + CELL_SIZE);
                    Vec3 v2 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE + CELL_SIZE, WORLD_EPSILON, mouse_cell_pos.y + dy * CELL_SIZE);
                    Vec3 v3 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE + CELL_SIZE, WORLD_EPSILON, mouse_cell_pos.y + dy * CELL_SIZE + CELL_SIZE);
                    push_quad_outline(&world_render_group, v0, v1, v2, v3, BLACK, 0.05f);
                } else {
                    Vec3 v0 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE, WORLD_EPSILON * 5.0f, mouse_cell_pos.y + dy * CELL_SIZE);
                    Vec3 v1 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE, WORLD_EPSILON * 5.0f, mouse_cell_pos.y + dy * CELL_SIZE + CELL_SIZE);
                    Vec3 v2 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE + CELL_SIZE, WORLD_EPSILON * 5.0f, mouse_cell_pos.y + dy * CELL_SIZE);
                    Vec3 v3 = Vec3(mouse_cell_pos.x + dx * CELL_SIZE + CELL_SIZE, WORLD_EPSILON * 5.0f, mouse_cell_pos.y + dy * CELL_SIZE + CELL_SIZE);
                    push_quad_outline(&world_render_group, v0, v1, v2, v3, RED, 0.05f);
                }
            }
        }
    }
    
    if (is_not_null(world_state->mouse_selected_entity)) {
        Entity *entity = get_entity_by_id(sim, world_state->mouse_selected_entity);
        assert(entity);
        Vec2 half_size = Vec2(1, 1) * 0.5f;
        Vec3 v[4];
        v[0] = xz(entity->p + Vec2(-half_size.x, -half_size.y), WORLD_EPSILON);
        v[1] = xz(entity->p + Vec2(-half_size.x, half_size.y),  WORLD_EPSILON);
        v[2] = xz(entity->p + Vec2(half_size.x, -half_size.y),  WORLD_EPSILON);
        v[3] = xz(entity->p + Vec2(half_size.x, half_size.y),   WORLD_EPSILON);
        AssetID select_tex_id = assets_get_first_of_type(assets, ASSET_TYPE_ADDITIONAL);
        push_quad(&world_render_group, v, select_tex_id);
    }
    END_BLOCK();
    
    BEGIN_BLOCK("Render entities");
    SortEntry *sort_a = alloc_arr(world_state->frame_arena, sim->entity_count, SortEntry);
    SortEntry *sort_b = alloc_arr(world_state->frame_arena, sim->entity_count, SortEntry);
    Vec3 cam_z = world_state->mvp.get_z();
    for (size_t entity_idx = 0; entity_idx < sim->entity_count; ++entity_idx) {
        sort_a[entity_idx].sort_key = dot(cam_z, xz(sim->entities[entity_idx].p) - world_state->cam_p);
        sort_a[entity_idx].sort_index = entity_idx;
    }
    radix_sort(sort_a, sort_b, sim->entity_count);
    Vec3 cam_x = world_state->mvp.get_x();
    Vec3 cam_y = world_state->mvp.get_y();
    for (size_t sorted_idx = 0; sorted_idx < sim->entity_count; ++sorted_idx) {
        Entity *entity = sim->entities + sort_a[sim->entity_count - sorted_idx - 1].sort_index;
        AssetID texture_id;
        switch (entity->kind) {
            case ENTITY_KIND_PLAYER: {
                texture_id = assets_get_first_of_type(assets, ASSET_TYPE_PLAYER);
            } break;
            case ENTITY_KIND_WORLD_OBJECT: {
                AssetTagList match_tags = {};
                AssetTagList weight_tags = {};
                match_tags.tags[ASSET_TAG_WORLD_OBJECT_KIND] = entity->world_object_kind;
                weight_tags.tags[ASSET_TAG_WORLD_OBJECT_KIND] = 1.0f;
                texture_id = assets_get_closest_match(assets, ASSET_TYPE_WORLD_OBJECT, &weight_tags, &match_tags);
            } break;
            case ENTITY_KIND_PAWN: {
                texture_id = assets_get_first_of_type(assets, ASSET_TYPE_PAWN);
            } break;
            INVALID_DEFAULT_CASE;
        }
        Vec3 v[4];
        get_billboard_positions(xz(entity->p), cam_x, cam_y, 1.5f, 1.5f, v);
        push_quad(&world_render_group, v, texture_id);
    }
    
    END_BLOCK();
    render_group_end(&world_render_group);
}

void update_and_render_world_state(WorldState *world_state, InputManager *input, RendererCommands *commands, Assets *assets) {
    
    u32 sim_region_count = world_state->anchor_count;
    // Zero anchor count so it can be set again from different sim regions
    world_state->anchor_count = 0;
    SimRegion *sim_regions = alloc_arr(world_state->frame_arena, sim_region_count, SimRegion);
    u32 total_sim_entities = 0;
    u32 total_sim_chunks = 0;
    for (u32 anchor_idx = 0; anchor_idx < sim_region_count; ++anchor_idx) {
        Anchor *anchor = world_state->anchors + anchor_idx;
        SimRegion *sim = sim_regions + anchor_idx;
        begin_sim(sim, world_state->frame_arena, world_state->world, anchor->chunk_x, anchor->chunk_y, anchor->radius);
        total_sim_entities += sim->entity_count;
        total_sim_chunks += sim->chunks_count;
        update_game(world_state, sim, input);
        render_game(world_state, sim, commands, assets);
        end_sim(sim, world_state);
    }
    {DEBUG_VALUE_BLOCK("World")
            DEBUG_SWITCH(&world_state->draw_frames, "Frames");
        DEBUG_VALUE(world_state->anchor_count, "Anchor count");
        DEBUG_VALUE(world_state->world->chunks_allocated, "Chunks allocated");
        DEBUG_VALUE(world_state->world->entity_blocks_allocated, "Entity blocks allocated");
        DEBUG_VALUE(world_state->world->entity_ids_allocated, "Entity ids allocated");
        DEBUG_VALUE(total_sim_entities, "Total sim entities");
        DEBUG_VALUE(total_sim_chunks, "Total sim chunks");
    }
}
