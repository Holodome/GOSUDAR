#include "lib/memory.hh"

#include "lib/strings.hh"
#include "lib/lists.hh"

#include <string.h> // memset, memcpy, memmove
#include <stdlib.h> // malloc, free

void *mem_alloc(uptr size) {
    void *result = 0;
    result = malloc(size);
    assert(result);
    mem_zero(result, size);
    return result;
}

void mem_free(void *ptr, uptr size) {
    (void)size;
    free(ptr);
}

void *mem_realloc(void *ptr, uptr old_size, uptr size) {
    void *new_ptr = mem_alloc(size);
    mem_copy(new_ptr, ptr, old_size);
    mem_free(ptr, old_size);
    return new_ptr;
}

char *mem_alloc_str(const char *str) {
    uptr len = str_len(str) + 1;
    char *result = (char *)mem_alloc(len);
    mem_copy(result, str, len);
    return result;    
}

void mem_copy(void *dst, const void *src, uptr size) {
    memcpy(dst, src, size);
}

void mem_move(void *dst, const void *src, uptr size) {
    memmove(dst, src, size);
}

void mem_zero(void *dst, uptr size) {
    memset(dst, 0, size);
}

bool mem_eq(const void *a, const void *b, uptr n) {
    return memcmp(a, b, n) == 0;
}
