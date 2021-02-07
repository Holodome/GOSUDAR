#if !defined(RENDERER_H)

#include "lib/lib.hh"

#include "renderer/assets.hh"

#include "thirdparty/glcorearb.h"

#define GLPROC(_name, _type) \
static _type _name;
#include "renderer/gl_procs.inc"
#undef GLPROC

const static GLuint GL_INVALID_ID = (GLuint)(0);

struct Vertex {
    Vec3 p;
    Vec2 uv;
    Vec3 n;
    Vec4 c;  
};

struct Image {
    Str name;
    Vec2i size;
    GLuint id;
    
    Image(char *name);
    void bind();
};

struct ImageLibrary {
    Array<Image *> images;
    Image *white;
    
    ImageLibrary();
    void init();
    Image *get(char *name);
};

struct Mesh {
    Str name;
    GLuint vao_id;
    GLuint vbo_id;
    GLuint ibo_id;
    size_t ic;
    
    Mesh(char *name);
    void init(Vertex *v, size_t vc, u32 *i, size_t ic);
    void bind();
};

struct MeshLibrary {
    Array<Mesh *> meshes;
    Mesh *quad;
    
    MeshLibrary();
    void init();
    void add(Mesh *mesh);
    Mesh *get(char *name);
};

struct Renderer {
    ImageLibrary image_lib;
    MeshLibrary mesh_lib;
    GLuint shader;
    Mat4x4 projection, view;
    
    Renderer();
    void init();      
    
    void begin_frame(Vec2 win_size);
    void set_mats(Mat4x4 projection, Mat4x4 view);
    void draw_mesh(Mesh *mesh, Image *diffuse, Vec3 p = Vec3(0), Quat4 ori = Quat4::identity(), Vec3 s = Vec3(1));
    void end_frame();
};

#define RENDERER_H 1
#endif
