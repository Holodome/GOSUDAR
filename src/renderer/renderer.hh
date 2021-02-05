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
    
    bool is_loaded;
    Image(char *name);
    void load();
    void bind();
};

struct ImageLibrary {
    Array<Image> images;
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
    
    bool is_loaded;
    Mesh(char *name);
    void load();
};

struct MeshLibrary {
    Array<Mesh> meshes;
    
    MeshLibrary();
    void init();
    Mesh *get(char *name);
};

struct Renderer {
    ImageLibrary image_lib;
    MeshLibrary mesh_lib;
    GLuint shader;
    
    Renderer();
    void init();      
    
    void begin_frame(Vec2 win_size);
    void draw_mesh(Mesh *mesh, Vec3 p = Vec3(0), Quat4 ori = Quat4::identity(), Vec3 s = Vec3(1));
    void end_frame();
};

#define RENDERER_H 1
#endif
