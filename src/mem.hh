#if !defined(MEM_HH)

#include "general.hh"
#include "lib.hh"

#include "debug.hh"

#define MEM_ALLOC_UNDERFLOW_CHECK 0x1
#define MEM_ALLOC_OVERFLOW_CHECK  0x2

// @NOTE: current memory system allows dynamically created and growing arenas
// Other option is to preallocate memory for whole game and allocate from it 
// selectively using subarens (for example let game use 8G, 4G be assets, 2G game world etc.)
// This is actually way more faster and effective and dynamic arenas
// Dynamic arenas have some problems like memory fragmentation, but allow not to care about arena sizes
// and tweak only default size
// In end of development we may want to switch to preallocating memory for whole game to be more
// effective and fast and let user specify amounth of memory that game can allocate
// @NOTE: Also using individual arenas can help catch memory access bugs, like overflow or underflow
#define MEM_DEFUALT_ARENA_BLOCK_SIZE (1024 * 1024)
#define MEM_DEFAULT_ALIGNMENT (2 * sizeof(uptr))
CT_ASSERT(IS_POW2(MEM_DEFAULT_ALIGNMENT));
#define MEM_DEFAULT_ALLOC_FLAGS 0
// @NOTE since all os allocation functions return memory all set to zero,
// we can try to avoid having need to memset it and only do that in cases 
// where we recycle memory - like in temp memory
// Lets see if this approach works
#define MEM_NO_MEMZERO 1
// @NOTE: DEBUG STUFF
// Flag if all block allocations should have guarded pages around them
#define MEM_DO_BOUNDS_CHECKING 0
#define MEM_BOUNDS_CHECKING_POLICY MEM_ALLOC_OVERFLOW_CHECK
// Flag if all allocations are done in separate blocks each of which is guarded
// This is highly ineffective and it is even not certain that we will be able to do so 
// with a lot of allocations because we could simply run out of memory
// Basically only this option enables us to do full bounds check, because bounds only for blocks
// may not be able to catch a lot of memory access errors within blocks, and even allow overflows
// up to alignment size
// @TODO: there is a way to reliably do hard bounds checking selectively if we pass this down as a 
// flag and don't record hard bounds-checked allocation in debug system
// We may want to switch having this as an option in contrast to global flag if running out of 
// memory becomes a problem
// But really without hard checks bounds checking makes little sence
#define MEM_DO_HARD_BOUNDS_CHECKING_INTERNAL 1
#define MEM_DO_HARD_BOUNDS_CHECKING (MEM_DO_HARD_BOUNDS_CHECKING_INTERNAL && MEM_DO_BOUNDS_CHECKING)
CT_ASSERT((u32)MEM_DO_HARD_BOUNDS_CHECKING <= MEM_DO_BOUNDS_CHECKING);

#if MEM_DO_BOUNDS_CHECKING
// @NOTE: There is no way to do both overflow and underflow check simulateously due
// to page size limitations.
// We don't raise errors in platform layer but do it here
CT_ASSERT((u32)TO_BOOL(MEM_BOUNDS_CHECKING_POLICY & MEM_ALLOC_OVERFLOW_CHECK) + (u32)TO_BOOL(MEM_BOUNDS_CHECKING_POLICY & MEM_ALLOC_UNDERFLOW_CHECK) == 1);
#undef MEM_DEFAULT_ALLOC_FLAGS
#define MEM_DEFAULT_ALLOC_FLAGS MEM_BOUNDS_CHECKING_POLICY
#endif 

#if MEM_DO_HARD_BOUNDS_CHECKING
#undef MEM_DEFAULT_ALIGNMENT
#define MEM_DEFAULT_ALIGNMENT 1
#endif 

// Things used to track memory allocations
// Basically depending on type of build (debug enalbed or disabled)
// different allocation functions are compiled and debug ones can capture source locations
// to get call sites at debug display
#if INTERNAL_BUILD
#define DEBUG_MEM_PARAM_NAME __debug_name
#define DEBUG_MEM_PARAM const char *DEBUG_MEM_PARAM_NAME,
#define DEBUG_MEM_PASS DEBUG_MEM_PARAM_NAME,
#define DEBUG_MEM_LOC DEBUG_NAME(),
#else 
#define DEBUG_MEM_PARAM
#define DEBUG_MEM_PASS
#define DEBUG_MEM_LOC
#endif 

struct MemoryBlock {
    u64 size;
    u64 used;
    u8 *base;
    MemoryBlock *next;
};

// @NOTE: Forward-declare this to OS layer
// Request to os to get memory block. Size of returned memory block can be greater than requested size
// Returned block memory is aligned on MEM_DEFAULT_ALIGNMENT
// If underflow/overflow checking is specified, regions guarding returned blocks are protected
// and access violation error is raised when they are touched.
// Protected region can start at most after MEM_DEFAULT_ALIGNMENT - 1 bytes after block end
// Returned block can be freed with os_free call
MemoryBlock *os_alloc_block(uptr size, u32 flags = MEM_DEFAULT_ALLOC_FLAGS);
void os_free(void *ptr);

struct MemoryArena {
    MemoryBlock *current_block;
    u32 minimum_block_size;
    u32 temp_count;
};

