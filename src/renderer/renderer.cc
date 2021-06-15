#include "renderer/renderer.hh"

#include "game/game.hh"

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
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(id, 1, GL_RGBA8, size.x, size.y);
    glTextureSubImage2D(id, 0, 0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
}

void Texture::bind(u32 unit) const {
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
   
    // glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    logprint("Renderer", "Init end\n");
}

void Renderer::cleanup() {
    delete standard_shader;
    delete terrain_shader;
}

void Renderer::begin_frame() {
    statistics = current_statistics;
    current_statistics.begin_frame();
}

void Renderer::clear(Vec4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);    
}

void Renderer::set_draw_region(Vec2 window_size) {
    glViewport(0, 0, window_size.x, window_size.y);
    glScissor(0, 0, window_size.x, window_size.y);
}

void Renderer::imm_begin() {
    // vertices.clear();
    // @UNSAFE
    vertices.len = 0;
    this->current_shader = this->default_shader;
    if (immediate_vao == GL_INVALID_ID) {
        glGenVertexArrays(1, &immediate_vao);
        glBindVertexArray(immediate_vao);
        glGenBuffers(1, &immediate_vbo);
    }
}

void Renderer::imm_flush() {
    if (!vertices.len) { return; }
    
    Shader *shader = current_shader;
    assert(shader);
    shader->bind();
    glUniformMatrix4fv(glGetUniformLocation(shader->id, "mvp"), 1, false, this->mvp.value_ptr());
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
    
    ++current_statistics.draw_call_count;
    glDrawArrays(GL_TRIANGLES, 0, vertices.len);
}

void Renderer::imm_vertex(const Vertex &v) {
    vertices.add(v);
}

void Renderer::set_mvp(const Mat4x4 &mvp) {
    this->mvp = mvp;
    this->imvp = Mat4x4::inverse(this->mvp);
}

void Renderer::set_shader(Shader *shader) {
    if (shader == 0) {
        shader = this->default_shader;
    }
    
    this->current_shader = shader;
}

void Renderer::set_texture(Texture *texture) {
    if (texture == 0) {
        texture = game->tex_lib.default_texture;
    }
    this->current_texture = texture;
}

void Renderer::imm_draw_quad(Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
                             Vec4 c00, Vec4 c01, Vec4 c10, Vec4 c11,
                             Vec2 uv00, Vec2 uv01, Vec2 uv10, Vec2 uv11,
                             Texture *texture) {
    this->set_texture(texture);
    this->imm_begin();
    Vertex v0, v1, v2, v3;
    v0.p = v00;
    v0.uv = uv00;
    v0.c = c00;
    v1.p = v01;
    v1.uv = uv01;
    v1.c = c01;
    v2.p = v10;
    v2.uv = uv10;
    v2.c = c10;
    v3.p = v11;
    v3.uv = uv11;
    v3.c = c11;
    this->imm_vertex(v3);
    this->imm_vertex(v1);
    this->imm_vertex(v0);
    this->imm_vertex(v0);
    this->imm_vertex(v2);
    this->imm_vertex(v3);
    this->imm_flush();                  
}

void Renderer::imm_draw_quad(Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
                             Vec4 c, Texture *texture) {
    this->imm_draw_quad(v00, v01, v10, v11, c, c, c, c, Vec2(0, 0), Vec2(0, 1), Vec2(1, 0), Vec2(1, 1), texture);
}

void Renderer::imm_draw_rect(Rect rect, Vec4 color, Rect uv_rect, Texture *texture) {
    Vec3 v[4]; 
    rect.store_points(v);
    Vec2 uvs[4];
    uv_rect.store_points(uvs);
    this->imm_draw_quad(v[0], v[1], v[2], v[3], color, color, color, color, uvs[0], uvs[1], uvs[2], uvs[3], texture);
}

// void Renderer::imm_draw_text(Vec2 p, Vec4 color, const char *text, Font *font, f32 scale) {
//     f32 line_height = font->size * scale;

// 	f32 rwidth  = 1.0f / (f32)font->tex->size.x;
// 	f32 rheight = 1.0f / (f32)font->tex->size.y;

// 	Vec3 offset = Vec3(p, 0);
// 	offset.y += line_height;
    
//     set_shader();
// 	for (const char *scan = text; *scan; ++scan) {
// 		char symbol = *scan;

// 		if ((symbol >= font->first_codepoint) && (symbol < font->first_codepoint + font->glyphs.len)) {
// 			FontGlyph *glyph = &font->glyphs[symbol - font->first_codepoint];

// 			f32 glyph_width  = (glyph->offset2_x - glyph->offset1_x) * scale;
// 			f32 glyph_height = (glyph->offset2_y - glyph->offset1_y) * scale;

// 			f32 y1 = offset.y + glyph->offset1_y * scale;
// 			f32 y2 = y1 + glyph_height;
// 			f32 x1 = offset.x + glyph->offset1_x * scale;
// 			f32 x2 = x1 + glyph_width;

// 			f32 s1 = glyph->min_x * rwidth;
// 			f32 t1 = glyph->min_y * rheight;
// 			f32 s2 = glyph->max_x * rwidth;
// 			f32 t2 = glyph->max_y * rheight;
//             this->imm_draw_rect(Rect(x1, y1, x2 - x1, y2 - y1), color, Rect(s1, t1, s2 - s1, t2 - t1), font->tex);
// 			f32 char_advance = glyph->x_advance * scale;
// 			offset.x += char_advance;
// 		}
// 	}
// }

void Renderer::set_renderering_3d(const Mat4x4 &mvp) {
    this->set_mvp(mvp);
    glEnable(GL_DEPTH_TEST);    
}

void Renderer::set_renderering_2d(Vec2 winsize) {
    Mat4x4 win_proj = Mat4x4::ortographic_2d(0, game->input.winsize.x, game->input.winsize.y, 0);
    game->renderer.set_mvp(win_proj);
    glDisable(GL_DEPTH_TEST);
}

void Renderer::imm_draw_line(Vec3 a, Vec3 b, Vec4 color, f32 thickness) {
    // @TODO not behaving properly when ab is close to parallel with cam_z
    // Vec3 cam_z = this->imvp.v[2].xyz;
    Vec3 cam_z = this->mvp.get_z();
    Vec3 line = (b - a);
    line -= cam_z * Math::dot(cam_z, line);
    Vec3 line_perp = Math::cross(line, cam_z);
    // Vec3 other_perp = Math::cross(line_perp, line);
    line_perp = Math::normalize(line_perp);
    // other_perp = Math::normalize(other_perp);
    line_perp *= thickness;
    // other_perp *= thickness;
    this->imm_draw_quad(a - line_perp, a + line_perp, b - line_perp, b + line_perp, color);
    // this->imm_draw_quad(a - other_perp, a + other_perp, b - other_perp, b + other_perp, color);
    
}

void Renderer::imm_draw_quad_outline(Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11, Vec4 color, f32 thickness) {
    this->imm_draw_line(v00, v01, color, thickness);
    this->imm_draw_line(v01, v11, color, thickness);
    this->imm_draw_line(v11, v10, color, thickness);
    this->imm_draw_line(v10, v00, color, thickness);
}

void Renderer::imm_draw_rect_outline(Rect rect, Vec4 color, f32 thickness) {
    Vec3 v[4]; 
    rect.store_points(v);
    this->imm_draw_quad_outline(v[0], v[1], v[2], v[3], color, thickness);
}