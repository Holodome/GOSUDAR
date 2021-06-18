#if !defined(ASSETS_HH)

#include "lib/lib.hh"
#include "framework/lexer.hh"
#include "framework/renderer.hh"

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
    Texture tex;  
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
    Str name;  
    AssetKind kind;
    AssetState state;
    size_t array_entry_idx;
    
    Str filename;
    Vec2 size;
    f32 height;
};

struct Assets {
    // This should only be texture creating-related stuff
    Renderer *renderer;
    
    Str sprites_cfg_name;
    HashTable<AssetInfo> asset_infos;
    
    Array<Texture> textures;
    Array<FontData> fonts;
    
    void init(const char *sprites_cfg_name);
    void cleanup();
    
    AssetInfo *get_info(const char *name);
    
    Texture get_tex(const char *name);
    FontData *get_font(const char *name);
    
    Vec2 get_text_size(const char *name, const char *text, size_t count = 0, f32 scale = 1.0f);
};

#define ASSETS_HH 1
#endif
