#include "game/debug.hh"

#include "framework/os.hh"
#include "game/game_state.hh"

DebugTable *debug_table;
DebugStatistics *DEBUG;

static void debug_collate_events(DebugState *debug_state, u32 invalid_event_array_index) {
    u64 collation_start_clock = __rdtsc();
    for (;; ++debug_state->collation_array_index) {
        if (debug_state->collation_array_index == DEBUG_MAX_EVENT_ARRAY_COUNT) {
            debug_state->collation_array_index = 0;
        }
        
        u32 event_array_index = debug_state->collation_array_index;
        if (event_array_index == invalid_event_array_index) {
            break;
        }
        
        DebugFrame *collation_frame = debug_state->frames + debug_state->frame_index;
        for (u32 event_index = 0; event_index < debug_table->event_counts[event_array_index]; ++event_index) {
            DebugEvent *event = debug_table->events[event_array_index] + event_index;
            
            switch (event->type) {
                case DEBUG_EVENT_FRAME_MARKER: {
                    collation_frame->end_clock = event->clock;
                    debug_state->frame_index = (debug_state->frame_index + 1) % DEBUG_MAX_FRAME_COUNT;
                    
                    collation_frame = debug_state->frames + debug_state->frame_index;
                    memset(collation_frame, 0, sizeof(*collation_frame));
                    collation_frame->begin_clock = event->clock;
                } break;
                case DEBUG_EVENT_BEGIN_BLOCK: {
                    DebugOpenBlock *debug_block = debug_state->first_free_block;
                    if (debug_block) {
                        debug_state->first_free_block = debug_block->next_free;
                    } else {
                        debug_block = alloc_struct(&debug_state->arena, DebugOpenBlock);
                    }
                    
                    debug_block->frame_index = debug_state->frame_index;
                    debug_block->opening_event = event;
                    debug_block->parent = debug_state->current_open_block;
                    debug_block->next_free = 0;
                    
                    debug_state->current_open_block = debug_block;
                } break;
                case DEBUG_EVENT_END_BLOCK: {
                    assert(debug_state->current_open_block);
                    DebugOpenBlock *matching_block = debug_state->current_open_block;
                    DebugEvent *opening_event = matching_block->opening_event;
                    // Get debug record from frame hash
                    DebugRecord *record = 0;
#if 1
                    u32 hash_value = (u32)(uintptr_t)opening_event->debug_name;
#else 
                    u32 hash_value = crc32_cstr(opening_event->debug_name);
#endif 
                    u32 hash_mask = (DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME - 1);
                    u32 hash_slot = hash_value & hash_mask;
                    for (size_t offset = 0; offset < DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME; ++offset) {
                        u32 hash_index = ((hash_value + offset) & hash_mask);
                        DebugRecordHash *test = collation_frame->records_hash + hash_index;
                        if (test->debug_name_hash == 0) {
                            test->debug_name_hash = hash_value;
                            assert(collation_frame->records_count < DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME);
                            test->index = collation_frame->records_count++;
                            record = collation_frame->records + test->index;
                            record->debug_name = opening_event->debug_name;
                            record->name = opening_event->name;
                            break;
                        } else if (test->debug_name_hash == hash_value) {
                            record = collation_frame->records + test->index;
                            break;
                        }
                    }
                    assert(record);
                    
                    ++record->times_called;
                    record->total_clocks += (event->clock - opening_event->clock);
                    
                    matching_block->next_free = debug_state->first_free_block;
                    debug_state->first_free_block = matching_block;
                    debug_state->current_open_block = matching_block->parent;
                } break;
                INVALID_DEFAULT_CASE;
            }
        }
    }
    u64 collation_end_clock = __rdtsc();
    debug_state->frames[debug_state->frame_index].collation_clocks = collation_end_clock - collation_start_clock;
}

