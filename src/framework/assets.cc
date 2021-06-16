#include "framework/assets.hh"

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

TextureData::TextureData(const char *filename) {
    this->filename = Str(filename);
    FILE *file = fopen(filename, "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *text = new char[size + 1];
    fread(text, 1, size, file);
    text[size] = 0;
    fclose(file);
    
    // stbi_set_flip_vertically_on_load(true);
    int w, h;
    this->data = stbi_load(filename, &w, &h, 0, 4);
    this->size = Vec2i(w, h);
    this->texture = Texture(this->data, this->size);
    delete[] text;
}

TextureData::TextureData(void *data, Vec2i size) {
    this->filename;
    this->data = data;
    this->size = size;
    this->texture = Texture(this->data, this->size);
}

TextureData::~TextureData() {
    Mem::free(data);
}

FontData::FontData(const char *filename, f32 height) {
    FILE *file = fopen(filename, "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *text = new char[size + 1];
    fread(text, 1, size, file);
    text[size] = 0;
    fclose(file);
    
    const u32 atlas_width  = 512;
	const u32 atlas_height = 512;
	const u32 first_codepoint = 32;
	const u32 codepoint_count = 95;  
    stbtt_packedchar *glyphs = new stbtt_packedchar[codepoint_count];
    
    u8 *loaded_atlas_data = new u8[atlas_width * atlas_height];
    stbtt_pack_context context = {};
	stbtt_PackBegin(&context, loaded_atlas_data, atlas_width, atlas_height, 0, 1, 0);
	stbtt_PackSetOversampling(&context, 2, 2);
	stbtt_PackFontRange(&context, (u8 *)text, 0, height, first_codepoint, codepoint_count, glyphs);
	stbtt_PackEnd(&context);

    u8 *atlas_data = new u8[atlas_width * atlas_height * 4];
	for (u32 i = 0; i < atlas_width * atlas_height; ++i) {
		u8 *dest = (u8 *)(atlas_data + i * 4);
		dest[0] = 255;
		dest[1] = 255;
		dest[2] = 255;
		// dest[3] = loaded_atlas_data[i];
		dest[3] = loaded_atlas_data[i];
	}
    delete[] loaded_atlas_data;
    tex = new Texture(atlas_data, Vec2i(atlas_width, atlas_height));
    delete[] atlas_data;
    
	this->first_codepoint = first_codepoint;
	this->size = height;
    this->glyphs.resize(codepoint_count);

	for (u32 i = 0; i < codepoint_count; ++i) {
		++this->glyphs.len;
		this->glyphs[i].utf32 = first_codepoint + i;
		this->glyphs[i].min_x = glyphs[i].x0;
		this->glyphs[i].min_y = glyphs[i].y0;
		this->glyphs[i].max_x = glyphs[i].x1;
		this->glyphs[i].max_y = glyphs[i].y1;
		this->glyphs[i].offset1_x = glyphs[i].xoff;
		this->glyphs[i].offset1_y = glyphs[i].yoff;
		this->glyphs[i].offset2_x = glyphs[i].xoff2;
		this->glyphs[i].offset2_y = glyphs[i].yoff2;
		this->glyphs[i].x_advance = glyphs[i].xadvance;
	}
    delete glyphs;
    delete text;
    logprintln("Fonts", "Loaded font '%s'", filename);
}

FontData::~FontData() {
    delete tex;
}

Vec2 FontData::get_text_size(const char *text, size_t count, f32 scale) {
    if (!count) {
        count = strlen(text);
    }
    
    Vec2 result = {};
    for (u32 i = 0; i < count; ++i) {
        char s = text[i];
        if (s >= first_codepoint && s < (first_codepoint + glyphs.len)) {
            FontGlyph *glyph = &glyphs[s - first_codepoint];
            // FontGlyph *glyph = &glyphs[first_codepoint];
            result.x += glyph->x_advance * scale;
        }
    }
    result.y = size * scale;
    
    return result;
}

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
    Token *token = lexer.peek_tok();
    AssetInfo current_info = {};
    bool has_info = false;
    printf("here\n");
    while (token->kind != TokenKind::EOS) {
        if ((u8)token->kind == '[') {
            if (has_info) {
                this->asset_infos.set(current_info.name.data, current_info);
            }
            has_info = true;
            lexer.eat_tok();
            token = lexer.peek_tok();
            assert(token->kind == TokenKind::Identifier);
            current_info.name = token->ident;
            lexer.eat_tok();
            token = lexer.peek_tok();
            assert((u8)token->kind == ']');
            lexer.eat_tok();
            token = lexer.peek_tok();
        } else if (token->kind == TokenKind::Identifier) {
            assert(has_info);
            if (token->ident.cmp("kind")) {
                lexer.eat_tok();
                token = lexer.peek_tok();
                assert((u8)token->kind == '=');
                lexer.eat_tok();
                token = lexer.peek_tok();
                assert(token->kind == TokenKind::Identifier);
                if (token->ident.cmp("img")) {
                    current_info.kind = AssetKind::Image;
                } else if (token->ident.cmp("font")) {
                    current_info.kind = AssetKind::Font;
                } else {
                    assert(false);
                }
                lexer.eat_tok();
                token = lexer.peek_tok();
            } else if (token->ident.cmp("filename")) {
                lexer.eat_tok();
                token = lexer.peek_tok();
                assert((u8)token->kind == '=');
                lexer.eat_tok();
                token = lexer.peek_tok();
                assert(token->kind == TokenKind::String);
                current_info.filename = token->string;
                lexer.eat_tok();
                token = lexer.peek_tok();
            } else if (token->ident.cmp("size")) {
                lexer.eat_tok();
                token = lexer.peek_tok();
                assert((u8)token->kind == '=');
                lexer.eat_tok();
                token = lexer.peek_tok();
                // @TODO
            } else {
                assert(false);
            }
        } else {
            assert(false);
        }
    }
    if (has_info) {
        this->asset_infos.set(current_info.name.data, current_info);
    }
    lexer.cleanup();
    printf("here\n");
    for (size_t i = 0; i < this->asset_infos.num_entries; ++i) {
        AssetInfo *info = this->asset_infos.get_index(i);
        assert(info);
        logprint("Assets", "Loaded asset info for '%s': ", info->name.data);
        print("kind: '%s' ", ASSET_KINDS[(u32)info->kind]);
        if (info->kind == AssetKind::Font) {
            print("filename: '%s' ", info->filename.data);
        } else if (info->kind == AssetKind::Image) {
            print("filename: '%s' ", info->filename.data);
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

TextureData *Assets::get_tex_data(const char *name) {
    AssetInfo *info = this->get_info(name);
    assert(info->kind == AssetKind::Image);
    if (info->state == AssetState::Loaded) {
    } else {
        logprintln("Assets", "Loading texture '%s'", name);
        size_t idx = this->texture_datas.add(new TextureData(info->filename.data));
        info->state = AssetState::Loaded;
        info->array_entry_idx = idx;
        logprintln("Assets", "Loaded texture '%s'", name);
    }
    return this->texture_datas[info->array_entry_idx];
}

Texture *Assets::get_tex(const char *name) {
    return &this->get_tex_data(name)->texture;
}

FontData *Assets::get_font(const char *name) {
    AssetInfo *info = this->get_info(name);
    assert(info->kind == AssetKind::Font);
    FontData *result = 0;
    if (info->state == AssetState::Loaded) {
    } else {
        logprintln("Assets", "Loading font '%s'", name);
        size_t idx = this->font_datas.add(new FontData(info->filename.data, 32));
        info->state = AssetState::Loaded;
        info->array_entry_idx = idx;
        logprintln("Assets", "Loaded font '%s'", name);
    }
    return this->font_datas[info->array_entry_idx];
}
