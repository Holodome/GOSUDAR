#include "renderer/renderer.hh"

#include "game/game.hh"

#define STBI_MALLOC Mem::alloc
#define STBI_REALLOC Mem::realloc
#define STBI_FREE Mem::free
#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"

#define STBTT_malloc(x,u)  ((void)(u),Mem::alloc(x))
#define STBTT_free(x,u)    ((void)(u),Mem::free(x))
#define STB_TRUETYPE_IMPLEMENTATION
#include "thirdparty/stb_truetype.h"

#define GLPROC(_name, _type) \
static _type _name;
#include "renderer/gl_procs.inc"
#undef GLPROC

static void APIENTRY
opengl_error_callback(GLenum source, GLenum type, GLenum id, GLenum severity, GLsizei length,
                      const GLchar* message, const void *_) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

    char *source_str;
    switch(source) {
        case GL_DEBUG_SOURCE_API_ARB: {
		    source_str = "Calls to OpenGL API";
        } break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: {
		    source_str = "Calls to window-system API";
        } break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: {
		    source_str = "A compiler for shading language"; 
        } break;
        case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: {
		    source_str = "Application associated with OpenGL"; 
        } break;
        case GL_DEBUG_SOURCE_APPLICATION_ARB: {
		    source_str = "Generated by user"; 
        } break;
        case GL_DEBUG_SOURCE_OTHER_ARB: {
		    source_str = "Other"; 
        } break;
        default: {
		    source_str = "Unknown"; 
        } break;
    }

    char *type_str;
    switch (type) {
        case GL_DEBUG_TYPE_ERROR_ARB: {
		    type_str = "ERROR"; 
        } break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: {
		    type_str = "DEPRECATED_BEHAVIOR"; 
        } break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: {
		    type_str = "UNDEFINED_BEHAVIOR"; 
        } break;
        case GL_DEBUG_TYPE_PORTABILITY_ARB: {
		    type_str = "PORTABILITY";
        } break;
        case GL_DEBUG_TYPE_PERFORMANCE_ARB: {
		    type_str = "PERFORMANCE"; 
        } break;
        case GL_DEBUG_TYPE_OTHER_ARB: {
		    type_str = "OTHER"; 
        } break;
        default: {
		    type_str = "UNKNOWN"; 
        } break;
    }

    char *severity_str;
    switch(severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION: {
		    severity_str = "NOTIFICATION"; 
        } break;
        case GL_DEBUG_SEVERITY_LOW_ARB: {
		    severity_str = "LOW"; 
        } break;
        case GL_DEBUG_SEVERITY_MEDIUM_ARB: {
		    severity_str = "MEDIUM"; 
        } break;
        case GL_DEBUG_SEVERITY_HIGH_ARB: {
		    severity_str = "HIGH"; 
        } break;
        default: {
		    severity_str = "UNKNOWN"; 
        } break;
    }

    fprintf(stderr, "OpenGL Error Callback\n<Source: %s, type: %s, Severity: %s, ID: %u>:::\n%s\n",
			source_str, type_str, severity_str, id, message);
}

Shader::Shader(const Str &source) {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const char *const vertex_source[] = { "#version 330\n", "#define VERTEX_SHADER\n", source.data };
    const char *const fragment_source[] = { "#version 330\n", "", source.data };
    glShaderSource(vertex_shader, ARRAY_SIZE(vertex_source), vertex_source, 0);
    glShaderSource(fragment_shader, ARRAY_SIZE(fragment_source), fragment_source, 0);
    glCompileShader(vertex_shader);
    glCompileShader(fragment_shader);
    
    GLint vertex_compiled, fragment_compiled;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_compiled);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_compiled);
    
    bool shader_failed = false;
    if (!(vertex_compiled && fragment_compiled)) {
        char shader_log[4096];
        if (!vertex_compiled) {
            glGetShaderInfoLog(vertex_shader, sizeof(shader_log), 0, shader_log);
            fprintf(stderr, "[ERROR] OpenGL vertex shader compilation failed: %s\n", shader_log);
            shader_failed = true;
        }
        if (!fragment_compiled) {
            glGetShaderInfoLog(fragment_shader, sizeof(shader_log), 0, shader_log);
            fprintf(stderr, "[ERROR] OpenGL fragment shader compilation failed: %s\n", shader_log);
            shader_failed = true;
        }
    }
    
    id = glCreateProgram();
    glAttachShader(id, vertex_shader);
    glAttachShader(id, fragment_shader);
    glLinkProgram(id);
    
    GLint link_success;
    glGetProgramiv(id, GL_LINK_STATUS, &link_success);
    if (!link_success) {
        char program_log[4096];
        glGetProgramInfoLog(id, sizeof(program_log), 0, program_log);
        fprintf(stderr, "[ERROR] OpenGL shader compilation failed: %s\n", program_log);
        shader_failed = true;
    }    
    
    assert(!shader_failed);
}

