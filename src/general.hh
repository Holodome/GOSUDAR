#if !defined(GENERAL_H)

#ifndef INTERNAL_BUILD
#define INTERNAL_BUILD 1
#endif

#define CT_ASSERT(_expr) static_assert(_expr, "Assertion " #_expr " failed")

#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>

#if INTERNAL_BUILD
#include <assert.h>
#else 
#define assert(_expr) (_expr)
#endif 

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

#define ARRAY_SIZE(_a) ((size_t)(sizeof(_a) / sizeof(*(_a))))
#define SQ(_a) ((_a) * (_a))

#define BYTES(_n) ((size_t)_n)
#define KILOBYTES(_n) (BYTES(_n) << 10) 
#define MEGABYTES(_n) (KILOBYTES(_n) << 10) 

// This is just to follow the style
#define SIZE_OF(_v) (sizeof(_v))
#define STRUCT_FIELD(_struct, _field) (((_struct *)(0))->_field)
#define STRUCT_OFFSET(_struct, _field) ((size_t)((u8 *)(&STRUCT_FIELD(_struct, _field))))
#define MAX_VALUE(_variable) ((1llu << (8llu * sizeof(_variable))) - 1)
#define INVALID_DEFAULT_CASE default: assert(!"Invalid switch default case");
#define INVALID_CODE_PATH assert(!"Invalid code path")
#define UNREFERENCED_VARIABLE(_var) ((void)_var)

// ID stuff
// In code there are several places where ids of different kinds are used
// But they all share same functionality and could as well just be integers 
// But decision was made to make them stucts to have some advantages of strong typing
// All id structs are defined with single field named value
#define IS_SAME(_a, _b) ((_a).value == (_b).value)
#define IS_NULL(_id) ((_id).value == 0)
#define IS_NOT_NULL(_id) ((_id).value != 0)

// Linked list
// Linked list entries must have .next field
#define LLIST_ITER(_name, _list) for (auto (_name) = (_list); (_name); (_name) = (_name)->next)
#define LLIST_ADD(_list, _node) do { (_node)->next = (_list); (_list) = (_node); } while (0);
#define LLIST_POP(_list) do { (_list) = (_list)->next; } while(0);
#define LLIST_ADD_OR_CREATE(_list_ptr, _node) do { \
if (*(_list_ptr)) { \
LLIST_ADD(*(_list_ptr), (_node)); \
} else { \
*(_list_ptr) = (_node); \
} \
} while (0);
// Double-linked list entries must have .next and .prev fields
// Ciricular double-linked list
// It works by defining single statically-allocated element, which serves as sentinel
// in points to no data but is used to make function calls easier (no need for double pointers)
#define CDLIST_ITER(_name, _list) for (auto (_name) = (_list)->next; (_name) != (_list); (_name) = (_name)->next)
#define CDLIST_INIT(_list) do { (_list)->next = (_list); (_list)->prev = (_list); } while (0);
#define CDLIST_ADD(_list, _node) do { \
(_node)->next = (_list)->next; \
(_node)->prev = (_list); \
(_node)->next->prev = (_node); \
(_node)->prev->next = (_node); \
} while (0);
#define CSLIST_ADD_LAST(_list, _node) do { \
(_node)->next = (_list); \
(_node)->prev = (_list)->prev; \
(_node)->next->prev = (_node); \
(_node)->prev->next = (_node); \
} while (0);
#define CDLIST_REMOVE(_node) do {\
(_node)->prev->next = (_node)->next;\
(_node)->next->prev = (_node)->prev;\
} while (0);

// All iterators are defined with 3 functions:
// next() - used internally (but can actually be made part of api???)
// is_valid()
// advance()
// This API allows use of this macro which saves space writing countless for loops
#define ITERATE(_iter_name, _iterator) for (auto (_iter_name) = (_iterator); is_valid(&(_iter_name)); advance(&(_iter_name)) )

#ifndef GOSUDAR_NO_STDIO
#define GOSUDAR_NO_STDIO 0
#endif

#if GOSUDAR_NO_STDIO
#include "stb_sprintf.h"
#ifdef snprintf 
#undef snprintf
#endif 
#define snprintf stbsp_snprintf
#ifdef vsnprintf
#undef vsnprintf
#endif 
#define vsnprintf stbsp_vsnprintf
#else 
#include <stdio.h>
#endif 

#define GENERAL_H 1
#endif
