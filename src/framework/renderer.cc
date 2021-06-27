#include "framework/renderer.hh"
#include "framework/assets.hh"
#include "game/game.hh"

#define GLPROC(_name, _type) \
_type _name;
#include "framework/gl_procs.inc"
#undef GLPROC

#include "framework/renderer_internal.cc"

RendererSetup setup_3d(Mat4x4 mvp) {
    RendererSetup result;
    result.mvp = mvp;
    result.has_depth = true;
    return result;
}

RendererSetup setup_2d(Mat4x4 mvp) {
    RendererSetup result;
    result.mvp = mvp;
    result.has_depth = false;
    return result;
}

// Returns quads that render group should use with given setup.
// This quads are guaranteed to fit at least single quad in it
// We may add additional checks for buffer overflow if some renderer call draws multiple quads while using single get_quads call
static RenderQuads *get_quads(RendererCommands *commands, RendererSetup setup) {
    RenderQuads *quads = 0;
    if (commands->vertex_count + 4 > commands->max_vertex_count || commands->index_count + 6 > commands->max_index_count) {
        logprintln("Renderer", "Commands buffer overflow. I is %llu/%llu, V is %llu/%llu", 
            commands->index_count, commands->max_index_count, commands->vertex_count, commands->max_vertex_count);
    } else {
        if (commands->last_quads && memcmp(&commands->last_quads->setup, &setup, sizeof(setup)) == 0) {
            // Check that index type can fit more indices without overflowing
            size_t base_index = commands->vertex_count - commands->last_quads->vertex_array_offset;
            if (base_index + 6 <= RENDERER_MAX_INDEX) {
                quads = commands->last_quads;
                ++quads->quad_count;
            }
        } 
        
        if (!quads) {
            if (commands->quads_count + 1 > commands->max_quads_count) {
                logprintln("Renderer", "Commands buffer overflow. Quads is %llu/%llu", 
                    commands->quads_count, commands->max_quads_count);
            } else {
                quads = commands->quads + commands->quads_count++;
                quads->index_array_offset = commands->index_count;
                quads->vertex_array_offset = commands->vertex_count;
                quads->quad_count = 1;
                quads->setup = setup;
                commands->last_quads = quads;
            }
        }
    }
    return quads;
}

void push_quad(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
               Vec4 c00, Vec4 c01, Vec4 c10, Vec4 c11,
               Vec2 uv00, Vec2 uv01, Vec2 uv10, Vec2 uv11,
               Texture texture) {
    RenderQuads *quads = get_quads(render_group->commands, render_group->setup);
    if (quads) {
        Vec2 uv_scale = Vec2(texture.width, texture.height) * RENDERER_RECIPROCAL_TEXTURE_SIZE;
        uv00 = uv00 * uv_scale;
        uv01 = uv01 * uv_scale;
        uv10 = uv10 * uv_scale;
        uv11 = uv11 * uv_scale;

        u16 texture_index = (u16)texture.index;
        // Vertex buffer
        assert(render_group->commands->vertex_count + 4 <= render_group->commands->max_vertex_count);
        Vertex *vertex_buffer = render_group->commands->vertices + render_group->commands->vertex_count;
        vertex_buffer[0].p = v00;
        vertex_buffer[0].uv  = uv00;
        vertex_buffer[0].c = c00;
        vertex_buffer[0].tex = texture_index;
        vertex_buffer[1].p = v01;
        vertex_buffer[1].uv  = uv01;
        vertex_buffer[1].c = c01;
        vertex_buffer[1].tex = texture_index;
        vertex_buffer[2].p = v10;
        vertex_buffer[2].uv  = uv10;
        vertex_buffer[2].c = c10;
        vertex_buffer[2].tex = texture_index;
        vertex_buffer[3].p = v11;
        vertex_buffer[3].uv  = uv11;
        vertex_buffer[3].c = c11;
        vertex_buffer[3].tex = texture_index;

        // Index buffer
        assert(render_group->commands->index_count + 6 <= render_group->commands->max_index_count);
        RENDERER_INDEX_TYPE *index_buffer = render_group->commands->indices + render_group->commands->index_count;
        RENDERER_INDEX_TYPE  base_index   = (RENDERER_INDEX_TYPE)(render_group->commands->vertex_count - quads->vertex_array_offset);
        index_buffer[0] = base_index + 0;
        index_buffer[1] = base_index + 2;
        index_buffer[2] = base_index + 3;
        index_buffer[3] = base_index + 0;
        index_buffer[4] = base_index + 1;
        index_buffer[5] = base_index + 3;

        // Update buffer sizes after we are finished.
        render_group->commands->vertex_count += 4;
        render_group->commands->index_count  += 6;
    }
}