void Shader::bind() {
    glUseProgram(id);
}

Texture::Texture(const void *buffer, Vec2i size) 
    : size(size) {
    stbi_set_flip_vertically_on_load(false);
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(id, 1, GL_RGBA8, size.x, size.y);
    glTextureSubImage2D(id, 0, 0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
}

void Texture::bind(u32 unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

Mesh::Mesh(const Vertex *vertices, size_t vertex_count, const u32 *indices, size_t index_count) {
    this->vertices = new Vertex[vertex_count];
    memcpy(this->vertices, vertices, sizeof(Vertex) * vertex_count);
    this->indices = new u32[index_count];
    memcpy(this->indices, indices, sizeof(u32) * index_count);
    this->vertex_count = vertex_count;
    this->index_count = index_count;
}

Mesh::~Mesh() {
    delete[] vertices;
    delete[] indices;
}


Font::Font(const char *filename, f32 height) {
    FILE *file = fopen(filename, "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *text = new char[size + 1];
    fread(text, 1, size, file);
    text[size] = 0;
    fclose(file);
    
    const u32 atlas_width  = 512;
	const u32 atlas_height = 512;
	const u32 first_codepoint = 32;
	const u32 codepoint_count = 95;  
    stbtt_packedchar *glyphs = new stbtt_packedchar[codepoint_count];
    
    u8 *loaded_atlas_data = new u8[atlas_width * atlas_height];
    stbtt_pack_context context = {};
	stbtt_PackBegin(&context, loaded_atlas_data, atlas_width, atlas_height, 0, 1, 0);
	stbtt_PackSetOversampling(&context, 2, 2);
	stbtt_PackFontRange(&context, (u8 *)text, 0, height, first_codepoint, codepoint_count, glyphs);
	stbtt_PackEnd(&context);

    u8 *atlas_data = new u8[atlas_width * atlas_height * 4];
	for (u32 i = 0; i < atlas_width * atlas_height; ++i) {
		u8 *dest = (u8 *)(atlas_data + i * 4);
		dest[0] = 255;
		dest[1] = 255;
		dest[2] = 255;
		// dest[3] = loaded_atlas_data[i];
		dest[3] = loaded_atlas_data[i];
	}
    delete[] loaded_atlas_data;
    tex = new Texture(atlas_data, Vec2i(atlas_width, atlas_height));
    delete[] atlas_data;
    
	this->first_codepoint = first_codepoint;
	this->size = height;
    this->glyphs.resize(codepoint_count);

	for (u32 i = 0; i < codepoint_count; ++i) {
		++this->glyphs.len;
		this->glyphs[i].utf32 = first_codepoint + i;
		this->glyphs[i].min_x = glyphs[i].x0;
		this->glyphs[i].min_y = glyphs[i].y0;
		this->glyphs[i].max_x = glyphs[i].x1;
		this->glyphs[i].max_y = glyphs[i].y1;
		this->glyphs[i].offset1_x = glyphs[i].xoff;
		this->glyphs[i].offset1_y = glyphs[i].yoff;
		this->glyphs[i].offset2_x = glyphs[i].xoff2;
		this->glyphs[i].offset2_y = glyphs[i].yoff2;
		this->glyphs[i].x_advance = glyphs[i].xadvance;
	}
    delete glyphs;
    delete text;
}

Font::~Font() {
    delete tex;
}

Vec2 Font::get_text_size(const char *text, size_t count, f32 scale) {
    if (!count) {
        count = strlen(text);
    }
    
    Vec2 result = {};
    for (u32 i = 0; i < count; ++i) {
        char s = text[i];
        if (s >= first_codepoint && s < (first_codepoint + glyphs.len)) {
            FontGlyph *glyph = &glyphs[s - first_codepoint];
            // FontGlyph *glyph = &glyphs[first_codepoint];
            result.x += glyph->x_advance * scale;
        }
    }
    result.y = size * scale;
    
    return result;
}

void Renderer::init() {
    logprintln("Renderer", "Init start");
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallback(opengl_error_callback, 0);
    
    Str standard_shader_code = R"FOO(#ifdef VERTEX_SHADER
layout(location = 0) in vec4 p;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 n;
layout(location = 3) in vec4 c;

out vec2 pass_uv;
out vec4 pass_c;
uniform mat4 mvp;

void main() {
    gl_Position = mvp * p;
    pass_uv = uv;
    pass_c = c;
}

#else

out vec4 out_c;

uniform sampler2D tex;

in vec2 pass_uv;
in vec4 pass_c;

void main() {
    //out_c = vec4(1, 0, 1, 0);
    out_c = pass_c * texture(tex, pass_uv);
}

#endif)FOO";
    standard_shader = new Shader(standard_shader_code);
    default_shader = standard_shader;
    Str terrain_shader_code = R"FOO(#ifdef VERTEX_SHADER
layout(location = 0) in vec4 p;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 n;
layout(location = 3) in vec4 c;

flat out vec4 pass_c;
uniform mat4 mvp;

void main() {
    gl_Position = mvp * p;
    pass_c = c;
}
#else 
flat in vec4 pass_c;
out vec4 out_c;

void main() {
    out_c = pass_c;
}
#endif 
)FOO";
    terrain_shader = new Shader(terrain_shader_code);
    
    TempArray<u8> white_tex = TempArray<u8>(512 * 512 * 4);
    memset(white_tex.data, 0xFF, white_tex.mem_size());
    white_texture = new Texture(white_tex.data, Vec2i(512, 512));
    default_texture = white_texture;
    
    // glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glCullFace(GL_BACK);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    logprint("Renderer", "Init end\n");
}

void Renderer::cleanup() {
    delete standard_shader;
    delete terrain_shader;
    delete white_texture;
}

void Renderer::begin_frame() {
    last_frame_statisitcs = statistics;
    statistics.begin_frame();
}

void Renderer::clear(Vec4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);    
}

void Renderer::set_draw_region(Vec2 window_size) {
    glViewport(0, 0, window_size.x, window_size.y);
    glScissor(0, 0, window_size.x, window_size.y);
}

void Renderer::immediate_begin() {
    // vertices.clear();
    // @UNSAFE
    vertices.len = 0;
    if (!IS_GL_VALID_ID(immediate_vao)) {
        glGenVertexArrays(1, &immediate_vao);
        glBindVertexArray(immediate_vao);
        glGenBuffers(1, &immediate_vbo);
    }
}

void Renderer::immediate_flush() {
    if (!vertices.len) { return; }
    
    Shader *shader = current_shader;
    assert(shader);
    shader->bind();
    Mat4x4 mvp = projection_matrix * view_matrix * model_matrix;
    glUniformMatrix4fv(glGetUniformLocation(shader->id, "mvp"), 1, false, mvp.value_ptr());
    Texture *texture = current_texture;
	assert(texture);
    texture->bind();
    glUniform1i(glGetUniformLocation(shader->id, "tex"), 0);
    
    glBindVertexArray(immediate_vao);
    glBindBuffer(GL_ARRAY_BUFFER, immediate_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.len, vertices.data, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, p));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, n));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, c));
    glEnableVertexAttribArray(3);
    
    ++statistics.draw_call_count;
    glDrawArrays(GL_TRIANGLES, 0, vertices.len);
}

