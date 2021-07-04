#if !defined(ASSETS_HH)

#include "lib.hh"

// Tags is per-asset kind settings
#define ASSET_TAG_COUNT 10
struct AssetTagList {
    f32 tags[ASSET_TAG_COUNT];
};

enum {
    // Tress
    ASSET_TAG_BIOME = 0,  
    // Building
    ASSET_TAG_WORLD_OBJECT_KIND = 0,
    ASSET_TAG_BUILDING_IS_BUILT = 1,
    // Player 
    // Font
};

enum AssetType {
    ASSET_TYPE_NONE,
    ASSET_TYPE_WORLD_OBJECT,
    ASSET_TYPE_PLAYER,  
    ASSET_TYPE_FONT,    
    ASSET_TYPE_GRASS,    
    ASSET_TYPE_ADDITIONAL,
    ASSET_TYPE_SENTINEL,  
};
// For example, when program wants to get some assetfor tree,
// it does not need to specify all parameters itself and select asset from list
// Instead it specifies tags and lets assets system choose best match 
// This way we can have enums for asset kinds with small number of kinds,
// but each of them can have unlimited number of individual assets

struct AssetTypeInfo {
    u32 first_info_idx;
    u32 asset_count;
};  

enum {
    ASSET_STATE_UNLOADED,  
    ASSET_STATE_LOADED,  
};

enum {
    ASSET_KIND_NONE,
    ASSET_KIND_TEXTURE,
    ASSET_KIND_FONT,
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

struct AssetFont {
    f32 height;
    Texture texture;
    FontGlyph glyphs[256];
    u32 first_codepoint;
    u32 codepoint_count;
};

struct AssetInfo {
    u32 kind;
    u32 state;
    const char *filename;
    
    u32 first_tag_idx;
    u32 tag_count;
    union {
        Texture   *texture;
        AssetFont *font;
    };
};

struct AssetTag {
    u32 id;
    f32 value;  
};

struct Assets {
    MemoryArena arena;
    
    struct Renderer *renderer;
    
    u32 max_asset_infos;
    u32 asset_info_count;
    AssetInfo *asset_infos;
    
    u32 max_tags_count;
    u32 tags_count;
    AssetTag *tags;

    AssetTypeInfo type_infos[ASSET_TYPE_SENTINEL];
};  

Assets *assets_init(Renderer *renderer);
AssetID get_closest_asset_match(Assets *assets, AssetType type, AssetTagList *weights, AssetTagList *matches);
AssetID get_first_of_type(Assets *assets, AssetType type);
Texture *assets_get_texture(Assets *assets, AssetID id);
AssetFont *assets_get_font(Assets *assets, AssetID id);

Vec2 get_text_size(AssetFont *font, const char *text);

#define ASSETS_HH 1
#endif
