#if !defined(MEM_HH)

#include "general.hh"
#include "lib.hh"

#include "debug.hh"

#define DEFUALT_ARENA_BLOCK_SIZE (1024 * 1024)
#define DEFAULT_ALIGNMENT (2 * sizeof(uptr))

struct OSMemoryBlock {
    u64 size;
    u64 used;
    u8 *base;
    OSMemoryBlock *next;
};

OSMemoryBlock *os_alloc_block(uptr size);
void os_free(void *ptr);

struct MemoryArena {
    OSMemoryBlock *current_block;
    u32 minimum_block_size;
    u32 temp_count;
};

inline uptr get_alignment_offset(MemoryArena *arena, uptr align) {
    assert(is_power_of_two(align));
    uptr result_ptr = (uptr)arena->current_block->base + arena->current_block->used;
    uptr align_mask = align - 1;
    uptr offset = 0;
    if (result_ptr & align_mask) {
        offset = align - (result_ptr & align_mask);
    }
    return offset;
}

inline uptr get_effective_size(MemoryArena *arena, uptr size_init) {
    return size_init + get_alignment_offset(arena, DEFAULT_ALIGNMENT);
}

#if INTERNAL_BUILD
#define DEBUG_MEM_PARAM const char *__debug_name,
#define DEBUG_MEM_PASS __debug_name,
#define DEBUG_MEM_LOC DEBUG_NAME(),
#endif 

#define alloc_struct(_arena, _type, ...) (_type *)alloc_(DEBUG_MEM_LOC _arena, sizeof(_type), ##__VA_ARGS__)
#define alloc_arr(_arena, _count, _type, ...) (_type *)alloc_(DEBUG_MEM_LOC _arena, _count * sizeof(_type), ##__VA_ARGS__)
#define alloc_string(_arena, _string, ...) (char *)alloc_copy_(DEBUG_MEM_LOC _arena, _string, strlen(_string) + 1, ##__VA_ARGS__)
#define alloc(_arena, _size) alloc_(DEBUG_MEM_LOC _arena, _size)
void *alloc_(DEBUG_MEM_PARAM
             MemoryArena *arena, uptr size_init) {
    void *result = 0;
    
    if (size_init) {
        uptr size = 0;
        if (arena->current_block) {
            size = get_effective_size(arena, size_init);
        }
        
        if (!arena->current_block ||
            (arena->current_block->used + size > arena->current_block->size)) {
            size = size_init;
            if (!arena->minimum_block_size) {
                arena->minimum_block_size = DEFUALT_ARENA_BLOCK_SIZE;
            }
            
            uptr block_size = arena->minimum_block_size;
            if (size > block_size) {
                block_size = size;
            }
            
            OSMemoryBlock *new_block = os_alloc_block(block_size);
            new_block->next = arena->current_block;
            arena->current_block = new_block;
            DEBUG_ARENA_BLOCK_ALLOCATE(new_block);
        }
        
        assert(arena->current_block->used + size <= arena->current_block->size);
        
        uptr align_offset = get_alignment_offset(arena, 16);
        uptr block_offset = arena->current_block->used + align_offset;
        result = arena->current_block->base + block_offset;
        arena->current_block->used += size;
        
        assert(size >= size_init);
        
        memset(result, 0, size_init);
        
        DEBUG_ARENA_ALLOCATE(__debug_name, arena->current_block, size, block_offset);
    }
    return result;
}

void arena_init(MemoryArena *arena, uptr minimum_block_size = 0) {
    arena->minimum_block_size = minimum_block_size;
    
}

#define bootstrap_alloc_struct(_type, _field, ...) (_type *)bootstrap_alloc_size(sizeof(_type), STRUCT_OFFSET(_type, _field), __VA_ARGS__)
inline void *bootstrap_alloc_size(uptr size, uptr arena_offset) {
    MemoryArena bootstrap = {};
    void *struct_ptr = alloc(&bootstrap, size);
    *(MemoryArena *)((u8 *)struct_ptr + arena_offset) = bootstrap;
    return struct_ptr;
}

inline void free_last_block(MemoryArena *arena) {
    OSMemoryBlock *block = arena->current_block;
    DEBUG_ARENA_BLOCK_FREE(block);
    arena->current_block = block->next;
    os_free(block);
}

void arena_clear(MemoryArena *arena) {
    while (arena->current_block) {
        // In case arena itself is stored in last block
        b32 is_last_block = (arena->current_block->next == 0);
        free_last_block(arena);
        if (is_last_block) {
            break;
        }
    }
}

struct TempMemory {
    MemoryArena *arena;
    OSMemoryBlock *block;
    u64 block_used;
};


inline TempMemory begin_temp_memory(MemoryArena *arena) {
    ++arena->temp_count;
    TempMemory result;
    result.arena = arena;
    result.block = arena->current_block;
    result.block_used = arena->current_block->used;
    return result;
}

inline void end_temp_memory(TempMemory mem) {
    assert(mem.arena->temp_count);
    --mem.arena->temp_count;
    
    while (mem.arena->current_block != mem.block) {
        free_last_block(mem.arena);
    }
    
    if (mem.arena->current_block) {
        assert(mem.arena->current_block->used >= mem.block_used);
        mem.arena->current_block->used = mem.block_used;
        DEBUG_ARENA_BLOCK_TRUNCATE(mem.arena->current_block);
    }
}

#define MEM_HH
#endif 