#include "game/debug.hh"

#include "os.hh"
#include "game/game_state.hh"

DebugTable *debug_table;

static DebugValueBlock *get_value_block(DebugState *debug_state) {
    DebugValueBlock *block = debug_state->first_free_value_block;
    if (block) {
        debug_state->first_free_value_block = block->next;
    } else {
        ++debug_state->value_blocks_allocated;
        block = alloc_struct(&debug_state->arena, DebugValueBlock);
    }
    return block;
}

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
        DebugOpenBlock *current_open_block = 0;
        DebugValueBlock *current_value_block = 0;
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
                        ++debug_state->debug_open_blocks_allocated;
                        debug_block = alloc_struct(&debug_state->arena, DebugOpenBlock);
                    }
                    
                    debug_block->frame_index = debug_state->frame_index;
                    debug_block->opening_event = event;
                    debug_block->parent = current_open_block;
                    debug_block->next_free = 0;
                    
                    current_open_block = debug_block;
                } break;
                case DEBUG_EVENT_END_BLOCK: {
                    assert(current_open_block);
                    DebugOpenBlock *matching_block = current_open_block;
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
                    current_open_block = matching_block->parent;
                } break;
#define DEBUG_EVENT_VALUE_DEF(_type)                            \
case DEBUG_EVENT_VALUE_##_type: {                               \
    DebugValue *value = debug_state->first_free_value;          \
    if (value) {                                                \
        debug_state->first_free_value = value->next;            \
    } else {                                                    \
        debug_state->debug_values_allocated++;                  \
        value = alloc_struct(&debug_state->arena, DebugValue);  \
    }                                                           \
    value->value_kind = DEBUG_VALUE_##_type;                    \
    value->value_##_type = event->value_##_type;                \
    value->name = event->name;                                  \
    if (!current_value_block) {                                 \
        value->next = debug_state->first_value;                 \
        debug_state->first_value = value;                       \
    } else {                                                    \
        value->next = current_value_block->first_value;         \
        current_value_block->first_value = value;               \
    }                                                           \
} break;
DEBUG_EVENT_VALUE_DEF(u64)
DEBUG_EVENT_VALUE_DEF(f32)
DEBUG_EVENT_VALUE_DEF(Vec2)
DEBUG_EVENT_VALUE_DEF(Vec3)
DEBUG_EVENT_VALUE_DEF(Vec2i)
                case DEBUG_EVENT_VALUE_SWITCH: {
                    DebugValue *value = debug_state->first_free_value;         
                    if (value) {                                               
                        debug_state->first_free_value = value->next;           
                    } else {                          
                        ++debug_state->debug_values_allocated;                         
                        value = alloc_struct(&debug_state->arena, DebugValue); 
                    }                                                          
                    value->value_kind = DEBUG_VALUE_SWITCH;                   
                    value->value_switch = event->value_switch;               
                    value->name = event->name;                                 
                    if (!current_value_block) {
                        value->next = debug_state->first_value;                    
                        debug_state->first_value = value;                          
                    } else {
                        value->next = current_value_block->first_value;
                        current_value_block->first_value = value;
                    }
                } break; 
                case DEBUG_EVENT_BEGIN_VALUE_BLOCK: {
                    assert(!current_value_block);
                    DebugValueBlock *block = get_value_block(debug_state);
                    block->name = event->name;
                    block->next = debug_state->first_value_block;
                    debug_state->first_value_block = block;
                    
                    current_value_block = block;
                } break;
                case DEBUG_EVENT_END_VALUE_BLOCK: {
                    assert(current_value_block);
                    current_value_block = 0;
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

void DEBUG_update(DebugState *debug_state, InputManager *input, RendererCommands *commands, Assets *assets) {
    TIMED_FUNCTION();
    if (is_key_pressed(input, KEY_F1, INPUT_ACCESS_TOKEN_ALL)) {
        debug_state->dev_mode = DEV_MODE_NONE;
    }
    if (is_key_pressed(input, KEY_F2, INPUT_ACCESS_TOKEN_ALL)) {
        debug_state->dev_mode = DEV_MODE_INFO;
    }
    if (is_key_pressed(input, KEY_F3, INPUT_ACCESS_TOKEN_ALL)) {
        debug_state->dev_mode = DEV_MODE_PROFILER;
    }
    if (is_key_pressed(input, KEY_F4, INPUT_ACCESS_TOKEN_ALL)) {
        debug_state->is_paused = !debug_state->is_paused;
    } 
    if (is_key_pressed(input, KEY_F5, INPUT_ACCESS_TOKEN_ALL)) {
        debug_state->dev_mode = DEV_MODE_MEMORY;
    }
    DevUILayout dev_ui = dev_ui_begin(&debug_state->dev_ui, input, assets);
    
    RenderGroup interface_render_group = render_group_begin(commands, assets,
        setup_2d(Mat4x4::ortographic_2d(0, window_size(input).x, window_size(input).y, 0)));
    if (debug_state->dev_mode == DEV_MODE_INFO) {
        dev_ui_labelf(&dev_ui, "FPS: %.3f; DT: %ums;", 1.0f / get_dt(input), (u32)(get_dt(input) * 1000));
        if (dev_ui_section(&dev_ui, "Variables")) {
            for (DebugValueBlock *block = debug_state->first_value_block;
                 block;
                 block = block->next) {
                if (dev_ui_section(&dev_ui, block->name)) {
                    for (DebugValue *value = block->first_value;
                        value;
                        value = value->next) {
                        char buffer[64];
                        switch (value->value_kind) {
                            case DEBUG_VALUE_f32: {
                                snprintf(buffer, sizeof(buffer), "%.2f", value->value_f32);
                            } break;
                            case DEBUG_VALUE_u64: {
                                snprintf(buffer, sizeof(buffer), "%llu", value->value_u64);
                            } break;
                            case DEBUG_VALUE_Vec2: {
                                snprintf(buffer, sizeof(buffer), "(%.2f %.2f)", value->value_Vec2.x, value->value_Vec2.y);
                            } break;
                            case DEBUG_VALUE_Vec2i: {
                                snprintf(buffer, sizeof(buffer), "(%d %d)", value->value_Vec2i.x, value->value_Vec2i.y);
                            } break;
                            case DEBUG_VALUE_Vec3: {
                                snprintf(buffer, sizeof(buffer), "(%.2f %.2f %.2f)", value->value_Vec3.x, value->value_Vec3.y, value->value_Vec3.z);
                            } break;
                            case DEBUG_VALUE_SWITCH: {
                                snprintf(buffer, sizeof(buffer), "%s: %s", value->name, *value->value_switch ? "true" : "false");       
                            } break;
                        }
                        
                        if (value->value_kind == DEBUG_VALUE_SWITCH) {
                            dev_ui_checkbox(&dev_ui, buffer, value->value_switch);
                        } else {
                            dev_ui_labelf(&dev_ui, "%s: %s", value->name, buffer);
                        }
                    }
                    dev_ui_end_section(&dev_ui);
                }            
            }
            for (DebugValue *value = debug_state->first_value;
                value;
                value = value->next) {
                char buffer[64];
                switch (value->value_kind) {
                    case DEBUG_VALUE_f32: {
                        snprintf(buffer, sizeof(buffer), "%.2f", value->value_f32);
                    } break;
                    case DEBUG_VALUE_u64: {
                        snprintf(buffer, sizeof(buffer), "%llu", value->value_u64);
                    } break;
                    case DEBUG_VALUE_Vec2: {
                        snprintf(buffer, sizeof(buffer), "(%.2f %.2f)", value->value_Vec2.x, value->value_Vec2.y);
                    } break;
                    case DEBUG_VALUE_Vec2i: {
                        snprintf(buffer, sizeof(buffer), "(%d %d)", value->value_Vec2i.x, value->value_Vec2i.y);
                    } break;
                    case DEBUG_VALUE_Vec3: {
                        snprintf(buffer, sizeof(buffer), "(%.2f %.2f %.2f)", value->value_Vec3.x, value->value_Vec3.y, value->value_Vec3.z);
                    } break;
                    case DEBUG_VALUE_SWITCH: {
                        snprintf(buffer, sizeof(buffer), "%s: %s", value->name, *value->value_switch ? "true" : "false");       
                    } break;
                }
                
                if (value->value_kind == DEBUG_VALUE_SWITCH) {
                    dev_ui_checkbox(&dev_ui, buffer, value->value_switch);
                } else {
                    dev_ui_labelf(&dev_ui, "%s: %s", value->name, buffer);
                }
            }
            dev_ui_end_section(&dev_ui);
        }
        const char *INPUT_ACCESS_NAMES[] = {
            "NO_LOCK", 
            "GAME_INTERFACE",  
            "GAME_MENU",  
            "DEV_UI",  
            "ALL",  
        };
        dev_ui_labelf(&dev_ui, "Input lock: %s", INPUT_ACCESS_NAMES[input->access_token]);
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
        dev_ui_labelf(&dev_ui, "Frame %llu", debug_state->total_frame_count);    
        dev_ui_labelf(&dev_ui, "Collation: %.2f%%", (f32)frame->collation_clocks / frame_time * 100);    
        for (size_t i = 0; i < min(frame->records_count, 20); ++i) {
            DebugRecord *record = frame->records + records_sorted[i];
            dev_ui_labelf(&dev_ui, "%2llu %32s %8llu %4u %8llu %.2f%%\n", i, record->name, record->total_clocks, 
                record->times_called, record->total_clocks / (u64)record->times_called, ((f32)record->total_clocks / frame_time * 100));
        }
        temp_memory_end(records_sort_temp);
    } else if (debug_state->dev_mode == DEV_MODE_MEMORY) {
    }
    
    dev_ui_end(&dev_ui, &interface_render_group);
}

void DEBUG_begin_frame(DebugState *debug_state) {
}

void DEBUG_frame_end(DebugState *debug_state) {
    ++debug_state->total_frame_count;
    ++debug_table->current_event_array_index;
    if (debug_table->current_event_array_index >= DEBUG_MAX_EVENT_ARRAY_COUNT) {
        debug_table->current_event_array_index = 0;
    }
    
    u64 event_array_index_event_index = _InterlockedExchange64((volatile i64 *)&debug_table->event_array_index_event_index,
                                                             (i64)debug_table->current_event_array_index << 32);
    u32 event_array_index = event_array_index_event_index >> 32;
    u32 event_count       = event_array_index_event_index & UINT32_MAX;
    debug_table->event_counts[event_array_index] = event_count;
    
    for (DebugValue *value = debug_state->first_value;
         value;
         ) {
        if (!value->next) {
            value->next = debug_state->first_free_value;
            break; 
        } else {
            value = value->next;
        }
    }
    debug_state->first_free_value = debug_state->first_value;
    debug_state->first_value = 0;
    
    for (DebugValueBlock *block = debug_state->first_value_block;
         block;
         ) {
        for (DebugValue *value = block->first_value;
            value;
            ) {
            if (!value->next) {
                value->next = debug_state->first_free_value;
                break; 
            } else {
                value = value->next;
            }
        }
        debug_state->first_free_value = block->first_value;
        block->first_value = 0;

        if (!block->next) {
            block->next = debug_state->first_free_value_block;
            break; 
        } else {
            block = block->next;
        }
    }
    debug_state->first_free_value_block = debug_state->first_value_block;
    debug_state->first_value_block = 0;
    
    if (!debug_state->is_paused) {
        debug_collate_events(debug_state, debug_table->current_event_array_index);
    }
    
}

DebugState *DEBUG_init() {
#define DEBUG_ARENA_SIZE MEGABYTES(256)
    DebugState *debug_state = bootstrap_alloc_struct(DebugState, arena, DEBUG_ARENA_SIZE);
    debug_table = &debug_state->debug_table;
    size_t dev_ui_arena_size = MEGABYTES(8);
    debug_state->dev_ui.arena = subarena(&debug_state->arena, dev_ui_arena_size);
    return debug_state;
}