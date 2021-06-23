#include "framework/assets.hh"

#include "framework/os.hh"

#define STBI_MALLOC Mem::alloc
#define STBI_REALLOC Mem::realloc
#define STBI_FREE Mem::free
#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"

#define STBTT_malloc(x,u)  ((void)(u),Mem::alloc(x))
#define STBTT_free(x,u)    ((void)(u),Mem::free(x))
#define STB_TRUETYPE_IMPLEMENTATION
#include "thirdparty/stb_truetype.h"

const char *ASSET_KINDS[] = {
    "None",
    "Image",
    "Font"
};

const char *ASSET_STATES[] = {
    "Unloaded",
    "Loaded"  
};

void Assets::init() {
    logprintln("Assets", "Init start");
    
    this->font_count = 0;
    this->texture_count = 0;
    this->asset_infos[Asset_White].filename = "white.png";
    this->asset_infos[Asset_White].kind = AssetKind::Image;
    this->asset_infos[Asset_White].state = AssetState::Unloaded;
    
    this->asset_infos[Asset_Dude].filename = "dude.png";
    this->asset_infos[Asset_Dude].kind = AssetKind::Image;
    this->asset_infos[Asset_Dude].state = AssetState::Unloaded;
    
    this->asset_infos[Asset_Grass].filename = "grass.png";
    this->asset_infos[Asset_Grass].kind = AssetKind::Image;
    this->asset_infos[Asset_Grass].state = AssetState::Unloaded;
    
    this->asset_infos[Asset_TreeForest].filename = "tree.png";
    this->asset_infos[Asset_TreeForest].kind = AssetKind::Image;
    this->asset_infos[Asset_TreeForest].state = AssetState::Unloaded;
    
    this->asset_infos[Asset_TreeJungle].filename = "jungle.png";
    this->asset_infos[Asset_TreeJungle].kind = AssetKind::Image;
    this->asset_infos[Asset_TreeJungle].state = AssetState::Unloaded;
    
    this->asset_infos[Asset_TreeDesert].filename = "cactus.png";
    this->asset_infos[Asset_TreeDesert].kind = AssetKind::Image;
    this->asset_infos[Asset_TreeDesert].state = AssetState::Unloaded;
    
    this->asset_infos[Asset_GoldVein].filename = "gold.png";
    this->asset_infos[Asset_GoldVein].kind = AssetKind::Image;
    this->asset_infos[Asset_GoldVein].state = AssetState::Unloaded;
    
    this->asset_infos[Asset_SelectCircle].filename = "select.png";
    this->asset_infos[Asset_SelectCircle].kind = AssetKind::Image;
    this->asset_infos[Asset_SelectCircle].state = AssetState::Unloaded;
    
    this->asset_infos[Asset_WoodIcon].filename = "wood_icon.png";
    this->asset_infos[Asset_WoodIcon].kind = AssetKind::Image;
    this->asset_infos[Asset_WoodIcon].state = AssetState::Unloaded;
    
    this->asset_infos[Asset_Building].filename = "building.png";
    this->asset_infos[Asset_Building].kind = AssetKind::Image;
    this->asset_infos[Asset_Building].state = AssetState::Unloaded;
    
    this->asset_infos[Asset_Font].filename = "c:/windows/fonts/consola.ttf";
    this->asset_infos[Asset_Font].kind = AssetKind::Font;
    this->asset_infos[Asset_Font].state = AssetState::Unloaded;
    this->asset_infos[Asset_Font].height = 16;

    this->asset_infos[Asset_FontAtlas].kind = AssetKind::Image;
    this->asset_infos[Asset_FontAtlas].state = AssetState::Unloaded;
    
    // @CLEAN
    this->get_tex(Asset_White);
    logprintln("Assets", "Init end");
}

void Assets::cleanup() {
}
    
AssetInfo *Assets::get_info(AssetID id) {
    assert(id < Asset_Count);
    AssetInfo *result = &this->asset_infos[id];
    return result;
}

Texture Assets::get_tex(AssetID id) {
    AssetInfo *info = this->get_info(id);
    assert(info->kind == AssetKind::Image);
    if (info->state == AssetState::Loaded) {
    } else {
        // logprintln("Assets", "Loading texture '%s'", name);
        
        FileHandle file = OS::open_file(info->filename);
        assert(OS::is_file_handle_valid(file));
        size_t file_size = OS::get_file_size(file);
        void *buffer = Mem::alloc(file_size);
        OS::read_file(file, 0, file_size, buffer);
        
        int w, h;
        void *data = stbi_load_from_memory((const stbi_uc *)buffer, file_size, &w, &h, 0, 4);
        Vec2i tex_size = Vec2i(w, h);
        Mem::free(buffer);
        
        Texture tex = renderer->create_texture(data, tex_size);
        size_t idx = this->texture_count++;
        this->textures[idx] = tex;
        Mem::free(data);
        
        info->state = AssetState::Loaded;
        info->array_entry_idx = idx;
        // logprintln("Assets", "Loaded texture '%s'", name);
    }
    return this->textures[info->array_entry_idx];
}