void push_quad(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
                             Vec4 c, AssetID texture_id) {
    Texture tex = render_group->assets->get_tex(texture_id);
    push_quad(render_group, v00, v01, v10, v11, c, c, c, c, Vec2(0, 0), Vec2(0, 1), Vec2(1, 0), Vec2(1, 1), tex);
}

void push_quad(RenderGroup *render_group, Vec3 v[4], AssetID texture_id) {
    Texture tex = render_group->assets->get_tex(texture_id);
    push_quad(render_group, v[0], v[1], v[2], v[3], Colors::white, Colors::white, Colors::white, Colors::white,
        Vec2(0, 0), Vec2(0, 1), Vec2(1, 0), Vec2(1, 1), tex);
}

void push_rect(RenderGroup *render_group, Rect rect, Vec4 color, Rect uv_rect, AssetID texture_id) {
    Vec3 v[4]; 
    rect.store_points(v);
    Vec2 uvs[4];
    uv_rect.store_points(uvs);
    Texture tex = render_group->assets->get_tex(texture_id);
    push_quad(render_group, v[0], v[1], v[2], v[3], color, color, color, color, uvs[0], uvs[1], uvs[2], uvs[3], tex);
}

void push_text(RenderGroup *render_group, Vec2 p, Vec4 color, const char *text, AssetID font_id, f32 scale) {
    AssetInfo *info = render_group->assets->get_info(font_id);
    f32 font_height = info->height;
    f32 line_height = font_height * scale;
    FontData *font = render_group->assets->get_font(font_id);
    AssetID texture_id = font->texture_id;

	f32 rwidth  = 1.0f / (f32)font->tex_size.x;
	f32 rheight = 1.0f / (f32)font->tex_size.y;

	Vec3 offset = Vec3(p, 0);
	offset.y += line_height;
    
	for (const char *scan = text; *scan; ++scan) {
		u8 symbol = *scan;
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
            
            push_rect(render_group, Rect(x1, y1, x2 - x1, y2 - y1), color, Rect(s1, t1, s2 - s1, t2 - t1), texture_id);
			f32 char_advance = glyph->x_advance * scale;
			offset.x += char_advance;
		}
	}
}

void push_line(RenderGroup *render_group, Vec3 a, Vec3 b, Vec4 color, f32 thickness) {
    // @TODO not behaving properly when ab is close to parallel with cam_z
    Vec3 cam_z = render_group->setup.mvp.get_z();
    Vec3 line = (b - a);
    line -= cam_z * Math::dot(cam_z, line);
    Vec3 line_perp = Math::cross(line, cam_z);
    line_perp = Math::normalize(line_perp);
    line_perp *= thickness;
    push_quad(render_group, a - line_perp, a + line_perp, b - line_perp, b + line_perp, color);
    
}

void push_quad_outline(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11, Vec4 color, f32 thickness) {
    push_line(render_group, v00, v01, color, thickness);
    push_line(render_group, v01, v11, color, thickness);
    push_line(render_group, v11, v10, color, thickness);
    push_line(render_group, v10, v00, color, thickness);
}

void push_rect_outline(RenderGroup *render_group, Rect rect, Vec4 color, f32 thickness) {
    Vec3 v[4]; 
    rect.store_points(v);
    push_quad_outline(render_group, v[0], v[1], v[2], v[3], color, thickness);
}

RenderGroup render_group_begin(RendererCommands *commands, Assets *assets, RendererSetup setup) {
    RenderGroup result = {};
    result.commands = commands;
    result.assets = assets;
    result.setup = setup;
    return result;
}

void render_group_end(RenderGroup *group) {
    (void)group;
    // group->renderer->has_render_group = false;
}

static GLuint create_shader(const Str &source) {
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
    
    GLuint id = glCreateProgram();
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
    return id;
}


