#if !defined(MEM_HH)

#include "general.hh"

namespace Mem {
    void *alloc(size_t size) {
        return malloc(size);
    }
    
    void free(void *ptr) {
       ::free(ptr);
    }
}

#define MEM_HH 1
#endif
