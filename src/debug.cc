#include "debug.hh"

#if INTERNAL_BUILD

#include "mem.hh"
#include "os.hh"
#include "dev_ui.hh"

struct DebugRecord {
    const char *debug_name;
    const char *name;
    u32 times_called;
    u64 total_clocks;
};  

struct DebugRecordHash {
    u32 debug_name_hash;
    u32 index;
};

struct DebugFrame {
    u64 frame_index;
    u64 begin_clock;
    u64 end_clock;
    
    u32 records_count;
    DebugRecord records         [DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME];
    DebugRecordHash records_hash[DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME];
};

struct DebugOpenBlock {
    u32 frame_index;
    DebugEvent *opening_event;
    DebugOpenBlock *parent;
    DebugOpenBlock *next_free;
};

enum {
    DEBUG_VALUE_NONE,  
    DEBUG_VALUE_SWITCH,  
    DEBUG_VALUE_DRAG,  
#define DEBUG_VALUE_TYPE(_name) DEBUG_VALUE_##_name,
    DEBUG_VALUE_TYPE_LIST()
#undef DEBUG_VALUE_TYPE
};

struct DebugValue {
    const char *name;
    u32 value_kind;
    union {
        bool *value_switch;
        f32 *value_drag;
#define DEBUG_VALUE_TYPE(_name) _name value_##_name;
        DEBUG_VALUE_TYPE_LIST()
#undef DEBUG_VALUE_TYPE
    };
    DebugValue *next;
};  

#define DEBUG_VALUE_BLOCK_MAX_DEPTH 8

struct DebugValueBlock {
    const char *name;
    DebugValue *first_value;
    DebugValueBlock *first_child;
    DebugValueBlock *next;
};

struct DebugArenaAllocation {
    const char *debug_name;
    uptr block_offset;
    uptr size;
    
    DebugArenaAllocation *next;
};

struct DebugArenaBlock {
    uptr mem_address;
    uptr size;
    
    DebugArenaAllocation *first_allocation;
    DebugArenaAllocation *last_allocation;
    DebugArenaBlock *next;
};

struct DebugArena {
    const char *name;
    DebugArenaBlock *first_block;
    
    DebugArena *next;
    b32 is_supressed;
};

struct DebugState {
    MemoryArena arena;
    
    DebugArena *first_arena;
    DebugArena *first_free_arena;
    DebugArenaBlock *first_free_arena_block;
    DebugArenaAllocation *first_free_arena_allocation;
    u32 arenas_allocted;
    u32 arena_blocks_allocated;
    u32 arena_allocations_allocated;
    
    u32 frame_index;
    DebugFrame frames[DEBUG_MAX_FRAME_COUNT];
    u64 debug_open_blocks_allocated;
    DebugOpenBlock *first_free_block;
    DebugOpenBlock *current_open_block;
    u32 collation_array_index;
    bool is_paused;
    
    u64 debug_values_allocated;
    DebugValue *first_free_value;
    u64 value_blocks_allocated;
    DebugValueBlock *first_free_value_block;
    DebugValueBlock *global_value_block;
    
    u64 total_frame_count;
    bool memory_fragmentation_detailed;
    
    DevUI dev_ui;
};

struct DebugMemoryArenaBlockInfo {
    u32 idx;
    u32 allocation_count;
    u32 size;
    u32 used;
    DebugMemoryArenaBlockInfo *next;
};

struct DebugMemoryArenaInfo {
    DebugArena *arena;
    const char *name;
    u32 blocks_count;
    u32 total_size;
    u32 allocation_count;
    
    DebugMemoryArenaBlockInfo *first_block;
};

struct DebugMemoryCallsiteInfo {
    DebugArena *arena;
    const char *debug_name;
    u32 allocation_count;
    u64 total_allocated;
    
    DebugMemoryCallsiteInfo *next;
};

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

