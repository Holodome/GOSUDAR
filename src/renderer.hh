#if !defined(RENDERER_H)

#include "lib.hh"

#include "glcorearb.h"

#define GLPROC(_name, _type) \
extern _type _name;
#include "gl_procs.inc"
#undef GLPROC

// Renderer is built the way, so up vector is {0, 1, 0}
// This may be not optimal, bacuause 2.5 requires some 3d space calculations,
// and converting from world plane position to renderer position requires setting 3d.z = 2d.y
// @TODO may be its better to set world up as {0, 0, 1}?

// Just for fun, see if we can use u16 for indices effectively
#define RENDERER_INDEX_TYPE u16
#define RENDERER_MAX_INDEX  VARIABLE_MAX_VALUE(RENDERER_INDEX_TYPE)
#define GL_INDEX_TYPE  (sizeof(RENDERER_INDEX_TYPE) == 4 ? GL_UNSIGNED_INT : sizeof(RENDERER_INDEX_TYPE) == 2 ? GL_UNSIGNED_SHORT : sizeof(RENDERER_INDEX_TYPE) == 1 ? GL_UNSIGNED_BYTE : 0)

struct Vertex {
    Vec3 p;
    Vec2 uv;
    Vec3 n;
    Vec4 c;  
    u16 tex;
};

struct Shader {
    GLuint id;
};

enum {
    RENDERER_COMMAND_NONE,
    RENDERER_COMMAND_QUADS,
    RENDERER_COMMAND_SENTINEL,
};

// Set of settings that defines RenderQuads command.
// So it is per-draw call opengl settings data
struct RendererSetup {
    bool has_depth;
    Mat4x4 view;
    Mat4x4 projection;
    Mat4x4 mvp;
};

inline RendererSetup setup_3d(Mat4x4 view, Mat4x4 projection);
inline RendererSetup setup_2d(Mat4x4 projection);

// Joined draw quads command.
// Each RenderQuads can have as many quads in it as index type can fit indices of
// Renderer specifies that each RenderQuads has its indices going from 0 with using array offset 
// to get actual ones laters, this way we can draw way more indices that single index type can fit
// If more quads don't fill in index type capacity, simply next RenderQuads is created and index goes from 0 again
struct RenderQuads {
    size_t quad_count;
    size_t vertex_array_offset;
    size_t index_array_offset;  
    RendererSetup setup;
};

// Per-frame abstracted renderer interface.
struct RendererCommands {
    size_t max_quads_count;
    size_t quads_count;
    RenderQuads *quads;
    
    size_t max_vertex_count;
    size_t vertex_count;
    Vertex *vertices;
    
    size_t max_index_count;
    size_t index_count;
    RENDERER_INDEX_TYPE *indices;
    
    RenderQuads *last_quads;
    
    Texture white_texture;
};

#define RENDERER_TEXTURE_DIM   512
#define RENDERER_TEXTURE_SIZE Vec2(RENDERER_TEXTURE_DIM, RENDERER_TEXTURE_DIM)
#define RENDERER_RECIPROCAL_TEXTURE_SIZE Vec2(1.0f / RENDERER_TEXTURE_DIM, 1.0f / RENDERER_TEXTURE_DIM)

struct Renderer {
    MemoryArena arena;
    
    GLuint standard_shader;
    GLuint view_location;
    GLuint projection_location;
    GLuint tex_location;
    
    RendererCommands commands;
    
    GLuint vertex_array;
    GLuint vertex_buffer;
    GLuint index_buffer;
    size_t max_texture_count;
    size_t texture_count;
    GLuint texture_array;
    
    Vec2 display_size;
    Vec4 clear_color;
};

void renderer_init(Renderer *renderer);
RendererCommands * renderer_begin_frame(Renderer *renderer, Vec2 winsize, Vec4 clear_color);
void renderer_end_frame(Renderer *renderer);
Texture renderer_create_texture_mipmaps(Renderer *renderer, void *data, u32 width, u32 height);

#define RENDERER_H 1
#endif
