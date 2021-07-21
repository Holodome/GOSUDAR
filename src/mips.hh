//
// This file describes mimmap interface
//
#if !defined(MIPS_HH)

#include "lib.hh"

// Iterator for mipmap levels
// It goes from source size, making each dimensions two times smaller with flooring on each new mip level
struct MipIterator {
    u32 level;
    u32 width;
    u32 height;
    // Offset in array continiously storing mipmap image data
    // This isn't necessary for iterating, but is used in several cases and placed here for convenience
    size_t pixel_offset;
};  

MipIterator iterate_mips(u32 width, u32 height);
bool is_valid(MipIterator *iter);
void next(MipIterator *iter);
// @CLEANUP Used in asset system, but place here anyway
void generate_sequential_mips(u32 width, u32 height, void *data);
size_t get_total_size_for_mips(u32 width, u32 height);

#define MIPS_HH 1
#endif