static DebugArena *get_arena_by_lookup_block(DebugState *debug_state, MemoryBlock *block, b32 allow_creation = false) {
    DebugArena *result = 0;
    uptr lookup_address = UPTR_FROM_PTR(block);
    
    LLIST_ITER(test, debug_state->first_arena) {
        assert(test->first_block);
        if (test->first_block->mem_address == lookup_address) {
            result = test;
            break;
        }
    }
    
    if (!result) {
        assert(allow_creation);
        result = debug_state->first_free_arena;
        if (!result) {
            ++debug_state->arenas_allocted;
            result = alloc_struct(&debug_state->arena, DebugArena);
        } else {
            LLIST_POP(debug_state->first_free_arena);
        }
        
        result->name = "(unnamed)";
        result->first_block = 0;
        LLIST_ADD(debug_state->first_arena, result);
    }
    
    return result;
}

static void debug_arena_set_name(DebugState *debug_state, DebugEvent *event) {
    MemoryBlock *lookup_block = event->mem_op.block;
    if (lookup_block) {
        DebugArena *arena = get_arena_by_lookup_block(debug_state, lookup_block);
        if (arena) {
            arena->name = event->name;
        }
    }
}

static void debug_arena_allocate(DebugState *debug_state, DebugEvent *event) {
    DebugMemoryOP *op = &event->mem_op;
    DebugArena *arena = get_arena_by_lookup_block(debug_state, op->block);
    if (!arena->is_supressed) {
        DebugArenaBlock *block = arena->first_block;
        assert(block->mem_address == UPTR_FROM_PTR(op->block));
        
        DebugArenaAllocation *allocation = debug_state->first_free_arena_allocation;
        if (!allocation) {
            ++debug_state->arena_allocations_allocated;
            allocation = alloc_struct(&debug_state->arena, DebugArenaAllocation);
        } else {
            LLIST_POP(debug_state->first_free_arena_allocation);
        }
        
        allocation->debug_name = event->debug_name;
        allocation->block_offset = op->offset_in_block;
        allocation->size = op->allocated_size;
        
        LLIST_ADD(block->first_allocation, allocation);
        if (!block->last_allocation) {
            block->last_allocation = allocation;
        }
    }
}

static void debug_arena_block_allocate(DebugState *debug_state, DebugEvent *event) {
    DebugMemoryOP *op = &event->mem_op;
    DebugArena *arena = get_arena_by_lookup_block(debug_state, op->arena_lookup_block, true);
    if (!arena->is_supressed
#if !MEM_DO_HARD_BOUNDS_CHECKING
        || true
#endif
        ) {
        DebugArenaBlock *block = debug_state->first_free_arena_block;
        if (!block) {
            ++debug_state->arena_blocks_allocated;
            block = alloc_struct(&debug_state->arena, DebugArenaBlock);
        } else {
            LLIST_POP(debug_state->first_free_arena_block);
        }
        
        block->first_allocation = 0;
        block->last_allocation = 0;
        block->mem_address = UPTR_FROM_PTR(op->block);
        block->size = op->allocated_size;
        
        LLIST_ADD(arena->first_block, block);
    } else {
        arena->first_block->mem_address = UPTR_FROM_PTR(op->block);
    }
}

static void free_allocations(DebugState *debug_state, DebugArenaAllocation *first, DebugArenaAllocation *last) {
    if (first) {
        assert(last);
        last->next = debug_state->first_free_arena_allocation;
        debug_state->first_free_arena_allocation = first;
    } else {
        assert(!last);
    }
}

static void remove_arena(DebugState *debug_state, DebugArena *arena) {
    LLIST_REMOVE(debug_state->first_arena, arena);
    LLIST_ADD(debug_state->first_free_arena, arena);
}

