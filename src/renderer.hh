#if !defined(RENDERER_H)

#include "lib.hh"

#include "glcorearb.h"

// @TODO investigate bug when lower part of billboard sprites does not pass 
// any of depth peel depth tests

// Renderer is built the way, so up vector is {0, 1, 0}
// This may be not optimal, bacuause 2.5 requires some 3d space calculations,
// and converting from world plane position to renderer position requires setting 3d.z = 2d.y
// @TODO may be its better to set world up as {0, 0, 1}?

// Just for fun, see if we can use u16 for indices effectively
#define RENDERER_INDEX_TYPE u16
#define RENDERER_MAX_INDEX  MAX_VALUE(RENDERER_INDEX_TYPE)
#define GL_INDEX_TYPE  (sizeof(RENDERER_INDEX_TYPE) == 4 ? GL_UNSIGNED_INT : sizeof(RENDERER_INDEX_TYPE) == 2 ? GL_UNSIGNED_SHORT : sizeof(RENDERER_INDEX_TYPE) == 1 ? GL_UNSIGNED_BYTE : 0)

struct Vertex {
    vec3 p;
    vec2 uv;
    vec3 n;
    vec4 c;  
    u16 tex;
};

// Per-frame abstracted renderer interface.
struct RendererCommands {
    uptr command_memory_size;
    uptr command_memory_used;
    u8 *command_memory;
    
    uptr max_vertex_count;
    uptr vertex_count;
    Vertex *vertices;
    
    uptr max_index_count;
    uptr index_count;
    RENDERER_INDEX_TYPE *indices;
    
    struct RendererCommandHeader *last_header;
    struct RendererSetup *last_setup;
    
    Texture white_texture;
};

struct RendererSettings {
    vec2 display_size;  
    bool filtered;
    bool mipmapping;
    bool vsync;
    u32 sample_count;
    
    u32 max_vertex_count;
    u32 max_index_count;
};

#define RENDERER_TEXTURE_DIM   512
#define RENDERER_TEXTURE_SIZE Vec2(RENDERER_TEXTURE_DIM, RENDERER_TEXTURE_DIM)
#define RENDERER_RECIPROCAL_TEXTURE_SIZE Vec2(1.0f / RENDERER_TEXTURE_DIM, 1.0f / RENDERER_TEXTURE_DIM)

struct Renderer; 

Renderer *renderer_init(RendererSettings settings);
void renderer_end_frame(Renderer *renderer, RendererCommands *commands);
// Creates texture from mipmaps data.
Texture renderer_create_texture_mipmaps(Renderer *renderer, void *data, u32 width, u32 height);
// Clean all previous settings and init new
void init_renderer_for_settings(Renderer *renderer, RendererSettings settings);
const RendererSettings *get_current_settings(Renderer *renderer);
// @TODO do already something about this stupidity...
Texture get_white_texture(Renderer *renderer);

#define RENDERER_H 1
#endif
