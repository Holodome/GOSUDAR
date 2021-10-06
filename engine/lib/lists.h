// Author: Holodome
// Date: 25.08.2021 
// File: pkby/src/lib/hashing.h
// Version: 1
// 
#pragma once
#include "lib/general.h"
#include "memory.h"

// @NOTE(hl): Although it was previously named LLIST_*,
// this implementations are more stack-based
#define STACK_ADD(_list, _node) \
do { \
(_node)->next = (_list); \
(_list) = (_node); \
} while (0); 
#define STACK_POP(_list) \
do { \
(_list) = (_list)->next; \
} while (0);

#define CDLIST_INIT(_list) \
do {\
(_list)->next = (_list); \
(_list)->prev = (_list); \
} while (0);
#define CDLIST_ADD(_list, _node) \
do { \
(_node)->next = (_list)->next; \
(_node)->prev = (_list); \
(_node)->next->prev = (_node); \
(_node)->prev->next = (_node); \
} while (0);
#define CDLIST_ADD_LAST(_list, _node) \
do { \
(_node)->next = (_list); \
(_node)->prev = (_list)->prev; \
(_node)->next->prev = (_node); \
(_node)->prev->next = (_node); \
} while (0);
#define CDLIST_REMOVE(_node)\
do {\
(_node)->prev->next = (_node)->next;\
(_node)->next->prev = (_node)->prev;\
} while (0);

typedef struct {
    u32 size;
    u32 capacity;
} DArray_Header;

#define da_header(_da) ((DArray_Header *)((u8 *)(_da) - sizeof(DArray_Header)))
#define da_size(_da) ((_da) ? da_header(_da)->size : 0)
#define da_capacity(_da) ((_da) ? da_header(_da)->capacity : 0)
#define da_is_full(_da) ((_da) ? (da_size(_da) == da_capacity(_da)) : true)
#define da_push(_da, _it) \
do { \
    da_is_full(_da) ? (_da) = da_grow((_da), sizeof(*(_da))) : (void)0; \
    (_da)[da_header(_da)->size++] = (_it); \
} while(0);
#define da_reserve(_type, _size) da_reserve_(sizeof(_type), _size)
#define da_free(_da) da_free_((_da), sizeof(*(_da)))
void *da_reserve_(u32 stride, u32 count);
void *da_grow(void *a, u32 stride);
void da_free_(void *a, u32 stride);