static void debug_arena_block_truncate(DebugState *debug_state, DebugEvent *event) {
    DebugMemoryOP *op = &event->mem_op;
    DebugArena *arena = get_arena_by_lookup_block(debug_state, op->block);
    DebugArenaBlock *block = arena->first_block;
    assert(block->mem_address == UPTR_FROM_PTR(op->block));
    
    DebugArenaAllocation *last_free = 0;
    DebugArenaAllocation *first_valid = block->first_allocation;
    while (first_valid) {
        if (first_valid->block_offset < op->allocated_size) {
            break;
        }
        
        last_free = first_valid;
        first_valid = first_valid->next;
    }
    
    if (block->first_allocation != first_valid) {
        free_allocations(debug_state, block->first_allocation, last_free);
        block->first_allocation = first_valid;
        if (block->last_allocation == last_free) {
            block->last_allocation = 0;
        }
    }
}

static void debug_arena_block_free(DebugState *debug_state, DebugEvent *event) {
    DebugMemoryOP *op = &event->mem_op;
    DebugArena *arena = get_arena_by_lookup_block(debug_state, op->arena_lookup_block);
    DebugArenaBlock *free_block = arena->first_block;
    assert(free_block->mem_address == UPTR_FROM_PTR(op->arena_lookup_block));
    
    free_allocations(debug_state, free_block->first_allocation, free_block->last_allocation);
    
    if (!arena->is_supressed
#if !MEM_DO_HARD_BOUNDS_CHECKING
        || true
#endif
        ) {
        LLIST_POP(arena->first_block);
        LLIST_ADD(debug_state->first_free_arena_block, free_block);
        if (arena->first_block == 0) {
            remove_arena(debug_state, arena);
        }
    } else {
        arena->first_block->mem_address = UPTR_FROM_PTR(op->block);
    }
}

