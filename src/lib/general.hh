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

#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof(*(_a)))

#define BYTES(_n) ((size_t)_n)
#define KILOBYTES(_n) (BYTES(_n) << 10) 
#define MEGABYTES(_n) (KILOBYTES(_n) << 10) 

char *
read_file_and_null_terminate(char *filename) {
    char *result = 0;
    
    FILE *file = fopen(filename, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        u64 size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        result = (char *)malloc(size + 1);
        fread(result, 1, size, file);
        result[size] = 0;
        
        fclose(file);
    }
    
    return result;
}

#define GENERAL_H 1
#endif
