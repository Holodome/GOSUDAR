#include "debug.hh"

#if INTERNAL_BUILD

#include "os.hh"
#include "game_state.hh"

DebugTable *debug_table;

static DebugValueBlock *get_value_block(DebugState *debug_state) {
    DebugValueBlock *block = debug_state->first_free_value_block;
    if (block) {
        debug_state->first_free_value_block = block->next;
    } else {
        ++debug_state->value_blocks_allocated;
        block = alloc_struct(&debug_state->arena, DebugValueBlock);
    }
    block->first_child = 0;
    block->first_value = 0;
    block->next = 0;
    return block;
}

static DebugValue *get_debug_value(DebugState *debug_state) {
    DebugValue *value = debug_state->first_free_value;         
    if (value) {                                               
        debug_state->first_free_value = value->next;           
    } else {                          
        ++debug_state->debug_values_allocated;                         
        value = alloc_struct(&debug_state->arena, DebugValue); 
    }
    return value;
}

static void debug_collate_events(DebugState *debug_state, u32 invalid_event_array_index) {
    // Free values and value blocks
    DebugValueBlock *value_block_stack[DEBUG_VALUE_BLOCK_MAX_DEPTH] = {};
    u32 current_value_block_stack_index = 0;
    value_block_stack[0] = debug_state->global_value_block;
    for (;;) {
        while (!value_block_stack[current_value_block_stack_index] && current_value_block_stack_index) {
            --current_value_block_stack_index;
        }
        if (!current_value_block_stack_index && !value_block_stack[current_value_block_stack_index]) {
            break;
        }
        DebugValueBlock *block = value_block_stack[current_value_block_stack_index];
        value_block_stack[current_value_block_stack_index] = block->next;
        block->next = debug_state->first_free_value_block;
        debug_state->first_free_value_block = block;
        
        for (DebugValue *value = block->first_value;
            value;
            ) {
            if (value->next) {
                value = value->next;
            } else {
                value->next = debug_state->first_free_value;
                break;
            }
        }
        debug_state->first_free_value = block->first_value;
        value_block_stack[++current_value_block_stack_index] = block->first_child;
    }
    debug_state->global_value_block = 0;
    
    for (;; ++debug_state->collation_array_index) {
        if (debug_state->collation_array_index == DEBUG_MAX_EVENT_ARRAY_COUNT) {
            debug_state->collation_array_index = 0;
        }
        
        u32 event_array_index = debug_state->collation_array_index;
        if (event_array_index == invalid_event_array_index) {
            break;
        }
        
        DebugFrame *collation_frame = debug_state->frames + debug_state->frame_index;
        DebugValueBlock *value_block_stack[DEBUG_VALUE_BLOCK_MAX_DEPTH] = {};
        u32 current_value_block_stack_index = 0;
        value_block_stack[0] = debug_state->global_value_block = get_value_block(debug_state);
        value_block_stack[0]->name = "Values";
        
        for (u32 event_index = 0; event_index < debug_table->event_counts[event_array_index]; ++event_index) {
            DebugEvent *event = debug_table->events[event_array_index] + event_index;
            switch (event->type) {
                case DEBUG_EVENT_FRAME_MARKER: {
                    collation_frame->end_clock = event->clock;
                    debug_state->frame_index = (debug_state->frame_index + 1) % DEBUG_MAX_FRAME_COUNT;
                    
                    collation_frame = debug_state->frames + debug_state->frame_index;
                    memset(collation_frame, 0, sizeof(*collation_frame));
                    collation_frame->begin_clock = event->clock;
                    collation_frame->frame_index = debug_state->total_frame_count;
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
#define DEBUG_EVENT_VALUE_DEF(_type)                            \
case DEBUG_EVENT_VALUE_##_type: {                               \
    DebugValue *value = get_debug_value(debug_state);           \
    value->value_kind = DEBUG_VALUE_##_type;                    \
    value->value_##_type = event->value_##_type;                \
    value->name = event->name;                                  \
    LLIST_ADD(value_block_stack[current_value_block_stack_index]->first_value, value); \
} break;
DEBUG_EVENT_VALUE_DEF(u64)
DEBUG_EVENT_VALUE_DEF(f32)
DEBUG_EVENT_VALUE_DEF(Vec2)
DEBUG_EVENT_VALUE_DEF(Vec3)
DEBUG_EVENT_VALUE_DEF(Vec2i)
                case DEBUG_EVENT_VALUE_SWITCH: {
                    DebugValue *value = get_debug_value(debug_state);                                                
                    value->value_kind = DEBUG_VALUE_SWITCH;                   
                    value->value_switch = event->value_switch;               
                    value->name = event->name;                                 
                    LLIST_ADD(value_block_stack[current_value_block_stack_index]->first_value, value); 
                } break; 
                case DEBUG_EVENT_VALUE_DRAG: {
                    DebugValue *value = get_debug_value(debug_state);                                                
                    value->value_kind = DEBUG_VALUE_DRAG;                   
                    value->value_drag = event->value_drag;               
                    value->name = event->name;                                
                    LLIST_ADD(value_block_stack[current_value_block_stack_index]->first_value, value); 
                } break; 
                case DEBUG_EVENT_BEGIN_VALUE_BLOCK: {
                    DebugValueBlock *block = get_value_block(debug_state);
                    block->name = event->name;
                    block->next = value_block_stack[current_value_block_stack_index]->first_child;
                    value_block_stack[current_value_block_stack_index]->first_child = block;
                    assert(current_value_block_stack_index + 1 < DEBUG_VALUE_BLOCK_MAX_DEPTH);
                    value_block_stack[++current_value_block_stack_index] = block;
                } break;
                case DEBUG_EVENT_END_VALUE_BLOCK: {
                    assert(current_value_block_stack_index);
                    --current_value_block_stack_index;
                } break;
                INVALID_DEFAULT_CASE;
            }
        }
    }
}

