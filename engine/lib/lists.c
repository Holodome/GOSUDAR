#include "lib/lists.h"

#define DA_DEFUALT_SIZE 10

void *
da_reserve_(u32 stride, u32 count) {
    u64 initial_size = sizeof(DArray_Header) + count * stride;
    DArray_Header *header = mem_alloc(initial_size);
    header->capacity = count;
    void *result = header + 1;
    return result;
}
void *
da_grow(void *a, u32 stride) {
    void *result = 0;
    if (a) {
        DArray_Header *header = da_header(a);
        u64 old_size = sizeof(*header) + header->capacity * stride;
        u64 new_size = sizeof(*header) + header->capacity * stride * 2;
        header = mem_realloc(header, old_size, new_size);
        header->capacity *= 2;
        result = header + 1;
    } else {
        result = da_reserve_(stride, DA_DEFUALT_SIZE);
    }
    return result;
}

void 
da_free_(void *a, u32 stride) {
    DArray_Header *header = da_header(a);
    u64 old_size = sizeof(*header) + header->capacity * stride;
    mem_free(header, old_size);    
}