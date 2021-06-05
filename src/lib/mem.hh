#if !defined(MEM_HH)

#include "general.hh"

#define MEM_CLEAR_TO_ZERO 1

namespace Mem {
    static u64 times_alloced = 0;
    
    void *alloc_base(size_t size) {
        assert(size);
        if (!size) {
            return 0;
        }
        size_t padded_size = (size + 15) & ~15;
        ++times_alloced;
        return ::_aligned_malloc(padded_size, 16);
    }
    
    void *alloc_clear(size_t size) {
        void *result = alloc_base(size);
        memset(result, 0, size);
        return result;
    }
    
    void *alloc(size_t size) {
#if MEM_CLEAR_TO_ZERO
        return alloc_clear(size);
#else
        return alloc(size);
#endif 
    }
    
    void free(void *ptr) {
        // @TODO stb_truetype calls free with null, better investigate it 
        // assert(ptr);
        if (ptr) {
            assert(times_alloced);
            --times_alloced;
            ::_aligned_free(ptr);
        }
    }
    
    void *realloc(void *ptr, size_t new_size) {
        return ::_aligned_realloc(ptr, new_size, 16);
    }
}

void *operator new(size_t size) {
    return Mem::alloc(size);
}
void operator delete(void *p) {
    Mem::free(p);
}
void *operator new[](size_t size) {
    return Mem::alloc(size);
}
void operator delete[](void *p) {
    Mem::free(p);
}

void *malloc(...) {
    assert(!"Direct use of malloc is prohibited");
    return 0;
}

void free(...) {
    assert(!"Direct use of malloc is prohibited");
}

void *realloc(...) {
    assert(!"Direct use of malloc is prohibited");
    return 0;
}

#define MEM_HH 1
#endif
