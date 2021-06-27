#include "framework/debug.hh"

#include "framework/os.hh"

DebugTable *debug_table;

static DebugFrameRegion *debug_add_region(DebugFrame *collation_frame) {
    assert(collation_frame->region_count < DEBUG_MAX_REGIONS_PER_FRAME);
    DebugFrameRegion *result = collation_frame->regions + collation_frame->region_count++;
    
    return result;
}

static char *debug_name_from(DebugOpenBlock *block) {
    char *record = block ? block->opening_event->debug_name : 0;
    return record;
}

static void debug_restart_collation(DebugState *debug_state, u32 invalid_event_array_index)
{
    temp_memory_end(debug_state->collate_temp);
    debug_state->collate_temp = temp_memory_begin(&debug_state->collate_arena);
    
    debug_state->first_free_block = 0;
    debug_state->frames = alloc_arr(&debug_state->collate_arena, DEBUG_MAX_FRAME_COUNT,
                                    DebugFrame);
    debug_state->frame_count = 0;
    debug_state->current_open_block = 0;
    
    debug_state->collation_array_index = invalid_event_array_index + 1;
    debug_state->collation_frame = 0;
}

static void 
debug_collate_events(DebugState *debug_state, u32 invalid_event_array_index)
{
    for (;
         ;
         ++debug_state->collation_array_index)
    {
        if (debug_state->collation_array_index == DEBUG_MAX_EVENT_ARRAY_COUNT)
        {
            debug_state->collation_array_index = 0;
        }
        
        u32 event_array_index = debug_state->collation_array_index;
        if (event_array_index == invalid_event_array_index)
        {
            break;
        }
        
        for (u32 event_index = 0;
             event_index < debug_table->event_counts[event_array_index];
             ++event_index)
        {
            DebugEvent *event = debug_table->events[event_array_index] + event_index;
            
            if (event->type == DEBUG_EVENT_FRAME_MARKER)
            {
                if (debug_state->collation_frame)
                {
                    debug_state->collation_frame->end_clock = event->clock;
                    ++debug_state->frame_count;
                }
                
                debug_state->collation_frame = debug_state->frames + debug_state->frame_count;
                debug_state->collation_frame->begin_clock = event->clock;
                debug_state->collation_frame->end_clock = 0;
                debug_state->collation_frame->region_count = 0;
                debug_state->collation_frame->regions = alloc_arr(&debug_state->collate_arena, DEBUG_MAX_REGIONS_PER_FRAME, DebugFrameRegion);
            }
            else if (debug_state->collation_frame)
            {
                u32 frame_index = debug_state->frame_count - 1;
                switch (event->type)
                {
                    case DEBUG_EVENT_BEGIN_BLOCK:
                    {
                        DebugOpenBlock *debug_block = debug_state->first_free_block;
                        if (debug_block)
                        {
                            debug_state->first_free_block = debug_block->next_free;
                        }
                        else 
                        {
                            debug_block = alloc_struct(&debug_state->collate_arena, DebugOpenBlock);
                        }
                        
                        debug_block->frame_index = frame_index;
                        debug_block->opening_event = event;
                        debug_block->parent = debug_state->current_open_block;
                        debug_block->next_free = 0;
                        // debug_block->source = 
                        debug_state->current_open_block = debug_block;
                        
                    } break;
                    case DEBUG_EVENT_END_BLOCK:
                    {
                        if (debug_state->current_open_block)
                        {
                            DebugOpenBlock *matching_block = debug_state->current_open_block;
                            DebugEvent *opening_event = matching_block->opening_event;
                            // check if opened event mathces current - at least we can check thread id
                            // if (opening_event->thread_id == event->thread_id)
                            {
                                // if (matching_block->frame_index == frame_index)
                                {
                                    if (debug_name_from(matching_block->parent) == debug_state->debug_name_to_record)
                                    {
                                        DebugFrameRegion *region = debug_add_region(debug_state->collation_frame);
                                        region->time_min = (f32)(opening_event->clock - debug_state->collation_frame->begin_clock);
                                        region->time_max = (f32)(event->clock - debug_state->collation_frame->begin_clock);
                                        // region->record = (opening_event ? opening_event : event) ;
                                        region->debug_name = opening_event->debug_name;
                                        region->name = opening_event->name;
                                    }
                                    
                                    matching_block->next_free = debug_state->first_free_block;
                                    debug_state->first_free_block = matching_block;
                                    debug_state->current_open_block = matching_block->parent;
                                }
                            }
                        }
                    } break;
                    INVALID_DEFAULT_CASE;
                }
            }
            
        }
    }
}

// static void debug_refresh_collation(DebugState *debug_state) {
//     debug_restart_collation(debug_state, debug_table->current_event_array_index);
//     debug_collate_events(debug_state,    debug_table->current_event_array_index);
// }

void debug_frame_end(DebugState *debug_state) {
    ++debug_table->current_event_array_index;
    if (debug_table->current_event_array_index >= DEBUG_MAX_EVENT_ARRAY_COUNT)
    {
        debug_table->current_event_array_index = 0;
    }
    
    u64 event_array_index_event_index = _InterlockedExchange64((volatile i64 *)&debug_table->event_array_index_event_index,
                                                             (i64)debug_table->current_event_array_index << 32);
    
    u32 event_array_index = event_array_index_event_index >> 32;
    u32 event_count       = event_array_index_event_index & UINT32_MAX;
    
    debug_table->event_counts[event_array_index] = event_count;
    
    if (!debug_state->is_paused)
    {
        if (debug_state->frame_count >= DEBUG_MAX_FRAME_COUNT)
        {
            debug_restart_collation(debug_state, debug_table->current_event_array_index);
        }
        debug_collate_events(debug_state, debug_table->current_event_array_index);
    }
}

DebugState *debug_init() {
    DebugState *debug_state = (DebugState *)os_alloc(sizeof(DebugState));
    
    debug_table = &debug_state->debug_table;
    
    size_t debug_collate_arena_size = MEGABYTES(256);
    arena_init(&debug_state->collate_arena, os_alloc(debug_collate_arena_size), debug_collate_arena_size);
    debug_state->collate_temp = temp_memory_begin(&debug_state->collate_arena);
    
    debug_restart_collation(debug_state, debug_table->current_event_array_index);
    
    return debug_state;
}