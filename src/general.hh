#if !defined(GENERAL_H)

#ifndef INTERNAL_BUILD
#define INTERNAL_BUILD 1
#endif

#define CT_ASSERT(_expr) static_assert(_expr, "Assertion " #_expr " failed")

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <stdarg.h>
#include <ctype.h>

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

#define BYTES(_n) ((size_t)_n)
#define KILOBYTES(_n) (BYTES(_n) << 10) 
#define MEGABYTES(_n) (KILOBYTES(_n) << 10) 

#define STRUCT_FIELD(_struct, _field) (((_struct *)(0))->_field)
#define STRUCT_OFFSET(_struct, _field) (&STRUCT_FIELD(_struct, _field))
#define MAX_VALUE(_variable) ((1llu << (8llu * sizeof(_variable))) - 1)
#define INVALID_DEFAULT_CASE default: assert(!"Invalid switch default case");
#define INVLALID_CODE_PATH assert(!"Invalid code path")
#define UNREFERENCED_VARIABLE(_var) ((void)_var)

#define GENERAL_H 1
#endif