inline uptr get_alignment_offset(MemoryArena *arena, uptr align) {
    assert(is_pow2(align));
    uptr result_ptr = (uptr)arena->current_block->base + arena->current_block->used;
    uptr align_mask = align - 1;
    uptr offset = 0;
    if (result_ptr & align_mask) {
        offset = align - (result_ptr & align_mask);
    }
    return offset;
}

inline uptr get_effective_size(MemoryArena *arena, uptr size_init) {
    return size_init + get_alignment_offset(arena, MEM_DEFAULT_ALIGNMENT);
}

#define alloc_struct(_arena, _type, ...) \
(_type *)alloc_(DEBUG_MEM_LOC _arena, sizeof(_type), ##__VA_ARGS__)
#define alloc_arr(_arena, _count, _type, ...) \
(_type *)alloc_(DEBUG_MEM_LOC _arena, _count * sizeof(_type), ##__VA_ARGS__)
#define alloc_string(_arena, _string, ...) \
(char *)alloc_copy_(DEBUG_MEM_LOC _arena, _string, strlen(_string) + 1, ##__VA_ARGS__)
#define alloc(_arena, _size) \
alloc_(DEBUG_MEM_LOC _arena, _size)
void *alloc_(DEBUG_MEM_PARAM MemoryArena *arena, uptr size_init) {
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
                arena->minimum_block_size = MEM_DEFUALT_ARENA_BLOCK_SIZE;
            }
            
            uptr block_size = size;
#if !MEM_DO_HARD_BOUNDS_CHECKING
            if (arena->minimum_block_size > block_size) {
                block_size = arena->minimum_block_size;
            }
#endif 
            
            MemoryBlock *new_block = os_alloc_block(block_size);
            new_block->next = arena->current_block;
            arena->current_block = new_block;
            DEBUG_ARENA_BLOCK_ALLOCATE(DEBUG_MEM_PARAM_NAME, new_block);
        }
        
        assert(arena->current_block->used + size <= arena->current_block->size);
        
        uptr align_offset = get_alignment_offset(arena, MEM_DEFAULT_ALIGNMENT);
        uptr block_offset = arena->current_block->used + align_offset;
        result = arena->current_block->base + block_offset;
        arena->current_block->used += size;
        assert(block_offset + size <= arena->current_block->size);
        
        assert(size >= size_init);
#if !MEM_NO_MEMZERO
        memset(result, 0, size_init);
#endif 
        DEBUG_ARENA_ALLOCATE(DEBUG_MEM_PARAM_NAME, arena->current_block, size, block_offset);
    }
    return result;
}

// Allocte memory block and write arena in it
#define bootstrap_alloc_struct(_type, _field, ...) \
(_type *)bootstrap_alloc_(DEBUG_MEM_LOC sizeof(_type), STRUCT_OFFSET(_type, _field), __VA_ARGS__)
#define bootstrap_alloc(_size, _arena_offset) \
bootstrap_alloc_(DEBUG_MEM_LOC _size, _arena_offset)
inline void *bootstrap_alloc_(DEBUG_MEM_PARAM uptr size, uptr arena_offset) {
    MemoryArena bootstrap = {};
    void *struct_ptr = alloc_(DEBUG_MEM_PASS &bootstrap, size);
    *(MemoryArena *)((u8 *)struct_ptr + arena_offset) = bootstrap;
    return struct_ptr;
}

inline void free_last_block_(DEBUG_MEM_PARAM MemoryArena *arena) {
    MemoryBlock *block = arena->current_block;
    DEBUG_ARENA_BLOCK_FREE(DEBUG_MEM_PARAM_NAME, block, block->next);
    arena->current_block = block->next;
    os_free(block);
}

#define arena_clear(_arena) arena_clear_(DEBUG_MEM_LOC _arena)
void arena_clear_(DEBUG_MEM_PARAM MemoryArena *arena) {
    while (arena->current_block) {
        // @NOTE: In case arena itself is stored in last block
        b32 is_last_block = (arena->current_block->next == 0);
        free_last_block_(DEBUG_MEM_PASS arena);
        if (is_last_block) {
            break;
        }
    }
}

struct TempMemory {
    MemoryArena *arena;
    MemoryBlock *block;
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

#define end_temp_memory(_mem) end_temp_memory_(DEBUG_MEM_LOC _mem)
inline void end_temp_memory_(DEBUG_MEM_PARAM TempMemory mem) {
    assert(mem.arena->temp_count);
    --mem.arena->temp_count;
    
    while (mem.arena->current_block != mem.block) {
        free_last_block_(DEBUG_MEM_PASS mem.arena);
    }
    
    if (mem.arena->current_block) {
        assert(mem.arena->current_block->used >= mem.block_used);
#if MEM_NO_MEMZERO
        memset(mem.arena->current_block->base + mem.block_used, 0, mem.arena->current_block->used - mem.block_used);
#endif 
        mem.arena->current_block->used = mem.block_used;
        DEBUG_ARENA_BLOCK_TRUNCATE(DEBUG_MEM_PARAM_NAME, mem.arena->current_block);
    }
}

#define MEM_HH
#endif 