FontData *Assets::get_font(AssetID id) {
    AssetInfo *info = this->get_info(id);
    assert(info->kind == AssetKind::Font);
    FontData *result = 0;
    if (info->state == AssetState::Loaded) {
    } else {
        // logprintln("Assets", "Loading font '%s'", name);
         
        FileHandle file = OS::open_file(info->filename);
        assert(OS::is_file_handle_valid(file));
        size_t file_size = OS::get_file_size(file);
        void *buffer = Mem::alloc(file_size);
        OS::read_file(file, 0, file_size, buffer);
        
        const u32 atlas_width  = 512;
        const u32 atlas_height = 512;
        const u32 first_codepoint = 32;
        const u32 codepoint_count = 95;  
        stbtt_packedchar *glyphs = new stbtt_packedchar[codepoint_count];
        
        u8 *loaded_atlas_data = new u8[atlas_width * atlas_height];
        stbtt_pack_context context = {};
        stbtt_PackBegin(&context, loaded_atlas_data, atlas_width, atlas_height, 0, 1, 0);
        stbtt_PackSetOversampling(&context, 2, 2);
        stbtt_PackFontRange(&context, (u8 *)buffer, 0, info->height, first_codepoint, codepoint_count, glyphs);
        stbtt_PackEnd(&context);
        
        Mem::free(buffer);

        u8 *atlas_data = new u8[atlas_width * atlas_height * 4];
        for (u32 i = 0; i < atlas_width * atlas_height; ++i) {
            u8 *dest = (u8 *)(atlas_data + i * 4);
            dest[0] = 0xFF;
            dest[1] = 0xFF;
            dest[2] = 0xFF;
            dest[3] = loaded_atlas_data[i];
        }
        delete[] loaded_atlas_data;
        
        size_t array_idx = this->font_count++;
        FontData *font = &this->fonts[array_idx];
        this->textures[this->texture_count] = renderer->create_texture(atlas_data, Vec2i(atlas_width, atlas_height));
        AssetInfo *tex_info = this->get_info(Asset_FontAtlas);
        tex_info->state = AssetState::Loaded;
        tex_info->array_entry_idx = this->texture_count;
        ++this->texture_count;
        delete[] atlas_data;
        
        font->texture_id = Asset_FontAtlas;
        font->tex_size = Vec2i(atlas_width, atlas_height);
        font->first_codepoint = first_codepoint;
        font->glyphs.resize(codepoint_count);

        for (u32 i = 0; i < codepoint_count; ++i) {
            ++font->glyphs.len;
            font->glyphs[i].utf32 = first_codepoint + i;
            font->glyphs[i].min_x = glyphs[i].x0;
            font->glyphs[i].min_y = glyphs[i].y0;
            font->glyphs[i].max_x = glyphs[i].x1;
            font->glyphs[i].max_y = glyphs[i].y1;
            font->glyphs[i].offset1_x = glyphs[i].xoff;
            font->glyphs[i].offset1_y = glyphs[i].yoff;
            font->glyphs[i].offset2_x = glyphs[i].xoff2;
            font->glyphs[i].offset2_y = glyphs[i].yoff2;
            font->glyphs[i].x_advance = glyphs[i].xadvance;
        }
        delete glyphs;
        info->state = AssetState::Loaded;
        info->array_entry_idx = array_idx;
        logprintln("Fonts", "Loaded font '%s'", info->filename);
    }
    return &this->fonts[info->array_entry_idx];
}

Vec2 Assets::get_text_size(AssetID id, const char *text, size_t count, f32 scale) {
    AssetInfo *info = this->get_info(id);
    FontData *font = this->get_font(id);
    
    if (!count) {
        count = strlen(text);
    }
    
    Vec2 result = {};
    for (u32 i = 0; i < count; ++i) {
        char s = text[i];
        if (s >= font->first_codepoint && s < (font->first_codepoint + font->glyphs.len)) {
            FontGlyph *glyph = &font->glyphs[s - font->first_codepoint];
            // FontGlyph *glyph = &glyphs[first_codepoint];
            result.x += glyph->x_advance * scale;
        }
    }
    result.y = info->height * scale;
    
    return result;   
}