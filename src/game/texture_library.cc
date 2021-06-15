#include "game/texture_library.hh"

#define STBI_MALLOC Mem::alloc
#define STBI_REALLOC Mem::realloc
#define STBI_FREE Mem::free
#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"

const char *NO_FILENAME = "NO_FILENAME";

TextureData::TextureData(const char *filename, const char *name) {
    if (name == 0) {
        name = filename;
    }
    
    this->filename = Str(filename);
    this->name = Str(name);
    FILE *file = fopen(filename, "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *text = new char[size + 1];
    fread(text, 1, size, file);
    text[size] = 0;
    fclose(file);
    
    // stbi_set_flip_vertically_on_load(true);
    int w, h;
    this->data = stbi_load(filename, &w, &h, 0, 4);
    this->size = Vec2i(w, h);
    this->texture = Texture(this->data, this->size);
    delete[] text;
}

TextureData::TextureData(const char *name, void *data, Vec2i size) {
    this->name = Str(name);
    this->filename = Str(NO_FILENAME);
    this->data = data;
    this->size = size;
    this->texture = Texture(this->data, this->size);
}

TextureData::~TextureData() {
    Mem::free(data);
}

void TextureLibrary::cleanup() {
    logprintln("TexLib", "Cleanup");
    for (size_t i = 0; i < this->textures.num_entries; ++i) {
        TextureData **data = this->textures.get_index(i);
        assert(data);
        logprintln("TexLib", "Deleting texture '%s'", (*data)->name.data);
        delete (*data);
    }   
}

void TextureLibrary::init() {
    logprintln("TexLib", "Init start");
    // White 
    size_t sizeof_white_tex = 512 * 512 * 4; 
    u8 *white_tex = new u8[sizeof_white_tex];
    memset(white_tex, 0xFF, sizeof_white_tex);
    this->load("white", white_tex, Vec2i(512));
    
    this->default_texture = this->get_tex("white");
    logprintln("TexLib", "Init end");
}

void TextureLibrary::load(const char *filename, const char *name) {
    logprintln("TexLib", "Loaded texture '%s' from file '%s'", name, filename);
    this->textures.set(name, new TextureData(filename, name));       
}

void TextureLibrary::load(const char *name, void *data, Vec2i size) {
    logprintln("TexLib", "Loaded texture '%s' from bin data", name);
    this->textures.set(name, new TextureData(name, data, size));
}

TextureData *TextureLibrary::get(const char *name) {
    TextureData **result = 0;
    bool found = this->textures.get(name, &result);
    assert(found);
    return *result;
}

Texture *TextureLibrary::get_tex(const char *name) {
    TextureData *data = this->get(name);
    return &data->texture;
}