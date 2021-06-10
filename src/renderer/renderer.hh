#if !defined(RENDERER_H)

#include "lib/lib.hh"

#include "thirdparty/glcorearb.h"

const static GLuint GL_INVALID_ID = 0;
#define IS_GL_VALID_ID(_id) (_id != GL_INVALID_ID)

#define GLPROC(_name, _type) \
extern _type _name;
#include "renderer/gl_procs.inc"
#undef GLPROC

typedef u32 Index;

struct Vertex {
    Vec3 p = Vec3(0);
    Vec2 uv = Vec2(0);
    Vec3 n = Vec3(0);
    Vec4 c = Vec4(1);  
};

struct Shader {
    GLuint id = GL_INVALID_ID;
    
    Shader(const Str &source);
    void bind();
};

struct Texture {
    GLuint id = GL_INVALID_ID;
    Vec2i size = Vec2i(0);
    
    Texture() = default;
    Texture(const void *buffer, Vec2i size);
    void bind(u32 unit = 0) const;
    
    bool is_valid() const {
        return id != GL_INVALID_ID;
    }
};  

struct FontGlyph {
	u32 utf32;
	u16 min_x;
	u16 min_y;
	u16 max_x;
	u16 max_y;
	f32 offset1_x;
	f32 offset1_y;
	f32 offset2_x;
	f32 offset2_y;
	f32 x_advance;
};

struct Font {
    f32 size;
    Texture *tex;  
    Array<FontGlyph> glyphs;
    u32 first_codepoint;
    
    Font(const char *filename, f32 size); 
    ~Font();
    Vec2 get_text_size(const char *text, size_t count = 0, f32 scale = 1.0f);   
};

struct Mesh {
    Vertex *vertices = 0;
    size_t vertex_count = 0;   
    Index *indices = 0;
    size_t index_count = 0;
    
    Mesh(const Vertex *vertices, size_t vertex_count, const Index *indices, size_t index_count);
    ~Mesh();
};


struct RendererStatistics {
    size_t draw_call_count = 0;
    
    void begin_frame() {
        draw_call_count = 0;
    }
};

struct Renderer {
    Shader *standard_shader = 0;
    Shader *terrain_shader = 0;
    
    Shader *default_shader = 0;
    
    Texture *current_texture = 0;
    Shader *current_shader = 0;
    GLuint immediate_vao = GL_INVALID_ID;
    GLuint immediate_vbo = GL_INVALID_ID;
    Array<Vertex> vertices = {};
    
    Mat4x4 view_matrix       = Mat4x4::identity();
    Mat4x4 projection_matrix = Mat4x4::identity();
    Mat4x4 model_matrix      = Mat4x4::identity();
    
    RendererStatistics last_frame_statisitcs = {}, statistics = {};
    
    void init();
    void cleanup();
    
    void begin_frame();
    
    void clear(Vec4 color);
    void set_draw_region(Vec2 window_size);
    void immediate_begin();
    void immediate_flush();
    void immediate_vertex(const Vertex &v);
    void set_projview(const Mat4x4 &proj = Mat4x4::identity(), const Mat4x4 &view = Mat4x4::identity());
    void set_model(const Mat4x4 &model = Mat4x4::identity());
    void set_shader(Shader *shader = 0);
    void set_texture(Texture *texture = 0);
    
    void set_renderering_3d(Mat4x4 proj, Mat4x4 view);
    void set_renderering_2d(Vec2 winsize);
    
    void draw_rect(Rect rect, Vec4 color, Rect uv_rect = Rect(0, 0, 1, 1));
    void draw_mesh(Mesh *mesh);
    void draw_text(Vec2 p, Vec4 color, const char *text, Font *font, f32 scale = 1.0f);
};

#define RENDERER_H 1
#endif
