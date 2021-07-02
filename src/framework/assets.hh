#if !defined(ASSETS_HH)

#include "lib/lib.hh"
#include "framework/lexer.hh"
#include "framework/renderer.hh"

enum AssetCategory {
    Asset_White,
    Asset_Dude,
    Asset_TreeForest,
    Asset_TreeJungle,
    Asset_TreeDesert,
    Asset_SelectCircle,
    Asset_GoldVein,
    Asset_Font,
    Asset_FontAtlas,
    Asset_Grass,
    Asset_WoodIcon,
    Asset_Building,
    Asset_Building1,
    Asset_Count
};  

struct FontGlyph {
	u32 utf32;
	u16 min_x;
	u16 min_y;
	u16 max_x;
	u16 max_y;
	f32 offset1_x;
	f32 offset1_y;
	f32 offset2_x;
	f32 offset2_y;
	f32 x_advance;
};

struct FontData {
    Vec2i tex_size;
    AssetID texture_id;
    Array<FontGlyph> glyphs;
    u32 first_codepoint;
};

enum struct AssetKind {
    None = 0x0,
    Image,
    Font
};

enum struct AssetState {
    Unloaded,
    Loaded  
};

struct AssetInfo {
    AssetKind kind;
    AssetState state;
    size_t array_entry_idx;
    
    const char *filename;
    Vec2 size;
    f32 height;
};


struct Assets {
    MemoryArena arena;
    // This should only be texture creating-related stuff
    Renderer *renderer;
    
    AssetInfo asset_infos[Asset_Count];
    size_t texture_count;
    Texture textures[128];
    size_t font_count;
    FontData fonts[128];
    
    void init();
    void cleanup();
    
    AssetInfo *get_info(AssetID id);
    
    Texture get_tex(AssetID id);
    FontData *get_font(AssetID id);
    
    Vec2 get_text_size(AssetID id, const char *text, size_t count = 0, f32 scale = 1.0f);
};

struct ImageU32 {
    u32 width;
    u32 height;
    u32 *data;  
};

struct MipIterator {
    u32 level;
    ImageU32 image;
};  

inline Vec4 rgba_unpack(u32 rgba) {
    f32 r = (f32)((rgba & 0x000000FF) >> 0 ) * (1.0f / 255.0f);
    f32 g = (f32)((rgba & 0x0000FF00) >> 8 ) * (1.0f / 255.0f);
    f32 b = (f32)((rgba & 0x00FF0000) >> 16) * (1.0f / 255.0f);
    f32 a = (f32)((rgba & 0xFF000000) >> 24) * (1.0f / 255.0f);
    return Vec4(r, g, b, a);
}

inline u32 
rgba_pack_4x8(u32 r, u32 g, u32 b, u32 a) {
    // If values passed here are greater that 255 something for sure went wrong
    assert(r <= 0xFF && b <= 0xFF && b <= 0xFF && a <= 0xFF);
    return r << 0 | g << 8 | b << 16 | a << 24;
}

inline u32
rgba_pack_4x8_linear1(Vec4 c) {
    u32 ru = roundf(Math::clamp(c.r, 0, 0.999f) * 255.0f);
    u32 gu = roundf(Math::clamp(c.g, 0, 0.999f) * 255.0f);
    u32 bu = roundf(Math::clamp(c.b, 0, 0.999f) * 255.0f);
    u32 au = roundf(Math::clamp(c.a, 0, 0.999f) * 255.0f);
    return rgba_pack_4x8(ru, gu, bu, au);
}

inline f32
linear1_to_srgb1(f32 l) {
    l = Math::clamp(l, 0, 0.999f);
    
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
            Vec4 p00 = rgba_unpack(*src_px0++);
            Vec4 p01 = rgba_unpack(*src_px1++);
            Vec4 p10 = p00;
            Vec4 p11 = p01;
            if (x + 1 < src.width) {
                p10 = rgba_unpack(*src_px0++);
                p11 = rgba_unpack(*src_px1++);
            }
            
            #define PREPARE_COLOR(_color)    \
            if (_color.a == 0.0f) {          \
                _color.xyz = Vec3(0);        \
            } 
            PREPARE_COLOR(p00)
            PREPARE_COLOR(p01)
            PREPARE_COLOR(p10)
            PREPARE_COLOR(p11)
            
            Vec4 c = (p00 + p10 + p01 + p11) * 0.25f;
            // if (c.a < 0.25f) {
            //     c.a = 0.0f;
            // }
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



/*

// Tags is per-asset kind settings
#define ASSET_TAG_COUNT 10
struct AssetTags {
    f32 tags[ASSET_TAG_COUNT];
};

enum AssetKind {
    AssetKind_Tree, // Tags: biome(kind), subkind(bush/tree), health, height, 
    AssetKind_Building, // Tags: kind, build progress, health
    AssetKind_Player, // Tags: facing direction, health 
    AssetKind_Font, // Tags: codepoint
    AssetKind_InterfaceElement, // Tags: kind, state (if button)
};
// For example, when program wants to get some assetfor tree,
// it does not need to specify all parameters itself and select asset from list
// Instead it specifies tags and lets assets system choose best match 
// This way we can have enums for asset kinds with small number of kinds,
// but each of them can have unlimited number of individual assets

// If we keep ASSET_TAG_COUNT small, we can use bitmask for choosing asset tags needed
// Tags-mask can be computed with knowing kind
Asset *get_asset_internal(AssetKind kind, AssetTags weights, AssetTags matches);
Asset *get_asset(AssetKind kind, AssetTags weights);

*/

#define ASSETS_HH 1
#endif