void Renderer::immediate_vertex(const Vertex &v) {
    vertices.add(v);
}

void Renderer::set_projview(const Mat4x4 &proj, const Mat4x4 &view) {
    this->projection_matrix = proj;
    this->view_matrix = view;
}

void Renderer::set_model(const Mat4x4 &model) {
    this->model_matrix = model;
}

void Renderer::set_shader(Shader *shader) {
    if (shader == 0) {
        shader = default_shader;
    }
    
    current_shader = shader;
}

void Renderer::set_texture(Texture *texture) {
    if (texture == 0) {
        texture = default_texture;
    }
    current_texture = texture;
}

void Renderer::draw_rect(Rect rect, Vec4 color, Rect uv_rect) {
    Vertex v0, v1, v2, v3;
    v0.p = Vec3(rect.top_left());
    v0.uv = uv_rect.top_left();
    v0.c = color;
    v1.p = Vec3(rect.top_right());
    v1.uv = uv_rect.top_right();
    v1.c = color;
    v2.p = Vec3(rect.bottom_left());
    v2.uv = uv_rect.bottom_left();
    v2.c = color;
    v3.p = Vec3(rect.bottom_right());
    v3.uv = uv_rect.bottom_right();
    v3.c = color;
    
    immediate_vertex(v3);
    immediate_vertex(v1);
    immediate_vertex(v0);
    immediate_vertex(v0);
    immediate_vertex(v2);
    immediate_vertex(v3);
}

