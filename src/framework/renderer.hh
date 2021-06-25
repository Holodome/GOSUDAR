#if !defined(RENDERER_H)

#include "lib/lib.hh"

#include "thirdparty/glcorearb.h"

const static GLuint GL_INVALID_ID = 0;

#define GLPROC(_name, _type) \
extern _type _name;
#include "framework/gl_procs.inc"
#undef GLPROC

// @CLEAN
typedef u32 AssetID;
#define INVALID_ASSET_ID ((AssetID)-1)

struct Vertex {
    Vec3 p;
    Vec2 uv;
    Vec3 n;
    Vec4 c;  
    u16 tex;
};

struct Shader {
    GLuint id;
    
    static Shader invalid() {
        return { GL_INVALID_ID };
    }
    bool operator==(Shader other) {
        return this->id == other.id;
    }
    bool operator!=(Shader other) {
        return this->id != other.id;
    }
};

struct Texture {
    u32 index;
    u16 width;
    u16 height;
};  

static Texture INVALID_TEXTURE = {(u32)(-1), 0, 0 };

struct RendererStatistics {
    size_t draw_call_count;
    
    void begin_frame() {
        draw_call_count = 0;
    }
};

enum {
    RENDERER_COMMAND_NONE,
    RENDERER_COMMAND_QUADS,
    RENDERER_COMMAND_SENTINEL,
};

struct RendererSetup {
    bool has_depth;
    Mat4x4 mvp;
};

inline RendererSetup setup_3d(Mat4x4 mvp) {
    RendererSetup result;
    result.mvp = mvp;
    result.has_depth = true;
    return result;
}

inline RendererSetup setup_2d(Mat4x4 mvp) {
    RendererSetup result;
    result.mvp = mvp;
    result.has_depth = false;
    return result;
}

struct RenderQuads {
    size_t quad_count;
    size_t vertex_array_offset;
    size_t index_array_offset;  
    RendererSetup setup;
};

struct RendererCommands {
    size_t max_quads_count;
    size_t quads_count;
    RenderQuads *quads;
    
    size_t max_vertex_count;
    size_t vertex_count;
    Vertex *vertices;
    
    size_t max_index_count;
    size_t index_count;
    u32 *indices;
    
    RenderQuads *last_quads;
};


struct RenderGroup {
    RendererCommands *commands;
    struct Assets *assets;
    
    RendererSetup setup;  
};

RenderGroup render_group_begin(struct RendererCommands *commands, Assets *assets, RendererSetup setup);
void render_group_end(RenderGroup *group);

#define RENDERER_TEXTURE_DIM 512

struct Renderer {
    MemoryArena arena;
    
    Shader standard_shader;
    
    bool has_render_group;
    
    RendererCommands commands;
    
    GLuint vertex_array;
    GLuint vertex_buffer;
    GLuint index_buffer;
    size_t max_texture_count;
    size_t texture_count;
    GLuint texture_array;
    
    RendererStatistics current_statistics = {}, statistics = {};
    // @CLEAN
    Texture white_texture;
    Vec2 display_size;
    Vec4 clear_color;
};

void renderer_init(Renderer *renderer);
void renderer_cleanup(Renderer *renderer);
RendererCommands * renderer_begin_frame(Renderer *renderer, Vec2 winsize, Vec4 clear_color);
void renderer_end_frame(Renderer *renderer);
Texture renderer_create_texture(Renderer *renderer, void *data, Vec2i size);

void push_quad(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
                    Vec4 c00, Vec4 c01, Vec4 c10, Vec4 c11,
                    Vec2 uv00 = Vec2(0, 0), Vec2 uv01 = Vec2(0, 1), Vec2 uv10 = Vec2(1, 0), Vec2 uv11 = Vec2(1, 1),
                    Texture texture = INVALID_TEXTURE);
void push_quad(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
                    Vec4 c = Colors::white, AssetID texture_id = INVALID_ASSET_ID);
void push_quad(RenderGroup *render_group, Vec3 v[4], AssetID texture_id);
void push_rect(RenderGroup *render_group, Rect rect, Vec4 color, Rect uv_rect = Rect(0, 0, 1, 1), AssetID texture_id = INVALID_ASSET_ID);
void push_line(RenderGroup *render_group, Vec3 a, Vec3 b, Vec4 color = Colors::white, f32 thickness = 1.0f);
void push_quad_outline(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11, Vec4 color = Colors::white, f32 thickness = 1.0f);
void push_rect_outline(RenderGroup *render_group, Rect rect, Vec4 color = Colors::white, f32 thickness = 1.0f);
void push_text(RenderGroup *render_group, Vec2 p, Vec4 color, const char *text, AssetID font_id, f32 scale);

#define RENDERER_H 1
#endif
