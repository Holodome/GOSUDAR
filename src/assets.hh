#if !defined(ASSETS_HH)

#include "lib.hh"

#include "game_enums.hh"
#include "asset_file.hh"
#include "os.hh"

// Tags is per-asset kind settings
#define ASSET_TAG_COUNT 10
struct AssetTagList {
    f32 tags[ASSET_TAG_COUNT];
};

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
    ASSET_KIND_SOUND,
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
    Texture texture;
    FontGlyph *glyphs;
};

struct AssetSound {
    // Samples in format [ 1: [l, r], 2: [l, r], ...]
    // Channel samples are stored next to each other for each individual sample
    // It may be benefitial to switch to having arrays of samples for each channel to simplify sound play arithmetic
    i16 *samples;
};  

struct Asset {
    AssetInfo file_info;
    u32 state;
    union {
        AssetFont  *font;
        AssetSound *sound;
        Texture    *texture;
    };
};

struct AssetTag {
    u32 id;
    f32 value;  
};

struct Assets {
    MemoryArena arena;
    MemoryArena *frame_arena;
    struct Renderer *renderer;
    
    FileHandle asset_file;
    
    u32 asset_info_count;
    Asset *asset_infos;
    
    u32 tags_count;
    AssetTag *tags;
    
    u32 type_info_count;
    AssetTypeInfo *type_infos;
};  

Assets *assets_init(Renderer *renderer, MemoryArena *frame_arena);
AssetID assets_get_closest_match(Assets *assets, AssetType type, AssetTagList *weights, AssetTagList *matches);
AssetID assets_get_first_of_type(Assets *assets, AssetType type);
AssetInfo *assets_get_info(Assets *assets, AssetID id);
Texture *assets_get_texture(Assets *assets, AssetID id);
AssetFont *assets_get_font(Assets *assets, AssetID id);
AssetSound *assets_get_sound(Assets *assets, AssetID id);
// This is not related to the assets api, but rather to assets usage code.
// Currently it is used in the dev ui, but more sophisticated ui system probably will
// not render use this, as it may want to do dynamic wrapping or something
// And furthermore, this function is not related to assets sytem in any way
Vec2 DEBUG_get_text_size(Assets *assets, AssetID id, const char *text);

#define ASSETS_HH 1
#endif
