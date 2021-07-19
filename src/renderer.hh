#if !defined(RENDERER_H)

#include "lib.hh"

#include "glcorearb.h"

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
    size_t command_memory_size;
    size_t command_memory_used;
    u8 *command_memory;
    
    size_t max_vertex_count;
    size_t vertex_count;
    Vertex *vertices;
    
    size_t max_index_count;
    size_t index_count;
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
};

#define RENDERER_TEXTURE_DIM   512
#define RENDERER_TEXTURE_SIZE Vec2(RENDERER_TEXTURE_DIM, RENDERER_TEXTURE_DIM)
#define RENDERER_RECIPROCAL_TEXTURE_SIZE Vec2(1.0f / RENDERER_TEXTURE_DIM, 1.0f / RENDERER_TEXTURE_DIM)

struct Renderer; 

Renderer *renderer_init(RendererSettings settings);
RendererCommands *renderer_begin_frame(Renderer *renderer);
void renderer_end_frame(Renderer *renderer);
// Creates texture from mipmaps data.
// @TODO see if we need to zero-initialize all mipmap levels to transparent so 
// filtering works properly - maybe we can just avoid using textures of non-standart sizes?
Texture renderer_create_texture_mipmaps(Renderer *renderer, void *data, u32 width, u32 height);
// Clean all previous settings and init new
void init_renderer_for_settings(Renderer *renderer, RendererSettings settings);

RendererSettings *get_current_settings(Renderer *renderer);

#define RENDERER_H 1
#endif
