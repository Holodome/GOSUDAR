#if !defined(ASSETS_HH)

#include "lib/lib.hh"
#include "framework/lexer.hh"
#include "framework/renderer.hh"

struct TextureData {
    Str filename;
    Str name;
    void *data;
    Vec2i size;  
    Texture texture;
    
    TextureData(const char *filename, const char *name = 0);
    TextureData(const char *name, void *data, Vec2i size);
    ~TextureData();
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
    f32 size;
    Texture *tex;  
    Array<FontGlyph> glyphs;
    u32 first_codepoint;
    
    FontData(const char *filename, f32 size); 
    ~FontData();
    Vec2 get_text_size(const char *text, size_t count = 0, f32 scale = 1.0f);   
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
    AssetKind kind = AssetKind::None;
    AssetState state = AssetState::Unloaded;
    Str name;  
    Str filename;
    Vec2 size;
    size_t array_entry_idx;
};

struct Assets {
    Str sprites_cfg_name;
    HashTable<AssetInfo> asset_infos;
    
    Array<TextureData> texture_datas;
    Array<FontData> font_datas;
    
    void init(const char *sprites_cfg_name);
    void cleanup();
    
    AssetInfo *get_info(const char *name);
    void load(const char *name);
    
    TextureData *get_tex_data(const char *name);
    Texture *get_tex(const char *name);
    FontData *get_font(const char *name);
};

extern Assets *assets;

#define ASSETS_HH 1
#endif
