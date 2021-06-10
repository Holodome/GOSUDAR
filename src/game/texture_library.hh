#if !defined(TEXTURE_LIBRARY_HH)

#include "lib/lib.hh"
#include "renderer/renderer.hh"

const size_t TEXTURE_LIBRARY_CAPACITY = 256;

struct TextureData {
    Str filename;
    Str name;
    void *data;
    Vec2i size;  
    Texture texture;
    
    TextureData(const char *filename, const char *name = 0);
    TextureData(const char *name, void *data, Vec2i size);
    ~TextureData();
};

struct TextureLibrary {
    HashTable<TextureData *> textures = {};
    Texture *default_texture = 0;
    
    TextureLibrary();
    ~TextureLibrary();  

    // Perform load of standart textures
    void init();
    // Perform load from file
    void load(const char *filename, const char *name = 0);
    // Perform load from data, data is made owned by this
    void load(const char *name, void *data, Vec2i size);
    TextureData *get(const char *name);
};

#define TEXTURE_LIBRARY_HH 1
#endif