static int records_sort(void *ctx, const void *a, const void *b) {
    DebugFrame *frame = (DebugFrame *)ctx;
    u32 index_a = *(u32 *)a;
    u32 index_b = *(u32 *)b;
    return frame->records[index_a].total_clocks < frame->records[index_b].total_clocks ? 1 : -1;
}

void DEBUG_update(DebugState *debug_state, GameState *game_state, Input *input, RendererCommands *commands) {
    TIMED_FUNCTION();
    if (input->is_key_pressed(Key::F1)) {
        debug_state->dev_mode = DEV_MODE_NONE;
    }
    if (input->is_key_pressed(Key::F2)) {
        debug_state->dev_mode = DEV_MODE_INFO;
    }
    if (input->is_key_pressed(Key::F3)) {
        debug_state->dev_mode = DEV_MODE_PROFILER;
    }
    if (input->is_key_pressed(Key::F4)) {
        debug_state->is_paused = !debug_state->is_paused;
    } 
    if (input->is_key_pressed(Key::F5)) {
        debug_state->dev_mode = DEV_MODE_MEMORY;
    }
    debug_state->dev_ui.mouse_d = input->mdelta;
    debug_state->dev_ui.mouse_p = input->mpos;
    debug_state->dev_ui.is_mouse_pressed = input->is_key_held(Key::MouseLeft);
    DevUILayout dev_ui = dev_ui_begin(&debug_state->dev_ui);
    
    RenderGroup interface_render_group = render_group_begin(commands, debug_state->assets,
        setup_2d(Mat4x4::ortographic_2d(0, input->winsize.x, input->winsize.y, 0)));
    if (debug_state->dev_mode == DEV_MODE_INFO) {
        dev_ui_labelf(&dev_ui, "FPS: %.3f; DT: %ums; D: %llu; E: %llu; S: %llu", 1.0f / input->dt, (u32)(input->dt * 1000), 
            DEBUG->draw_call_count, game_state->world->entity_count,
            DEBUG->last_frame_sim_region_entity_count);
        Entity *player = get_world_entity(game_state->world, game_state->camera_followed_entity_id);
        Vec2 player_pos = DEBUG_world_pos_to_p(player->world_pos);
        dev_ui_labelf(&dev_ui, "P: (%.2f %.2f); O: (%.3f %.3f); Chunk: (%d %d)", 
            player_pos.x, player_pos.y,
            player->world_pos.offset.x, player->world_pos.offset.y,
            player->world_pos.chunk.x, player->world_pos.chunk.y);
        dev_ui_labelf(&dev_ui, "Wood: %u; Gold: %u", game_state->wood_count, game_state->gold_count);    
        dev_ui_checkbox(&dev_ui, "In building mode", &game_state->is_in_building_mode);
        dev_ui_checkbox(&dev_ui, "Allow camera controls", &game_state->allow_camera_controls);
        if (!is_same(game_state->interactable, null_id())) {
            SimEntity *interactable = &get_world_entity(game_state->world, game_state->interactable)->sim;
            if (interactable->world_object_flags & WORLD_OBJECT_FLAG_IS_BUILDING) {
                dev_ui_labelf(&dev_ui, "Building build progress: %.2f", interactable->build_progress);
            }
        }
        if (game_state->interaction_kind) {
            dev_ui_labelf(&dev_ui, "I: %u%%", (u32)(game_state->interaction_current_time / game_state->interaction_time * 100));    
        }
    } else if (debug_state->dev_mode == DEV_MODE_PROFILER) {
        DebugFrame *frame = debug_state->frames + (debug_state->frame_index ? debug_state->frame_index - 1: DEBUG_MAX_FRAME_COUNT - 1);
        // DebugFrame *frame = game->debug_state->frames;
        f32 frame_time = (f32)(frame->end_clock - frame->begin_clock);
        u64 record_count = frame->records_count;
        TempMemory records_sort_temp = temp_memory_begin(&debug_state->arena);
        u32 *records_sorted = alloc_arr(&debug_state->arena, record_count, u32);
        for (size_t i = 0; i < record_count; ++i) {
            records_sorted[i] = i;
        }
        
        qsort_s(records_sorted, record_count, sizeof(*records_sorted), records_sort, frame);
        dev_ui_labelf(&dev_ui, "Collation: %.2f%%", (f32)frame->collation_clocks / frame_time * 100);    
        for (size_t i = 0; i < frame->records_count; ++i) {
            DebugRecord *record = frame->records + records_sorted[i];
            dev_ui_labelf(&dev_ui, "%2llu %32s %8llu %4u %8llu %.2f%%\n", i, record->name, record->total_clocks, 
                record->times_called, record->total_clocks / (u64)record->times_called, ((f32)record->total_clocks / frame_time * 100));
        }
        temp_memory_end(records_sort_temp);
    } else if (debug_state->dev_mode == DEV_MODE_MEMORY) {
        // dev_ui_labelf(&dev_ui, "Debug Arena: %llu/%llu (%.2f%%)", game->debug_state->collate_arena.data_size, game->debug_state->collate_arena.data_capacity,
        //     game->debug_state->collate_arena.data_size * 100.0f / game->debug_state->collate_arena.data_capacity);
        // dev_ui_labelf(&dev_ui, "Renderer Arena: %llu/%llu (%.2f%%)", game->renderer.arena.data_size, game->renderer.arena.data_capacity,
        //     game->renderer.arena.data_size * 100.0f / game->renderer.arena.data_capacity);
        // dev_ui_labelf(&dev_ui, "Assets Arena: %llu/%llu (%.2f%%)", game->assets.arena.data_size, game->assets.arena.data_capacity,
        //     game->assets.arena.data_size * 100.0f /  game->assets.arena.data_capacity);
        // dev_ui_labelf(&dev_ui, "Frame Arena: %llu/%llu (%.2f%%)", game->game_state.frame_arena.data_size, game->game_state.frame_arena.data_capacity,
        //     game->game_state.frame_arena.data_size * 100.0f / game->game_state.frame_arena.data_capacity);
        // dev_ui_labelf(&dev_ui, "Game Arena: %llu/%llu (%.2f%%)", game->game_state.arena.data_size, game->game_state.arena.data_capacity,
        //     game->game_state.arena.data_size * 100.0f / game->game_state.arena.data_capacity);
    }
    
    dev_ui_end(&dev_ui, &interface_render_group);
}

