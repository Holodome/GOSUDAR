#include "renderer/font.hh"

#include "game/game.hh"

#define STB_TRUETYPE_IMPLEMENTATION
#include "thirdparty/stb_truetype.h"

Font::Font(char *filename, f32 height) {
    FILE *file = fopen(filename, "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *text = (char *)malloc(size + 1);
    fread(text, 1, size, file);
    text[size] = 0;
    fclose(file);
    
    const u32 atlas_width  = 512;
	const u32 atlas_height = 512;
	const u32 first_codepoint = 32;
	const u32 codepoint_count = 95;  
    stbtt_packedchar *glyphs = (stbtt_packedchar *)Mem::alloc(codepoint_count * sizeof(stbtt_packedchar));
    
    u8 *loaded_atlas_data = (u8 *)Mem::alloc(atlas_width * atlas_height);
    u8 *atlas_data = (u8 *)Mem::alloc(atlas_width * atlas_height * 4);
    stbtt_pack_context context = {};
	stbtt_PackBegin(&context, loaded_atlas_data, atlas_width, atlas_height, 0, 1, 0);
	stbtt_PackSetOversampling(&context, 2, 2);
	stbtt_PackFontRange(&context, (u8 *)text, 0, height, first_codepoint, codepoint_count, glyphs);
	stbtt_PackEnd(&context);

	for (u32 i = 0; i < atlas_width * atlas_height; ++i) {
		u8 *dest = (u8 *)(atlas_data + i * 4);
		dest[0] = 255;
		dest[1] = 255;
		dest[2] = 255;
		dest[3] = loaded_atlas_data[i];
	}

	atlas = game->renderer.make_texture(atlas_width, atlas_height, atlas_data);

	this->first_codepoint = first_codepoint;
	this->height = height;
    this->glyphs.reserve(codepoint_count);

	for (u32 i = 0; i < codepoint_count; ++i) {
		++this->glyphs.size;
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
}

Vec2 Font::get_text_size(char *text, size_t count, f32 scale) {
    if (!count) {
        count = strlen(text);
    }
    
    Vec2 result = {};
    for (u32 i = 0; i < count; ++i) {
        char s = text[i];
        if (s > first_codepoint && s < (first_codepoint + glyphs.size)) {
            FontGlyph *glyph = &glyphs[s - first_codepoint];
            result.x += glyph->x_advance * scale;
        }
    }
    result.y = height * scale;
    
    return result;
}