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


Assets *assets;

const char *ASSET_KINDS[] = {
    "None",
    "Image",
    "Font"
};

const char *ASSET_STATES[] = {
    "Unloaded",
    "Loaded"  
};

void Assets::init(const char *sprites_cfg_name) {
    logprintln("Assets", "Init start");
    assert(!::assets);
    ::assets = this;
    
    this->sprites_cfg_name = Str(sprites_cfg_name);
    FILE *file = fopen(sprites_cfg_name, "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *text = new char[size + 1];
    fread(text, 1, size, file);
    text[size] = 0;
    fclose(file);
    Lexer lexer;
    lexer.init(text, size + 1);
    delete[] text;
    const Token *token = lexer.peek_tok();
    AssetInfo current_info = {};
    bool has_info = false;
    while (!token->is_kind(TokenKind::EOS)) {
        if (token->is_kind('[')) {
            if (has_info) {
                this->asset_infos.set(current_info.name.c_str(), current_info);
            }
            has_info = true;
            token = lexer.peek_next_tok();
            assert(token->is_kind(TokenKind::Identifier));
            current_info.name = token->get_ident();
            token = lexer.peek_next_tok();
            assert(token->is_kind(']'));
            token = lexer.peek_next_tok();
        } else if (token->is_kind(TokenKind::Identifier)) {
            assert(has_info);
            if (token->get_ident().cmp("kind")) {
                token = lexer.peek_next_tok();
                assert(token->is_kind(TokenKind::Identifier));
                if (token->get_ident().cmp("img")) {
                    current_info.kind = AssetKind::Image;
                } else if (token->get_ident().cmp("font")) {
                    current_info.kind = AssetKind::Font;
                } else {
                    assert(false);
                }
                token = lexer.peek_next_tok();
            } else if (token->get_ident().cmp("filename")) {
                token = lexer.peek_next_tok();
                assert(token->is_kind(TokenKind::String));
                current_info.filename = token->get_str();
                token = lexer.peek_next_tok();
            } else if (token->get_ident().cmp("height")) {
                token = lexer.peek_next_tok();
                assert(token->is_kind(TokenKind::Integer));
                current_info.height = token->get_int();
                token = lexer.peek_next_tok();
            } else {
                assert(false);
            }
        } else {
            assert(false);
        }
    }
    if (has_info) {
        this->asset_infos.set(current_info.name.c_str(), current_info);
    }
    lexer.cleanup();
    for (size_t i = 0; i < this->asset_infos.num_entries; ++i) {
        AssetInfo *info = this->asset_infos.get_index(i);
        assert(info);
        logprint("Assets", "Loaded asset info for '%s': ", info->name.c_str());
        print("kind: '%s' ", ASSET_KINDS[(u32)info->kind]);
        if (info->kind == AssetKind::Font) {
            print("filename: '%s' ", info->filename.c_str());
        } else if (info->kind == AssetKind::Image) {
            print("filename: '%s' ", info->filename.c_str());
        }
        print("\n");
    }
    
    // @CLEAN
    this->get_tex("white");
    logprintln("Assets", "Init end");
}

void Assets::cleanup() {
}
    
AssetInfo *Assets::get_info(const char *name) {
    AssetInfo *result = 0;
    bool found = this->asset_infos.get(name, &result);
    assert(found);
    return result;
}

Texture Assets::get_tex(const char *name) {
    AssetInfo *info = this->get_info(name);
    assert(info->kind == AssetKind::Image);
    if (info->state == AssetState::Loaded) {
    } else {
        logprintln("Assets", "Loading texture '%s'", name);
        
        FileHandle file = OS::open_file(info->filename.c_str());
        assert(OS::is_file_handle_valid(file));
        size_t file_size = OS::get_file_size(file);
        void *buffer = Mem::alloc(file_size);
        OS::read_file(file, 0, file_size, buffer);
        
        int w, h;
        void *data = stbi_load_from_memory((const stbi_uc *)buffer, file_size, &w, &h, 0, 4);
        Vec2i tex_size = Vec2i(w, h);
        Mem::free(buffer);
        
        Texture tex = renderer->create_texture(data, tex_size);
        size_t array_idx = this->textures.add(tex);
        Mem::free(data);
        
        info->state = AssetState::Loaded;
        info->array_entry_idx = array_idx;
        logprintln("Assets", "Loaded texture '%s'", name);
    }
    return this->textures[info->array_entry_idx];
}

FontData *Assets::get_font(const char *name) {
    AssetInfo *info = this->get_info(name);
    assert(info->kind == AssetKind::Font);
    FontData *result = 0;
    if (info->state == AssetState::Loaded) {
    } else {
        logprintln("Assets", "Loading font '%s'", name);
         
        FileHandle file = OS::open_file(info->filename.c_str());
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
        
        size_t array_idx = this->fonts.add({});
        FontData *font = &this->fonts[array_idx];
        font->tex = renderer->create_texture(atlas_data, Vec2i(atlas_width, atlas_height));
        delete[] atlas_data;
        
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
        logprintln("Fonts", "Loaded font '%s'", info->filename.c_str());
        info->state = AssetState::Loaded;
        info->array_entry_idx = array_idx;
        logprintln("Assets", "Loaded font '%s'", name);
    }
    return &this->fonts[info->array_entry_idx];
}

Vec2 Assets::get_text_size(const char *name, const char *text, size_t count, f32 scale) {
    AssetInfo *info = this->get_info(name);
    FontData *font = this->get_font(name);
    
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