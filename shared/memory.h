// Author: Holodome
// Date: 21.08.2021 
// File: pkby/src/platform/memory.h
// Version: 0
//
// Defines memory-related functions, as well as block and arena allocator.
// @NOTE All allocation functions set allocated memory to zero
#pragma once 
#include "general.h"
#include "utils.h"

// standard library-like functions
// Unlike malloc, mem_alloc is guaranteed to return already zeroed memory
// malloc
#define mem_alloc_arr(_count, _type) (_type *)mem_alloc(_count * sizeof(_type))
ATTR((malloc))
void *mem_alloc(uptr size);
// realloc
void *mem_realloc(void *ptr, uptr old_size, uptr size);
// strdup
char *mem_alloc_str(const char *str);
// free
void mem_free(void *ptr, uptr size);
// memcpy
void mem_copy(void *dst, const void *src, uptr size);
// memmove
void mem_move(void *dst, const void *src, uptr size);
// memset
void mem_zero(void *dst, uptr size);
// memcmp
bool mem_eq(const void *a, const void *b, uptr n);