void Renderer::draw_mesh(Mesh *mesh) {
    for (size_t i = 0; i < mesh->index_count; i += 3) {
        immediate_vertex(mesh->vertices[mesh->indices[i]]);
        immediate_vertex(mesh->vertices[mesh->indices[i + 1]]);
        immediate_vertex(mesh->vertices[mesh->indices[i + 2]]);
    }
}


void Renderer::draw_text(Vec2 p, Vec4 color, const char *text, Font *font, f32 scale) {
    f32 line_height = font->size * scale;

	f32 rwidth  = 1.0f / (f32)font->tex->size.x;
	f32 rheight = 1.0f / (f32)font->tex->size.y;

	Vec3 offset = Vec3(p, 0);
	offset.y += line_height;
    
    set_shader();
    set_texture(font->tex);
	for (const char *scan = text; *scan; ++scan) {
		char symbol = *scan;

		if ((symbol >= font->first_codepoint) && (symbol < font->first_codepoint + font->glyphs.len)) {
			FontGlyph *glyph = &font->glyphs[symbol - font->first_codepoint];

			f32 glyph_width  = (glyph->offset2_x - glyph->offset1_x) * scale;
			f32 glyph_height = (glyph->offset2_y - glyph->offset1_y) * scale;

			f32 y1 = offset.y + glyph->offset1_y * scale;
			f32 y2 = y1 + glyph_height;
			f32 x1 = offset.x + glyph->offset1_x * scale;
			f32 x2 = x1 + glyph_width;

			f32 s1 = glyph->min_x * rwidth;
			f32 t1 = glyph->min_y * rheight;
			f32 s2 = glyph->max_x * rwidth;
			f32 t2 = glyph->max_y * rheight;
            draw_rect(Rect(x1, y1, x2 - x1, y2 - y1), color, Rect(s1, t1, s2 - s1, t2 - t1));
			f32 char_advance = glyph->x_advance * scale;
			offset.x += char_advance;
		}
	}
}

void Renderer::set_renderering_3d(Mat4x4 proj, Mat4x4 view) {
    set_projview(proj, view);
    glEnable(GL_DEPTH_TEST);    
}

void Renderer::set_renderering_2d(Vec2 winsize) {
    Mat4x4 win_proj = Mat4x4::ortographic_2d(0, game->input.winsize.x, game->input.winsize.y, 0);
    game->renderer.set_projview(win_proj);
    glDisable(GL_DEPTH_TEST);
}