void renderer_init(Renderer *renderer) {
    logprintln("Renderer", "Init start");
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallback(opengl_error_callback, 0);
    
    Str standard_shader_code = R"FOO(#ifdef VERTEX_SHADER       
layout(location = 0) in vec4 position;     
layout(location = 1) in vec2 uv;       
layout(location = 2) in vec3 n;        
layout(location = 3) in vec4 color;        
layout(location = 4) in int texture_index;     
       
out vec4 rect_color;       
out vec2 frag_uv;      
       
flat out int frag_texture_index;       
uniform mat4 mvp = mat4(1);     
void main()        
{      
    vec4 world_space = position;       
    vec4 clip_space = mvp * world_space;        
    gl_Position = clip_space;      
       
    rect_color = color;        
    frag_uv = uv;      
    frag_texture_index = texture_index;        
}      
#else 

float linear1_to_srgb(float l) {
    l = clamp(l, 0, 1);
    float s = l * 12.92f;
    if (l > 0.0031308f)
    {
        s = 1.055f * pow(l, 1.0 / 2.4) - 0.055f;
    }
    return s;
}

in vec4 rect_color;        
in vec2 frag_uv;       
flat in int frag_texture_index;        
uniform sampler2DArray tex;        
out vec4 out_color;        
void main()        
{      
    vec3 array_uv = vec3(frag_uv.x, frag_uv.y, frag_texture_index);        
    vec4 texture_sample = texture(tex, array_uv);      
    if (texture_sample.a == 0) {
        discard;
    } 
    
    out_color = texture_sample * rect_color;       
}      
#endif)FOO";
    renderer->standard_shader = create_shader(standard_shader_code);
    renderer->mvp_location = glGetUniformLocation(renderer->standard_shader, "mvp");
    renderer->tex_location = glGetUniformLocation(renderer->standard_shader, "tex");
   
    size_t max_quads_count = 1024;
    renderer->commands.max_quads_count = max_quads_count;
    renderer->commands.quads = alloc_arr(&renderer->arena, max_quads_count, RenderQuads);
    size_t max_vertex_count = (1 << 16);
    renderer->commands.max_vertex_count = max_vertex_count;
    renderer->commands.vertices = alloc_arr(&renderer->arena, max_vertex_count, Vertex);
    size_t max_index_count = max_vertex_count / 2 * 3;
    renderer->commands.max_index_count = max_index_count;
    renderer->commands.indices = alloc_arr(&renderer->arena, max_index_count, RENDERER_INDEX_TYPE);
   
    glGenVertexArrays(1, &renderer->vertex_array);
    glBindVertexArray(renderer->vertex_array);

    glGenBuffers(1, &renderer->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, renderer->commands.max_vertex_count * sizeof(Vertex), 0, GL_STREAM_DRAW);

    glGenBuffers(1, &renderer->index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, renderer->commands.max_index_count * sizeof(RENDERER_INDEX_TYPE), 0, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, p));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, n));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, c));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_SHORT, sizeof(Vertex), (void *)offsetof(Vertex, tex));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);
    
    renderer->texture_count = 0;
    renderer->max_texture_count = 256;
    glGenTextures(1, &renderer->texture_array);
    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->texture_array);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1,
                   GL_RGBA8,
                   RENDERER_TEXTURE_DIM,
                   RENDERER_TEXTURE_DIM,
                   (GLsizei)renderer->max_texture_count);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    logprintln("Renderer", "Init end");
}

void renderer_cleanup(Renderer *renderer) {
    (void)(renderer);    
}

RendererCommands *renderer_begin_frame(Renderer *renderer, Vec2 display_size, Vec4 clear_color) {
    renderer->statistics = renderer->current_statistics;
    renderer->current_statistics = {};
    renderer->display_size = display_size;
    renderer->clear_color = clear_color;
    RendererCommands *commands = &renderer->commands;
    commands->quads_count = 0;
    commands->vertex_count = 0;
    commands->index_count = 0;
    commands->last_quads = 0;
    return commands;
}

void renderer_end_frame(Renderer *renderer) {
    TIMED_FUNCTION();
    // glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, (GLsizei)renderer->display_size.x, (GLsizei)renderer->display_size.y);
    glScissor(0, 0, (GLsizei)renderer->display_size.x, (GLsizei)renderer->display_size.y);
    // Upload data from vertex array to OpenGL buffer
    glBindVertexArray(renderer->vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vertex_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, renderer->commands.vertex_count * sizeof(Vertex), renderer->commands.vertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->index_buffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, renderer->commands.index_count * sizeof(RENDERER_INDEX_TYPE), renderer->commands.indices);

    glClearColor(renderer->clear_color.r, renderer->clear_color.g,
                 renderer->clear_color.b, renderer->clear_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(renderer->standard_shader);
    for (size_t i = 0; i < renderer->commands.quads_count; ++i) {
        RenderQuads *quads = renderer->commands.quads + i;
        
        if (quads->setup.has_depth) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        glUniformMatrix4fv(renderer->mvp_location, 1, false, quads->setup.mvp.value_ptr());
        glUniform1i(renderer->tex_location, 0); 
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->texture_array);
        glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)(6 * quads->quad_count), GL_INDEX_TYPE,
            (GLvoid *)(sizeof(RENDERER_INDEX_TYPE) * quads->index_array_offset),
            (GLint)quads->vertex_array_offset);
            
        ++renderer->current_statistics.draw_call_count;
    }
}

Texture renderer_create_texture(Renderer *renderer, void *data, Vec2i size) {
    assert(size.x <= RENDERER_TEXTURE_DIM && size.y <= RENDERER_TEXTURE_DIM);
    Texture tex;
    tex.index = (u32)renderer->texture_count++;
    tex.width  = (u16)size.x;
    tex.height = (u16)size.y;
    assert(tex.width == size.x && tex.height == size.y);
    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->texture_array);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0,
                    tex.index, size.x, size.y, 1,
                    GL_RGBA, GL_UNSIGNED_BYTE, data);    
    return tex;
}