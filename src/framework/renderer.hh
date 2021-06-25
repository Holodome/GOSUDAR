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
    GLuint id;
    
    static Texture invalid() {
        return { GL_INVALID_ID };
    }
    bool operator==(Texture other) {
        return this->id == other.id;
    }
    bool operator!=(Texture other) {
        return this->id != other.id;
    }
};  

struct RendererStatistics {
    size_t draw_call_count;
    
    void begin_frame() {
        draw_call_count = 0;
    }
};

struct RenderGroup {
    Renderer *renderer;
    struct Assets *assets;
    
    bool has_depth;
    Mat4x4 mvp;
    Mat4x4 imvp;
    Texture texture;
    Shader shader;
};

RenderGroup render_group_begin(Renderer *renderer, Assets *assets, Mat4x4 mvp);
void render_group_set_texture(RenderGroup *group, AssetID texture_id);
void render_group_end(RenderGroup *group);

enum RendererCommand {
    RendererCommand_Clear,
    RendererCommand_Quads
};

struct RendererCommands {
    size_t max_command_buffer_size;
    u8 *push_buffer_at;
    u8 *push_buffer_data_at;
    
    size_t max_vertex_count;
    size_t vertex_count;
    Vertex *vertices;
    
    size_t max_index_count;
    size_t index_count;
    u32 *indices;
};

struct Renderer {
    MemoryArena arena;
    
    Shader standard_shader;
    Shader terrain_shader;
    
    Shader default_shader = Shader::invalid();
    
    bool has_render_group;
    
    GLuint immediate_vao = GL_INVALID_ID;
    GLuint immediate_vbo = GL_INVALID_ID;
    size_t vertex_count;
    size_t max_vertex_count;
    Vertex *vertices;
    
    RendererStatistics current_statistics = {}, statistics = {};
    // @CLEAN
    Texture white_texture;
    
    void init();
    void cleanup();
    
    void begin_frame();
    
    Texture create_texture(void *buffer, Vec2i size);
    
    void clear(Vec4 color);
    void set_draw_region(Vec2 window_size);
    void imm_begin();
    void imm_flush(Shader shader, Texture Texture, Mat4x4 mvp, bool has_depth);
    void imm_vertex(const Vertex &v);
};


void imm_draw_v(RenderGroup *render_group, Vertex vertices[4], AssetID texture_id);
void imm_draw_quad(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
                    Vec4 c00, Vec4 c01, Vec4 c10, Vec4 c11,
                    Vec2 uv00 = Vec2(0, 0), Vec2 uv01 = Vec2(0, 1), Vec2 uv10 = Vec2(1, 0), Vec2 uv11 = Vec2(1, 1),
                    AssetID texture_id = INVALID_ASSET_ID);
void imm_draw_quad(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
                    Vec4 c = Colors::white, AssetID texture_id = INVALID_ASSET_ID);
void imm_draw_quad(RenderGroup *render_group, Vec3 v[4], AssetID texture_id);
void imm_draw_rect(RenderGroup *render_group, Rect rect, Vec4 color, Rect uv_rect = Rect(0, 0, 1, 1), AssetID texture_id = INVALID_ASSET_ID);
void imm_draw_line(RenderGroup *render_group, Vec3 a, Vec3 b, Vec4 color = Colors::white, f32 thickness = 1.0f);
void imm_draw_quad_outline(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11, Vec4 color = Colors::white, f32 thickness = 1.0f);
void imm_draw_rect_outline(RenderGroup *render_group, Rect rect, Vec4 color = Colors::white, f32 thickness = 1.0f);
void imm_draw_text(RenderGroup *render_group, Vec2 p, Vec4 color, const char *text, AssetID font_id, f32 scale);

#define RENDERER_H 1
#endif