static void display_values(DevUILayout *dev_ui, DebugState *debug_state) {
    DebugValueBlock *value_block_stack[DEBUG_VALUE_BLOCK_MAX_DEPTH] = {};
    u32 current_value_block_stack_index = 0;
    value_block_stack[0] = debug_state->global_value_block;
    for (;;) {
        while (!value_block_stack[current_value_block_stack_index] && current_value_block_stack_index) {
            dev_ui_end_section(dev_ui);
            --current_value_block_stack_index;
        }
        if (!current_value_block_stack_index && !value_block_stack[current_value_block_stack_index]) {
            break;
        }
        DebugValueBlock *block = value_block_stack[current_value_block_stack_index];
        value_block_stack[current_value_block_stack_index] = block->next;
        
        if (dev_ui_section(dev_ui, block->name)) {
            LLIST_ITER(block->first_value, value) {
                char buffer[64];
                switch (value->value_kind) {
                    case DEBUG_VALUE_f32: {
                        snprintf(buffer, sizeof(buffer), "%s: %.2f", value->name, value->value_f32);
                    } break;
                    case DEBUG_VALUE_u64: {
                        snprintf(buffer, sizeof(buffer), "%s: %llu", value->name, value->value_u64);
                    } break;
                    case DEBUG_VALUE_Vec2: {
                        snprintf(buffer, sizeof(buffer), "%s: (%.2f %.2f)", value->name, value->value_Vec2.x, value->value_Vec2.y);
                    } break;
                    case DEBUG_VALUE_Vec2i: {
                        snprintf(buffer, sizeof(buffer), "%s: (%d %d)", value->name, value->value_Vec2i.x, value->value_Vec2i.y);
                    } break;
                    case DEBUG_VALUE_Vec3: {
                        snprintf(buffer, sizeof(buffer), "%s: (%.2f %.2f %.2f)",value->name, value->value_Vec3.x, value->value_Vec3.y, value->value_Vec3.z);
                    } break;
                    case DEBUG_VALUE_SWITCH: {
                        snprintf(buffer, sizeof(buffer), "%s: %s", value->name, *value->value_switch ? "true" : "false");       
                    } break;
                    case DEBUG_VALUE_DRAG: {
                        snprintf(buffer, sizeof(buffer), "%s", value->name);       
                    } break;
                }
                
                if (value->value_kind == DEBUG_VALUE_SWITCH) {
                    dev_ui_checkbox(dev_ui, buffer, value->value_switch);
                } else if (value->value_kind == DEBUG_VALUE_DRAG) {
                    dev_ui_drag(dev_ui, buffer, value->value_drag);
                } else {
                    dev_ui_labelf(dev_ui, buffer);
                }
            }
        
            value_block_stack[++current_value_block_stack_index] = block->first_child;
        }
    }
}

void DEBUG_update(DebugState *debug_state, InputManager *input, RendererCommands *commands, Assets *assets) {
    TIMED_FUNCTION();
    DevUILayout dev_ui = dev_ui_begin(&debug_state->dev_ui, input, assets, commands);
    dev_ui_labelf(&dev_ui, "FPS: %.3f; DT: %ums;", 1.0f / input->platform->frame_dt, (u32)(input->platform->frame_dt * 1000));
    display_values(&dev_ui, debug_state);
    if (dev_ui_section(&dev_ui, "Profiler")) {
        DebugFrame *frame = debug_state->frames + (debug_state->frame_index ? debug_state->frame_index - 1: DEBUG_MAX_FRAME_COUNT - 1);
        f32 frame_time = (f32)(frame->end_clock - frame->begin_clock);
        u64 record_count = frame->records_count;
        TempMemory records_sort_temp = begin_temp_memory(&debug_state->arena);
        SortEntry *sort_a = alloc_arr(&debug_state->arena, record_count, SortEntry);
        SortEntry *sort_b = alloc_arr(&debug_state->arena, record_count, SortEntry);
        for (size_t i = 0; i < record_count; ++i) {
            sort_a[i].sort_key = frame->records[i].total_clocks;
            sort_a[i].sort_index = i;
        }
        radix_sort(sort_a, sort_b, record_count);
        dev_ui_labelf(&dev_ui, "Frame %llu", frame->frame_index);    
        dev_ui_checkbox(&dev_ui, "Pause", &debug_state->is_paused);
        dev_ui_begin_sizable(&dev_ui);
        for (size_t i = 0; i < min(frame->records_count, 20); ++i) {
            DebugRecord *record = frame->records + sort_a[record_count - i - 1].sort_index;
            dev_ui_labelf(&dev_ui, "%2llu %32s %8llu %4u %8llu %.2f%%\n", i, record->name, record->total_clocks, 
                record->times_called, record->total_clocks / (u64)record->times_called, ((f32)record->total_clocks / frame_time * 100));
        }
        dev_ui_end_sizable(&dev_ui);
        end_temp_memory(records_sort_temp);
        dev_ui_end_section(&dev_ui);
    }
    
    dev_ui_end(&dev_ui);
}

void DEBUG_begin_frame(DebugState *debug_state) {
}

void DEBUG_frame_end(DebugState *debug_state) {
    TIMED_FUNCTION();
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
    
    if (!debug_state->is_paused) {
        debug_collate_events(debug_state, debug_table->current_event_array_index);
    }
}

DebugState *DEBUG_init() {
#define DEBUG_ARENA_SIZE MEGABYTES(256)
    DebugState *debug_state = bootstrap_alloc_struct(DebugState, arena, DEBUG_ARENA_SIZE);
    debug_table = &debug_state->debug_table;
    return debug_state;
}

#endif 