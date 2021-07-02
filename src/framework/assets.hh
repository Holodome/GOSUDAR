#if !defined(ASSETS_HH)

#include "lib.hh"
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
    FontGlyph glyphs[256];
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
    Texture textures[32];
    size_t font_count;
    FontData fonts[1];
    
    void init();
    void cleanup();
    
    AssetInfo *get_info(AssetID id);
    
    Texture get_tex(AssetID id);
    FontData *get_font(AssetID id);
    
    Vec2 get_text_size(AssetID id, const char *text, size_t count = 0, f32 scale = 1.0f);
};


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
