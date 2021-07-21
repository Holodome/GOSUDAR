#if !defined(MEM_HH)

#include "general.hh"
#include "lib.hh"

#define DEFUALT_ARENA_BLOCK_SIZE MEGABYTES(1)
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

void DEBUG_memzero16(void *ptr_init, uptr size) {
    __m128 *data = (__m128 *)ptr_init;
    __m128 zero = _mm_setzero_ps();
    for (uptr i = 0; i < size / sizeof(__m128); ++i) {
        *data++ = zero;
    }
}

#define alloc_struct(_arena, _type, ...) (_type *)alloc(_arena, sizeof(_type), ##__VA_ARGS__)
#define alloc_arr(_arena, _count, _type, ...) (_type *)alloc(_arena, _count * sizeof(_type), ##__VA_ARGS__)
#define alloc_string(_arena, _string, ...) (const char *)alloc_copy(_arena, _string, strlen(_string) + 1, ##__VA_ARGS__)
void *alloc(MemoryArena *arena, uptr size_init) {
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
        }
        
        assert(arena->current_block->used + size <= arena->current_block->size);
        
        uptr align_offset = get_alignment_offset(arena, 16);
        result = arena->current_block->base + arena->current_block->used + align_offset;
        arena->current_block->used += size;
        
        assert(size >= size_init);
#if 1
        //memset(result, 0, size_init);
#else 
        DEBUG_memzero16(result, size_init);
#endif
    }
    return result;
}

void arena_init(MemoryArena *arena, uptr minimum_block_size = 0) {
    arena->minimum_block_size = minimum_block_size;
    
}
void arena_clear(MemoryArena *arena);

#define bootstrap_alloc_struct(_type, _field, ...) (_type *)bootstrap_alloc_size(sizeof(_type), STRUCT_OFFSET(_type, _field), __VA_ARGS__)
inline void *bootstrap_alloc_size(uptr size, uptr arena_offset, uptr minimal_block_size = MEGABYTES(4)) {
    MemoryArena bootstrap = {};
    arena_init(&bootstrap, minimal_block_size);
    void *struct_ptr = alloc(&bootstrap, size);
    *(MemoryArena *)((u8 *)struct_ptr + arena_offset) = bootstrap;
    return struct_ptr;
}

void arena_clear(MemoryArena *arena) {
    while (arena->current_block) {
        OSMemoryBlock *block = arena->current_block->next;
        os_free(arena->current_block);
        arena->current_block = block;
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
        OSMemoryBlock *block = mem.arena->current_block->next;
        os_free(mem.arena->current_block);
        mem.arena->current_block = block;
    }
}

#define MEM_HH
#endif 