static void supress_arena(DebugState *debug_state, DebugEvent *event) {
    DebugArena *arena = get_arena_by_lookup_block(debug_state, event->mem_op.block);
    arena->is_supressed = true;
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
        
        for (u32 event_index = 0;
             event_index < debug_table->event_counts[event_array_index];
             ++event_index) {
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
                    u32 hash_value = (u32)(uptr)opening_event->debug_name;
#else 
                    u32 hash_value = crc32_cstr(opening_event->debug_name);
#endif 
                    u32 hash_mask = (DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME - 1);
                    u32 hash_slot = hash_value & hash_mask;
                    for (uptr offset = 0; offset < DEBUG_MAX_UNIQUE_REGIONS_PER_FRAME; ++offset) {
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
#define DEBUG_VALUE_TYPE DEBUG_EVENT_VALUE_DEF
                DEBUG_VALUE_TYPE_LIST()
#undef DEBUG_VALUE_TYPE
#undef DEBUG_EVENT_VALUE_DEF
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
                case DEBUG_EVENT_ARENA_NAME: {
                    debug_arena_set_name(debug_state, event);
                } break;
                case DEBUG_EVENT_ARENA_BLOCK_ALLOCATE: {
                    debug_arena_block_allocate(debug_state, event);
                } break;
                case DEBUG_EVENT_ARENA_BLOCK_TRUNCATE: {
                    debug_arena_block_truncate(debug_state, event);
                } break;
                case DEBUG_EVENT_ARENA_BLOCK_FREE: {
                    debug_arena_block_free(debug_state, event);
                } break;
                case DEBUG_EVENT_ARENA_ALLOCATE: {
                    debug_arena_allocate(debug_state, event);
                } break;
                case DEBUG_EVENT_ARENA_SUPRESS: {
                    supress_arena(debug_state, event);
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
            LLIST_ITER(value, block->first_value) {
                char buffer[64];
                switch (value->value_kind) {
                    case DEBUG_VALUE_f32: {
                        snprintf(buffer, sizeof(buffer), "%s: %.2f", value->name, value->value_f32);
                    } break;
                    case DEBUG_VALUE_u64: {
                        snprintf(buffer, sizeof(buffer), "%s: %llu", value->name, value->value_u64);
                    } break;
                    case DEBUG_VALUE_vec2: {
                        snprintf(buffer, sizeof(buffer), "%s: (%.2f %.2f)", value->name, value->value_vec2.x, value->value_vec2.y);
                    } break;
                    case DEBUG_VALUE_vec3: {
                        snprintf(buffer, sizeof(buffer), "%s: (%.2f %.2f %.2f)",value->name, value->value_vec3.x, value->value_vec3.y, value->value_vec3.z);
                    } break;
                    case DEBUG_VALUE_SWITCH: {
                        snprintf(buffer, sizeof(buffer), "%s: %s", value->name, *value->value_switch ? "true" : "false");       
                    } break;
                    case DEBUG_VALUE_DRAG: {
                        snprintf(buffer, sizeof(buffer), "%s", value->name);       
                    } break;
                    case DEBUG_VALUE_bool: {
                        snprintf(buffer, sizeof(buffer), "%s: %s", value->name, value->value_bool ? "true" : "false");
                    } break;
                    case DEBUG_VALUE_i32: {
                        snprintf(buffer, sizeof(buffer), "%s: %d", value->name, value->value_i32);
                    } break;
                    case DEBUG_VALUE_u32: {
                        snprintf(buffer, sizeof(buffer), "%s: %u", value->name, value->value_i32);
                    } break;
                    INVALID_DEFAULT_CASE;
                }
                
                if (value->value_kind == DEBUG_VALUE_SWITCH) {
                    // @TODO THIS IS WRONG!! id is not generated correctly
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

static void display_profiler(DevUILayout *dev_ui, DebugState *debug_state) {
    DebugFrame *frame = debug_state->frames + (debug_state->frame_index ? debug_state->frame_index - 1: DEBUG_MAX_FRAME_COUNT - 1);
    f32 frame_time = (f32)(frame->end_clock - frame->begin_clock);
    u64 record_count = frame->records_count;
    TempMemory records_sort_temp = begin_temp_memory(&debug_state->arena);
    SortEntry *sort_a = alloc_arr(&debug_state->arena, record_count, SortEntry);
    SortEntry *sort_b = alloc_arr(&debug_state->arena, record_count, SortEntry);
    for (uptr i = 0; i < record_count; ++i) {
        sort_a[i].sort_key = frame->records[i].total_clocks;
        sort_a[i].sort_index = i;
    }
    radix_sort(sort_a, sort_b, record_count);
    dev_ui_labelf(dev_ui, "Frame %llu", frame->frame_index);    
    dev_ui_checkbox(dev_ui, "Pause", &debug_state->is_paused);
    dev_ui_begin_sizeable(dev_ui, "ProfilerDisp");
    for (uptr i = 0; i < Mini(frame->records_count, 20); ++i) {
        DebugRecord *record = frame->records + sort_a[record_count - i - 1].sort_index;
        dev_ui_labelf(dev_ui, "%2llu %32s %8llu %4u %8llu %.2f%%\n", i, record->name, record->total_clocks, 
                      record->times_called, record->total_clocks / (u64)record->times_called, ((f32)record->total_clocks / frame_time * 100));
    }
    dev_ui_end_sizeable(dev_ui);
    end_temp_memory(records_sort_temp);
}

static void display_memory(DevUILayout *dev_ui, DebugState *debug_state) {
#define DEBUG_MEMORY_CALLSITES_HASH_SIZE 128
    CT_ASSERT(IS_POW2(DEBUG_MEMORY_CALLSITES_HASH_SIZE));
    
    TempMemory temp = begin_temp_memory(&debug_state->arena);
    u32 arena_count = 0;
    LLIST_ITER(arena, debug_state->first_arena) {
        ++arena_count;
    }
    
    DebugMemoryCallsiteInfo **callsite_hash = alloc_arr(temp.arena, DEBUG_MEMORY_CALLSITES_HASH_SIZE, DebugMemoryCallsiteInfo *);
    DebugMemoryArenaInfo *arena_infos = alloc_arr(temp.arena, arena_count, DebugMemoryArenaInfo);
    
    u32 total_game_memory = sizeof(DebugTable);
    u32 arena_cursor = 0;
    LLIST_ITER(arena, debug_state->first_arena) {
        DebugMemoryArenaInfo *arena_info = arena_infos + arena_cursor++;
        
        u32 arena_blocks_count = 0;
        u32 arena_total_size = 0;
        u32 arena_allocation_count = 0;
        LLIST_ITER(block, arena->first_block) {
            ++arena_blocks_count;
            arena_total_size += block->size;
            
            u32 block_used = 0;
            u32 block_allocation_count = 0;
            LLIST_ITER(allocation, block->first_allocation) {
                block_used += allocation->size;
                ++block_allocation_count;
                
                u32 hash_mask = DEBUG_MEMORY_CALLSITES_HASH_SIZE - 1;
                u32 hash_slot_idx = ((u32)(uptr)allocation->debug_name & hash_mask);
                DebugMemoryCallsiteInfo *site = 0;
                LLIST_ITER(test, callsite_hash[hash_slot_idx]) {
                    if (test->arena == arena && 
                        test->debug_name == allocation->debug_name) {
                        site = test;
                        break;
                    }
                }
                
                if (!site) {
                    site = alloc_struct(temp.arena, DebugMemoryCallsiteInfo);
                    site->arena = arena;
                    site->debug_name = allocation->debug_name;
                    LLIST_ADD(callsite_hash[hash_slot_idx], site);
                }
                
                site->allocation_count += 1;
                site->total_allocated += allocation->size;
            }
            arena_allocation_count += block_allocation_count;
            
            DebugMemoryArenaBlockInfo *block_info = alloc_struct(temp.arena, DebugMemoryArenaBlockInfo);
            block_info->idx = arena_blocks_count - 1;
            block_info->allocation_count = block_allocation_count;
            block_info->size = block->size;
            block_info->used = block_used;
            LLIST_ADD(arena_info->first_block, block_info);
        } 
        total_game_memory += arena_total_size;
        
        arena_info->arena = arena;
        arena_info->name = arena->name;
        arena_info->blocks_count = arena_blocks_count;
        arena_info->total_size = arena_total_size;
        arena_info->allocation_count = arena_allocation_count;
    }
    
    if (dev_ui_section(dev_ui, "Overview")) {
        dev_ui_begin_sizeable(dev_ui, "OverviewDisp");
        for (u32 i = 0; i < arena_count; ++i) {
            DebugMemoryArenaInfo *arena_info = arena_infos + i;
            dev_ui_labelf(dev_ui, "%-15s %4umb(%5.2f%%) %4ub %4ua", 
                          arena_info->name,
                          arena_info->total_size >> 20,
                          (f32)arena_info->total_size / total_game_memory * 100,
                          arena_info->blocks_count,
                          arena_info->allocation_count);
        }
        dev_ui_labelf(dev_ui, "Debug table size: %llumb(%.2f%%)", 
                      sizeof(DebugTable) >> 20,
                      (f32)sizeof(DebugTable) / total_game_memory * 100);
        dev_ui_labelf(dev_ui, "Total: %llumb", total_game_memory >> 20);
        dev_ui_end_sizeable(dev_ui);
        dev_ui_end_section(dev_ui);
    }
    if (dev_ui_section(dev_ui, "Call sites")) {
        dev_ui_begin_sizeable(dev_ui, "CallsiteDisp");
        for (u32 i = 0; i < arena_count; ++i) {
            DebugMemoryArenaInfo *arena_info = arena_infos + i;
            dev_ui_labelf(dev_ui, "%15s", arena_info->name);
            for (u32 hash_slot_idx = 0;
                 hash_slot_idx < DEBUG_MEMORY_CALLSITES_HASH_SIZE;
                 ++hash_slot_idx) {
                LLIST_ITER(callsite, callsite_hash[hash_slot_idx]) {
                    if (callsite->arena == arena_info->arena) {
                        dev_ui_labelf(dev_ui, " %4umb(%5.2f%%) %4ua %-s",
                                      callsite->total_allocated >> 20,
                                      (f32)callsite->total_allocated / total_game_memory * 100,
                                      callsite->allocation_count,
                                      callsite->debug_name);
                    }
                }
            }
        }
        dev_ui_end_sizeable(dev_ui);
        dev_ui_end_section(dev_ui);
    }
    if (dev_ui_section(dev_ui, "Fragmentation")) {
        dev_ui_checkbox(dev_ui, "Detailed", &debug_state->memory_fragmentation_detailed);
        dev_ui_begin_sizeable(dev_ui, "FragmentationDisp");
        for (u32 i = 0; i < arena_count; ++i) {
            DebugMemoryArenaInfo *arena_info = arena_infos + i;
            u32 arena_total_used = 0;
            u32 arena_commited_used = 0;
            u32 arena_commited_size = 0;
            LLIST_ITER(block, arena_info->first_block) {
                arena_total_used += block->used;
                if (block->next) {
                    arena_commited_used += block->used;
                    arena_commited_size += block->size;
                }
            }
            dev_ui_labelf(dev_ui, "%15s %4u/%-4umb(%5.2f%% total %5.2f%% commited)", 
                          arena_info->name,
                          arena_total_used >> 20, 
                          arena_info->total_size >> 20,
                          (f32)arena_total_used / arena_info->total_size * 100,
                          SAFE_RATIO((f32)arena_commited_used, arena_commited_size) * 100);
            if (debug_state->memory_fragmentation_detailed) {
                LLIST_ITER(block, arena_info->first_block) {
                    dev_ui_labelf(dev_ui, "  %2u %8u/%-8ukb(%5.2f%%) %4ua",
                                  block->idx,
                                  block->used >> 10, block->size >> 10,
                                  (f32)block->used / block->size * 100,
                                  block->allocation_count);
                }
            }
        }
        dev_ui_end_sizeable(dev_ui);
        dev_ui_end_section(dev_ui);
    }
    
    end_temp_memory(temp);
}

void DEBUG_update(DebugState *debug_state, GameLinks links) {
    TIMED_FUNCTION();
    InputManager *input = links.input;
    Assets *assets = links.assets;
    RendererCommands *commands = links.commands;
    DevUILayout dev_ui = dev_ui_begin(&debug_state->dev_ui, input, assets, commands);
    dev_ui_labelf(&dev_ui, "FPS: %.3f; DT: %ums;", 1.0f / input->platform->frame_dt, (u32)(input->platform->frame_dt * 1000));
    display_values(&dev_ui, debug_state);
    if (dev_ui_section(&dev_ui, "Profiler")) {
        display_profiler(&dev_ui, debug_state);
        dev_ui_end_section(&dev_ui);
    }
    if (dev_ui_section(&dev_ui, "Memory")) {
        display_memory(&dev_ui, debug_state);
        dev_ui_end_section(&dev_ui);
    }
    
    dev_ui_end(&dev_ui);
}

void DEBUG_begin_frame(DebugState *debug_state) {
}

void DEBUG_frame_end(DebugState *debug_state) {
    TIMED_FUNCTION();
    DEBUG_BEGIN_VALUE_BLOCK("DEBUG");
    DEBUG_VALUE(debug_state->arenas_allocted, "Arenas allocated");
    DEBUG_VALUE(debug_state->arena_blocks_allocated, "Arena blocks allocated");
    DEBUG_VALUE(debug_state->arena_allocations_allocated, "Arena allocations allocated");
    f32 cur_ec = (debug_table->event_counts[debug_table->current_event_array_index]);
    DEBUG_VALUE(cur_ec * 100 / DEBUG_MAX_EVENT_COUNT, "Event array fill");
    DEBUG_END_VALUE_BLOCK();
    
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
    
    //if (!debug_state->is_paused) {
    debug_collate_events(debug_state, debug_table->current_event_array_index);
    //}
}

DebugState *DEBUG_init() {
    debug_table = (DebugTable *)os_alloc(sizeof(DebugTable));
    
    DebugState *debug_state = bootstrap_alloc_struct(DebugState, arena);
    DEBUG_ARENA_SUPRESS(&debug_state->arena);
    DEBUG_ARENA_NAME(&debug_state->arena, "Debug");
    return debug_state;
}

#endif 