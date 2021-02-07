#if !defined(MEM_HH)

#include "general.hh"

namespace Mem {
    void *alloc(size_t size) {
        if (!size) {
            return 0;
        }
        size_t padded_size = (size + 15) & ~15;
        return _aligned_malloc(padded_size, 16);
    }
    
    void *alloc_clear(size_t size) {
        void *result = alloc(size);
        memset(result, 0, size);
        return result;
    }
    
    void free(void *ptr) {
       ::_aligned_free(ptr);
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

#define MEM_HH 1
#endif