void DEBUG_frame_end(DebugState *debug_state) {
    ++debug_table->current_event_array_index;
    if (debug_table->current_event_array_index >= DEBUG_MAX_EVENT_ARRAY_COUNT) {
        debug_table->current_event_array_index = 0;
    }
    
    u64 event_array_index_event_index = _InterlockedExchange64((volatile i64 *)&debug_table->event_array_index_event_index,
                                                             (i64)debug_table->current_event_array_index << 32);
    u32 event_array_index = event_array_index_event_index >> 32;
    u32 event_count       = event_array_index_event_index & UINT32_MAX;
    debug_table->event_counts[event_array_index] = event_count;
    
    if (!debug_state->is_paused) {
        debug_collate_events(debug_state, debug_table->current_event_array_index);
    }
}

void DEBUG_init(DebugState *debug_state, Assets *assets) {
    debug_table = &debug_state->debug_table;
    DEBUG = &debug_state->statistics;
    
    debug_state->assets = assets;
    debug_state->first_free_block = 0;
    debug_state->current_open_block = 0;
    debug_state->collation_array_index = 0;    
    
    size_t dev_ui_arena_size = MEGABYTES(8);
    debug_state->dev_ui.arena = subarena(&debug_state->arena, dev_ui_arena_size);
    dev_ui_init(&debug_state->dev_ui, assets);
}

DebugState *DEBUG_create() {
    DebugState *debug_state = (DebugState *)os_alloc(sizeof(DebugState));
    size_t debug_arena_size = MEGABYTES(256);
    arena_init(&debug_state->arena, os_alloc(debug_arena_size), debug_arena_size);
    return debug_state;
    
}