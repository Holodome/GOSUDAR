#if !defined(ASSETS_HH)

#include "lib/lib.hh"
#include "framework/lexer.hh"
#include "framework/renderer.hh"

enum AssetCategory {
    Asset_White,
    Asset_Dude,
    Asset_Tree,
    Asset_Font,
    Asset_FontAtlas,
    Asset_Grass,
    Asset_WoodIcon,
    Asset_Building,
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

#define ASSETS_HH 1
#endif
