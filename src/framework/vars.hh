#if !defined(VARS_HH)

#include "lib/lib.hh"

enum struct VarKind {
    Bool, 
    F32,
    Vec2,
    Vec3,
    Vec4,
};

struct Var {
    VarKind kind;
    Str name;
    Str value;
    union {
        void *data;
        bool *_bool;
        f32  *_f32;
        Vec2 *_vec2;
        Vec3 *_vec3;
        Vec4 *_vec4;
    };
};

struct Vars {
    Str filename;    
    HashTable<Var> storage;
    
    void check_for_updates();
    
    void add(const char *name, bool *value);
    void add(const char *name, f32 *value);
    void add(const char *name, Vec2 *value);
    void add(const char *name, Vec3 *value);
    void add(const char *name, Vec4 *value);
};

extern Vars *vars;

#define VARS_HH 1
#endif
