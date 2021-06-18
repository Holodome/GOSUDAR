#if !defined(MEMORY_ARENA_HH)

#include "general.hh"

#define DEFAULT_ALIGNMENT (2 * sizeof(void *))

inline uintptr_t align_forward(uintptr_t ptr, size_t align) {
    assert(!(align & (align - 1)));
    
    uintptr_t p = ptr;
    uintptr_t a = align;
    uintptr_t modulo = p & (a - 1);
    
    if (modulo) {
        p += a - modulo;
    }
    return p;
}

struct MemoryArena {
    u8 *data;
    size_t last_data_size;
    size_t data_size;
    size_t data_capacity;
    
    size_t peak_size;
    u32 temp_count;
    
    void init(void *buffer, size_t buffer_size) {
        memset(this, 0, sizeof(*this));
        this->data = (u8 *)buffer;
        this->data_capacity = buffer_size;
    }
    
    void *alloc(size_t size, size_t align = DEFAULT_ALIGNMENT) {
        void *result = 0;
    
        if (size) {
            uintptr_t curr_ptr = (uintptr_t)data + data_size;
            uintptr_t offset = align_forward(curr_ptr, align) - (uintptr_t)data;
            
            if (offset + size < data_capacity) {
                u8 *ptr = data + offset;
                last_data_size = offset;
                data_size = offset + size;
                
                result = ptr;
            } else {
                assert(!"Memory is out of bounds");
            }
            
            if (result) {
                memset(result, 0, size);
            }
        }
        
        if (data_size > peak_size) {
            peak_size = data_size;
        }
        return result;
    }
    
    void *realloc(void *old_mem, size_t old_size, size_t new_size, size_t align = DEFAULT_ALIGNMENT) {
        void *result = 0;
    
        u8 *old_mem_i = (u8 *)old_mem;
        
        if (!old_mem_i || !old_size) {
            result = alloc(new_size, align);
        } else if (data <= old_mem_i && old_mem_i < data + data_capacity) {
            if (data + last_data_size == old_mem_i) {
                data_size = last_data_size + new_size;
                if (new_size > old_size) {
                    memset(data + data_size, 0, new_size - old_size);
                }
                result = old_mem_i;
            } else {
                result = alloc(new_size, align);
                u64 copy_size = new_size;
                if (old_size < new_size) {
                    copy_size = old_size;
                }
                
                memmove(result, old_mem_i, copy_size);
            }
        } else {
            assert(!"Memory is out of bounds");
        }
    
        return result;
    }
    
    void *copy(void *src, size_t size) {
        void *result = alloc(size);
        memcpy(result, src, size);
        return result;
    }
    
    void clear() {
        data_size = 0;
        last_data_size = 0;
    }
};

typedef struct {
    MemoryArena *arena;
    u64 data_size;
    u64 last_data_size;
} TempMemory;

inline TempMemory 
temp_memory_begin(MemoryArena *arena) {
    ++arena->temp_count;
    TempMemory result;
    result.arena = arena;
    result.data_size = arena->data_size;
    result.last_data_size = arena->last_data_size;
    return result;
}

inline void
temp_memory_end(TempMemory mem) {
    assert(mem.arena->temp_count);
    --mem.arena->temp_count;
    mem.arena->data_size = mem.data_size;
    mem.arena->last_data_size = mem.last_data_size;
}

#define MEMORY_ARENA_HH 1
#endif
