#if !defined(GENERAL_H)

#define CT_ASSERT(_expr) static_assert(_expr, "Assertion " #_expr " failed")

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <stdarg.h>
#include <ctype.h>

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
// For overengineering!
// We want to use as small data types for enums as possible, so its better to have some check that all enum values can fit into it
// Sentinel is max value of enum + 1. So in case u8 field, 1 << 8 >= (1 << 8 + 1) - false, because 8-bit value cant fit 9 bit
// Max value of u8 is (1 << 8 - 1) and enum is 1 << 8, so (1 << 8 - 1) >= (1 << 8) <=> (1 << 8) >= (1 << 8 + 1) - right side is 
#define ENUM_FITS_IN_VARIABLE(_sentinel, _variable) (_sentinel <= (1llu << (8llu * sizeof(_variable)) ))
#define INVALID_DEFAULT_CASE default: assert(false);

#define GENERAL_H 1
#endif
