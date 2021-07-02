#if !defined(MIMS_HH)

#include "lib.hh"

struct ImageU32 {
    u32 width;
    u32 height;
    u32 *data;  
};

struct MipIterator {
    u32 level;
    ImageU32 image;
};  

inline f32
linear1_to_srgb1(f32 l) {
    l = clamp(l, 0, 0.999f);
    
    f32 s = l * 12.92f;
    if (l > 0.0031308f) {
        s = 1.055f * powf(l, 1.0 / 2.4f) - 0.055f;
    }
    return s;
}

void downsample2x(ImageU32 src, ImageU32 dest) {
    u32 *dest_px = dest.data;
    u32 *src_px_row = src.data;
    
    size_t n_wirtten = 0;
    for (size_t y = 0; y < dest.height; ++y) {
        u32 *src_px0 = src_px_row;
        u32 *src_px1 = src_px_row;
        if (y + 1 < src.height) {
            src_px1 += src.width;
        }
        
        for (size_t x = 0; x < dest.width; ++x) {
            Vec4 p00 = rgba_unpack_linear1(*src_px0++);
            Vec4 p01 = rgba_unpack_linear1(*src_px1++);
            Vec4 p10 = p00;
            Vec4 p11 = p01;
            if (x + 1 < src.width) {
                p10 = rgba_unpack_linear1(*src_px0++);
                p11 = rgba_unpack_linear1(*src_px1++);
            }

            // #define PREPARE_COLOR(_color)    \
            // if (_color.a == 0.0f) {          \
            //     _color.xyz = Vec3(0);        \
            // } 
            // PREPARE_COLOR(p00)
            // PREPARE_COLOR(p01)
            // PREPARE_COLOR(p10)
            // PREPARE_COLOR(p11)
            
            Vec4 c = (p00 + p10 + p01 + p11) * 0.25f;
            *dest_px++ = rgba_pack_4x8_linear1(c);
            ++n_wirtten;
        }
        src_px_row += 2 * src.width;
    }
    assert(n_wirtten == dest.width * dest.height);
}

MipIterator iterate_mips(u32 width, u32 height, void *data) {
    MipIterator iter;
    iter.level = 0;
    iter.image.width = width;
    iter.image.height = height;
    iter.image.data = (u32 *)data;
    return iter;
}

bool is_valid(MipIterator *iter) {
    return iter->image.width && iter->image.height;
}

void advance(MipIterator *iter) {
    iter->image.data += (iter->image.width * iter->image.height);
    if (iter->image.width == 1 && iter->image.height == 1) {
        iter->image.width = iter->image.height = 0;
    } else {
        ++iter->level;
        if (iter->image.width > 1) {
            iter->image.width = (iter->image.width + 1) / 2;
        } 
        if (iter->image.height > 1) {
            iter->image.height = (iter->image.height + 1) / 2;
        }
    }
}

size_t get_total_size_for_mips(u32 width, u32 height) {
    size_t result = 0;
    for (MipIterator iter = iterate_mips(width, height, 0);
         is_valid(&iter);
         advance(&iter)) {
        result += iter.image.width * iter.image.height * 4;
    }
    return result;
}

void generate_sequential_mips(u32 width, u32 height, void *data) {
    MipIterator iter = iterate_mips(width, height, data);
    ImageU32 src = iter.image;
    advance(&iter);
    while(is_valid(&iter)) {
        assert((u8 *)src.data + src.width * src.height * 4 == (u8 *)iter.image.data);
        downsample2x(src, iter.image);
        src = iter.image;
        advance(&iter);
    }
    assert(get_total_size_for_mips(width, height) == ((u8 *)iter.image.data - (u8 *)data));
}



#define MIMS_HH 1
#endif
