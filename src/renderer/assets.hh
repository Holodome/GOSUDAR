#if !defined(ASSETS_HH)

#include "lib/lib.hh"

// struct RendererTexture {
//     u32 index;
//     u32 w;
//     u32 h;
    
//     bool operator==(RendererTexture other) {
//         return index == other.index && w == other.w && h == other.h;
//     }
// };

// struct FontGlyph {
// 	u32 utf32;
// 	u16 min_x;
// 	u16 min_y;
// 	u16 max_x;
// 	u16 max_y;
// 	f32 offset1_x;
// 	f32 offset1_y;
// 	f32 offset2_x;
// 	f32 offset2_y;
// 	f32 x_advance;
// };

// struct Font {
//     f32 height;
//     u32 first_codepoint;
//     RendererTexture atlas;  
//     Array<FontGlyph> glyphs;

//     Font(char *filename, f32 height);
//     Vec2 get_text_size(char *text, size_t count = 0, f32 scale = 1.0f);
// };

// struct Image {
//     Str name;
//     Vec2i size;
      
// };

#define ASSETS_HH 1
#endif
