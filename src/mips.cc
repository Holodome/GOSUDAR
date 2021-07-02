#include "mips.hh"

static void downsample2x(u32 src_width, u32 src_height, u32 *src_px,
    u32 dest_width, u32 dest_height, u32 *dest_px) {
    size_t DEBUG_n_written = 0;
    for (size_t y = 0; y < dest_height; ++y) {
        u32 *src_px0 = src_px;
        u32 *src_px1 = src_px;
        if (y + 1 < src_height) {
            src_px1 += src_width;
        }
        
        for (size_t x = 0; x < dest_width; ++x) {
            Vec4 p00 = rgba_unpack_linear1(*src_px0++);
            Vec4 p01 = rgba_unpack_linear1(*src_px1++);
            Vec4 p10 = p00;
            Vec4 p11 = p01;
            if (x + 1 < src_width) {
                p10 = rgba_unpack_linear1(*src_px0++);
                p11 = rgba_unpack_linear1(*src_px1++);
            }
#if 0
            // Straigt averags
            Vec4 c = (p00 + p10 + p01 + p11) * 0.25f;
#else 
            // Weighted based on alpha average
            f32 alpha_sum = p00.a + p01.a + p10.a + p11.a;
            Vec4 c = Vec4((p00.xyz * p00.a + p01.xyz * p01.a + p10.xyz * p10.a + p11.xyz * p11.a) / alpha_sum,
                alpha_sum * 0.25f);
#endif 
            *dest_px++ = rgba_pack_4x8_linear1(c);
            ++DEBUG_n_written;
        }
        src_px += 2 * src_width;
    }
    
    assert(DEBUG_n_written == dest_width * dest_height);
}

MipIterator iterate_mips(u32 width, u32 height) {
    MipIterator iter;
    iter.level = 0;
    iter.width = width;
    iter.height = height;
    iter.pixel_offset = 0;
    return iter;
}

bool is_valid(MipIterator *iter) {
    return iter->width && iter->height;
}

void advance(MipIterator *iter) {
    iter->pixel_offset += iter->width * iter->height;
    if (iter->width == 1 && iter->height == 1) {
        iter->width = iter->height = 0;
    } else {
        ++iter->level;
        if (iter->width > 1) {
            iter->width = (iter->width + 1) / 2;
        } 
        if (iter->height > 1) {
            iter->height = (iter->height + 1) / 2;
        }
    }
}

size_t get_total_size_for_mips(u32 width, u32 height) {
    size_t result = 0;
    for (MipIterator iter = iterate_mips(width, height);
         is_valid(&iter);
         advance(&iter)) {
        result += iter.width * iter.height * 4;
    }
    return result;
}

void generate_sequential_mips(u32 width, u32 height, void *data) {
    MipIterator iter = iterate_mips(width, height);
    advance(&iter);
    u32 *src_px = (u32 *)data;
    u32 src_width = width;
    u32 src_height = height;
    while(is_valid(&iter)) {
        downsample2x(src_width, src_height, src_px, iter.width, iter.height, (u32 *)data + iter.pixel_offset);
        src_px = (u32 *)data + iter.pixel_offset;
        src_width = iter.width;
        src_height = iter.height;
        advance(&iter);
    }
    assert(get_total_size_for_mips(width, height) == iter.pixel_offset * 4);
}

