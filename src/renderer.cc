#include "renderer.hh"
#include "game.hh"

#include "mips.hh"

#define GLPROC(_name, _type) \
_type _name;
#include "gl_procs.inc"
#undef GLPROC

static GLuint create_shader(const char *source, const char *defines = "") {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const char *const vertex_source[] = { "#version 330\n", "#define VERTEX_SHADER\n", defines, source };
    const char *const fragment_source[] = { "#version 330\n", defines, source };
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

static GLuint get_uniform(GLuint shader, const char *name) {
    GLuint result = glGetUniformLocation(shader, name);
    assert(result != (GLuint)-1);
    return result;
}

struct OpenGLQuadShader {
    GLuint id;
    GLuint view_location;
    GLuint projection_location;
    GLuint tex_location;
    
    bool is_depth_peeling;
    GLuint depth_location;
};

OpenGLQuadShader compile_quad_shader(bool depth_peel) {
    const char *standard_shader_code = 
#include "quad_shader.glsl"
    ;
    
    char defines[256];
    snprintf(defines, sizeof(defines), "#define DEPTH_PEEL %u\n", (u32)TO_BOOL(depth_peel));
    
    OpenGLQuadShader result = {};
    result.is_depth_peeling = depth_peel;
    result.id = create_shader(standard_shader_code, defines);
    result.projection_location = get_uniform(result.id, "projection_matrix");
    result.view_location = get_uniform(result.id, "view_matrix");
    result.tex_location = get_uniform(result.id, "tex");
    if (depth_peel) {
        result.depth_location = get_uniform(result.id, "depth");
    }
    return result;
}

static void bind_shader(OpenGLQuadShader *shader, Mat4x4 *view, Mat4x4 *projection, u32 tex, u32 depth = 1) {
    glUseProgram(shader->id);
    glUniformMatrix4fv(shader->view_location, 1, false, view->value_ptr());
    glUniformMatrix4fv(shader->projection_location, 1, false, projection->value_ptr());
    glUniform1i(shader->tex_location, tex);
    if (shader->is_depth_peeling) {
        glUniform1i(shader->depth_location, depth);
    }
}

struct OpenGLBlitFramebufferShader {
    GLuint id;
    GLuint tex_location;
};

static OpenGLBlitFramebufferShader compile_blit_framebuffer_shader() {
    const char *render_framebuffer_shader_code = 
#include "blit_framebuffer.glsl"
    ;
    OpenGLBlitFramebufferShader result = {};
    result.id = create_shader(render_framebuffer_shader_code);
    result.tex_location = get_uniform(result.id, "tex");
    return result;
}

static void bind_shader(OpenGLBlitFramebufferShader *shader, u32 tex) {
    glUseProgram(shader->id);
    glUniform1i(shader->tex_location, tex);
}

struct OpenGLHorizontalBlurShader {
    GLuint id;
    GLuint tex_location;
    GLuint target_width_location;
};

static OpenGLHorizontalBlurShader compile_horizontal_blur_shader() {
    const char *horizontal_gaussian_blur_shader_code = 
#include "horizontal_gaussian_blur.glsl"
    ;
    OpenGLHorizontalBlurShader result = {};
    result.id = create_shader(horizontal_gaussian_blur_shader_code);
    result.tex_location = get_uniform(result.id, "tex");
    result.target_width_location = get_uniform(result.id, "target_width");
    return result;
}

static void bind_shader(OpenGLHorizontalBlurShader *shader, f32 target_width, u32 tex) {
    glUseProgram(shader->id);
    glUniform1i(shader->tex_location, tex);
    glUniform1f(shader->target_width_location, target_width);
}

struct OpenGLVerticalBlurShader {
    GLuint id;
    GLuint tex_location;
    GLuint target_height_location;
};

static OpenGLVerticalBlurShader compile_vertical_blur_shader() {
    const char *vertical_gaussian_blur_shader_code = 
#include "vertical_gaussian_blur.glsl"
    ;
    OpenGLVerticalBlurShader result = {};
    result.id = create_shader(vertical_gaussian_blur_shader_code);
    result.tex_location = get_uniform(result.id, "tex");
    result.target_height_location = get_uniform(result.id, "target_height");
    return result;
}

static void bind_shader(OpenGLVerticalBlurShader *shader, f32 target_height, u32 tex) {
    glUseProgram(shader->id);
    glUniform1i(shader->tex_location, tex);
    glUniform1f(shader->target_height_location, target_height);
}

struct OpenGLDepthPeelCompositeShader {
    GLuint id;
    GLuint peel0_location;
    GLuint peel1_location;
    GLuint peel2_location;
    GLuint peel3_location;
};

static OpenGLDepthPeelCompositeShader compile_depth_peel_composite() {
    const char *code = 
#include "depth_peel_composite.glsl"
    ;
    OpenGLDepthPeelCompositeShader result = {};
    result.id = create_shader(code);
    result.peel0_location = get_uniform(result.id, "peel0_tex");
    result.peel1_location = get_uniform(result.id, "peel1_tex");
    result.peel2_location = get_uniform(result.id, "peel2_tex");
    result.peel3_location = get_uniform(result.id, "peel3_tex");
    return result;
}

void bind_shader(OpenGLDepthPeelCompositeShader *shader, u32 peel0, u32 peel1, u32 peel2, u32 peel3) {
    glUseProgram(shader->id);
    glUniform1i(shader->peel0_location, peel0);
    glUniform1i(shader->peel1_location, peel1);
    glUniform1i(shader->peel2_location, peel2);
    glUniform1i(shader->peel3_location, peel3);
}

// This is more like description
struct RendererFramebuffer {
    vec2 size;
    u32 video_memory;
    bool has_depth;
};

enum {
    RENDERER_FRAMEBUFFER_MAIN = UINT32_MAX,
    RENDERER_FRAMEBUFFER_SEPARATED = 0,
    RENDERER_FRAMEBUFFER_BLUR1,
    RENDERER_FRAMEBUFFER_BLUR2,
    RENDERER_FRAMEBUFFER_PEEL1,
    RENDERER_FRAMEBUFFER_PEEL2,
    RENDERER_FRAMEBUFFER_PEEL3,
    RENDERER_FRAMEBUFFER_PEEL4,
    RENDERER_FRAMEBUFFER_SENTINEL,
};

struct Renderer {
    MemoryArena arena;
    RendererSettings settings;
    RendererCommands commands;
    
    OpenGLQuadShader quad_shader;
    OpenGLQuadShader depth_peel_shader;
    OpenGLBlitFramebufferShader blit_framebuffer_shader;
    OpenGLVerticalBlurShader vertical_blur_shader;
    OpenGLHorizontalBlurShader horizontal_blur_shader;
    OpenGLDepthPeelCompositeShader depth_peel_composite_shader;
    
    GLuint render_framebuffer_vao;
    GLuint vertex_array;
    GLuint vertex_buffer;
    GLuint index_buffer;
    size_t max_texture_count;
    size_t texture_count;
    GLuint texture_array;
    
    GLuint framebuffer_ids[RENDERER_FRAMEBUFFER_SENTINEL];
    GLuint framebuffer_textures[RENDERER_FRAMEBUFFER_SENTINEL];
    GLuint framebuffer_depths[RENDERER_FRAMEBUFFER_SENTINEL];
    RendererFramebuffer framebuffers[RENDERER_FRAMEBUFFER_SENTINEL];
    
    u64 video_memory_used;
};

static void APIENTRY opengl_error_callback(GLenum source, GLenum type, GLenum id, GLenum severity, GLsizei length,
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
    fprintf(stderr, "OpenGL Error Callback\n<Source: %s, type: %s, Severity: %s, ID: %u>:::\n%s\n", source_str, type_str, severity_str, id, message);
}

static void init_framebuffer(Renderer *renderer, u32 idx,
                             vec2 size, bool has_depth, bool filtered) {
    RendererFramebuffer result = {};
    result.has_depth = has_depth;
    result.size = size;
    
    GLuint id = renderer->framebuffer_ids[idx];
    GLuint tex_id = renderer->framebuffer_textures[idx];
    u32 mem = 0;
    
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    GLenum filter = filtered ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_id, 0);
    mem += size.x * size.y * 4;
    if (has_depth) {
        GLuint depth_id = renderer->framebuffer_depths[idx];
        glBindTexture(GL_TEXTURE_2D, depth_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_id, 0);
        
        mem += size.x * size.y * 3;
    }
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    renderer->video_memory_used += mem;
    result.video_memory = mem;
    renderer->framebuffers[idx] = result;
}

void init_renderer_for_settings(Renderer *renderer, RendererSettings settings) {
    renderer->texture_count = 0;
    GLenum min_filter, mag_filter;
    if (settings.filtered) {
        mag_filter = GL_LINEAR;
        if (settings.mipmapping) {
            min_filter = GL_LINEAR_MIPMAP_LINEAR;
        }  else {
            min_filter = GL_LINEAR;
        }
    } else {
        mag_filter = GL_NEAREST;
        if (settings.mipmapping) {
            min_filter = GL_NEAREST_MIPMAP_NEAREST;
        }  else {
            min_filter = GL_NEAREST;
        }
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->texture_array); 
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    
#if 1
    size_t white_data_size = get_total_size_for_mips(RENDERER_TEXTURE_DIM, RENDERER_TEXTURE_DIM);
    TempMemory white_temp = begin_temp_memory(&renderer->arena);
    void *white_data = alloc(&renderer->arena, white_data_size);
    memset(white_data, 0xFF, white_data_size);
    renderer->commands.white_texture = renderer_create_texture_mipmaps(renderer, white_data, RENDERER_TEXTURE_DIM, RENDERER_TEXTURE_DIM);
    end_temp_memory(white_temp);
#else   
    // ...
    u32 white_data = 0xFFFFFFFF;
    renderer->commands.white_texture = renderer_create_texture_mipmaps(renderer, &white_data, 1, 1);
#endif   
    
    for (u32 i = 0; i < RENDERER_FRAMEBUFFER_SENTINEL; ++i) {
        renderer->video_memory_used -= renderer->framebuffers[i].video_memory;
    }
    
    vec2 size = settings.display_size;
    vec2 blur_size = size * 0.25f;
    b32 filter = settings.filtered;
    init_framebuffer(renderer, RENDERER_FRAMEBUFFER_SEPARATED, size, true, filter);
    init_framebuffer(renderer, RENDERER_FRAMEBUFFER_PEEL4, size, true, filter);
    init_framebuffer(renderer, RENDERER_FRAMEBUFFER_PEEL3, size, true, filter);
    init_framebuffer(renderer, RENDERER_FRAMEBUFFER_PEEL2, size, true, filter);
    init_framebuffer(renderer, RENDERER_FRAMEBUFFER_PEEL1, size, true, filter);
    init_framebuffer(renderer, RENDERER_FRAMEBUFFER_BLUR1, blur_size, false, filter);
    init_framebuffer(renderer, RENDERER_FRAMEBUFFER_BLUR2, blur_size, false, filter);
    
    renderer->settings = settings;
}

Renderer *renderer_init(RendererSettings settings) {
#define RENDERER_ARENA_SIZE MEGABYTES(256)
    Renderer *renderer = bootstrap_alloc_struct(Renderer, arena, RENDERER_ARENA_SIZE);
    
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallback(opengl_error_callback, 0);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
    
#define MAX_QUADS_COUNT MEGABYTES(16)
#define MAX_VERTEX_COUNT (1 << 16)
#define MAX_INDEX_COUNT (MAX_VERTEX_COUNT / 2 * 3)
    renderer->commands.command_memory_size = MAX_QUADS_COUNT;
    renderer->commands.command_memory = (u8 *) alloc(&renderer->arena, MAX_QUADS_COUNT);
    renderer->commands.max_vertex_count = MAX_VERTEX_COUNT;
    renderer->commands.vertices = alloc_arr(&renderer->arena, MAX_VERTEX_COUNT, Vertex);
    renderer->commands.max_index_count = MAX_INDEX_COUNT;
    renderer->commands.indices = alloc_arr(&renderer->arena, MAX_INDEX_COUNT, RENDERER_INDEX_TYPE);
    
    renderer->quad_shader = compile_quad_shader(false);
    renderer->depth_peel_shader = compile_quad_shader(true);
    renderer->blit_framebuffer_shader = compile_blit_framebuffer_shader();
    renderer->horizontal_blur_shader = compile_horizontal_blur_shader();
    renderer->vertical_blur_shader = compile_vertical_blur_shader();
    renderer->depth_peel_composite_shader = compile_depth_peel_composite();
    
    // Generate vertex arrays
    glGenVertexArrays(1, &renderer->render_framebuffer_vao);
    glBindVertexArray(renderer->render_framebuffer_vao);
    GLuint renderer_framebuffer_vbo;
    f32 render_framebuffer_data[] = {
        -1.0f, -1.0f, 
        -1.0f, 1.0f, 
        1.0f, -1.0f, 
        1.0f, 1.0f, 
    };
    glGenBuffers(1, &renderer_framebuffer_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, renderer_framebuffer_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(render_framebuffer_data), render_framebuffer_data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    
    glGenVertexArrays(1, &renderer->vertex_array);
    glBindVertexArray(renderer->vertex_array);
    
    glGenBuffers(1, &renderer->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, renderer->commands.max_vertex_count * sizeof(Vertex), 0, GL_STREAM_DRAW);
    renderer->video_memory_used += renderer->commands.max_vertex_count * sizeof(Vertex);
    
    glGenBuffers(1, &renderer->index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, renderer->commands.max_index_count * sizeof(RENDERER_INDEX_TYPE), 0, GL_STREAM_DRAW);
    renderer->video_memory_used += renderer->commands.max_index_count * sizeof(RENDERER_INDEX_TYPE);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)STRUCT_OFFSET(Vertex, p));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)STRUCT_OFFSET(Vertex, uv));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)STRUCT_OFFSET(Vertex, n));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)STRUCT_OFFSET(Vertex, c));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_SHORT, sizeof(Vertex), (void *)STRUCT_OFFSET(Vertex, tex));
    glEnableVertexAttribArray(4);
    
    glBindVertexArray(0);
    
#define MAX_TEXTURE_COUNT 256
    renderer->max_texture_count = MAX_TEXTURE_COUNT;
    glGenTextures(1, &renderer->texture_array);
    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->texture_array);
    ITERATE(iter, iterate_mips(RENDERER_TEXTURE_DIM, RENDERER_TEXTURE_DIM)) {
        glTexImage3D(GL_TEXTURE_2D_ARRAY, iter.level, GL_RGBA8, 
                     iter.width, iter.height, MAX_TEXTURE_COUNT,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, 0);        
        renderer->video_memory_used += iter.width * iter.height * 4 * renderer->max_texture_count;
    }
    
    glGenFramebuffers(RENDERER_FRAMEBUFFER_SENTINEL, renderer->framebuffer_ids);
    glGenTextures(RENDERER_FRAMEBUFFER_SENTINEL, renderer->framebuffer_textures);
    glGenTextures(1, renderer->framebuffer_depths + RENDERER_FRAMEBUFFER_SEPARATED);
    glGenTextures(1, renderer->framebuffer_depths + RENDERER_FRAMEBUFFER_PEEL1);
    glGenTextures(1, renderer->framebuffer_depths + RENDERER_FRAMEBUFFER_PEEL2);
    glGenTextures(1, renderer->framebuffer_depths + RENDERER_FRAMEBUFFER_PEEL3);
    glGenTextures(1, renderer->framebuffer_depths + RENDERER_FRAMEBUFFER_PEEL4);
    
    init_renderer_for_settings(renderer, settings);
    return renderer;
}

RendererCommands *renderer_begin_frame(Renderer *renderer) {
    RendererCommands *commands = &renderer->commands;
    commands->command_memory_used = 0;
    commands->vertex_count = 0;
    commands->index_count = 0;
    commands->last_header = 0;
    commands->last_setup = 0;
    return commands;
}

static void bind_framebuffer(Renderer *renderer, u32 id, bool clear = false) {
    b32 has_depth = false;
    if (id == RENDERER_FRAMEBUFFER_MAIN) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, renderer->settings.display_size.x, renderer->settings.display_size.y);
        has_depth = true;
    } else {
        RendererFramebuffer *framebuffer = renderer->framebuffers + id;
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->framebuffer_ids[id]);
        glViewport(0, 0, framebuffer->size.x, framebuffer->size.y);
        has_depth = framebuffer->has_depth;
    }
    
    if (clear) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        u32 flags = GL_COLOR_BUFFER_BIT;
        if (has_depth) {
            flags |= GL_DEPTH_BUFFER_BIT;
        }
        glClear(flags);
    }
    
}

static void blit_framebuffer(Renderer *renderer, u32 from, u32 to, bool clear = false) {
    bind_framebuffer(renderer, to, clear);
    
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    bind_shader(&renderer->blit_framebuffer_shader, 0);
    glBindVertexArray(renderer->render_framebuffer_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->framebuffer_textures[from]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void renderer_end_frame(Renderer *renderer) {
    TIMED_FUNCTION();
    // Upload data from vertex array to OpenGL buffers
    glBindVertexArray(renderer->vertex_array);
    glBufferSubData(GL_ARRAY_BUFFER, 0, renderer->commands.vertex_count * sizeof(Vertex), renderer->commands.vertices);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, renderer->commands.index_count * sizeof(RENDERER_INDEX_TYPE), renderer->commands.indices);
    glBindVertexArray(0);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    // This is local renderer state
    RendererSetup *current_setup = 0;
    u32 current_framebuffer = RENDERER_FRAMEBUFFER_MAIN;
    bind_framebuffer(renderer, current_framebuffer, true);
    
    u8 *cursor = renderer->commands.command_memory;
    u8 *commands_bound = renderer->commands.command_memory + renderer->commands.command_memory_used;
    u8 *peel_header_restore = 0;
    u32 peel_count = 0;
    // b32 is_peeling = false;
    
    while (cursor < commands_bound) {
        RendererCommandHeader *header = (RendererCommandHeader *)cursor;
        cursor += sizeof(*header);
        switch (header->type) {
            case RENDERER_COMMAND_QUADS: {
                RendererCommandQuads *quads = (RendererCommandQuads *)cursor;
                cursor += sizeof(*quads);
                
                assert(current_setup);
                if (peel_count) {
                    bind_shader(&renderer->depth_peel_shader, &current_setup->view, &current_setup->projection, 0, 1);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, renderer->framebuffer_depths[RENDERER_FRAMEBUFFER_PEEL1 + peel_count - 1]);
                } else {
                    bind_shader(&renderer->quad_shader, &current_setup->view, &current_setup->projection, 0);
                }
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->texture_array);
                glBindVertexArray(renderer->vertex_array);
                glDrawElementsBaseVertex(GL_TRIANGLES, 6 * quads->quad_count, GL_INDEX_TYPE, (void *)(sizeof(RENDERER_INDEX_TYPE) * quads->index_array_offset), quads->vertex_array_offset);
                glBindVertexArray(0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
                if (peel_count) {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                glUseProgram(0);
            } break;
            case RENDERER_COMMAND_BLUR: {
                assert(current_framebuffer == RENDERER_FRAMEBUFFER_SEPARATED);
                
                glDisable(GL_DEPTH_TEST);
                glDisable(GL_BLEND);
                bind_framebuffer(renderer, RENDERER_FRAMEBUFFER_BLUR1, true);
                bind_shader(&renderer->horizontal_blur_shader, renderer->framebuffers[RENDERER_FRAMEBUFFER_BLUR2].size.x, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, renderer->framebuffer_textures[RENDERER_FRAMEBUFFER_SEPARATED]);
                glBindVertexArray(renderer->render_framebuffer_vao);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(0);
                glBindTexture(GL_TEXTURE_2D, 0);
                glUseProgram(0);
                
                bind_framebuffer(renderer, RENDERER_FRAMEBUFFER_BLUR2, true);
                bind_shader(&renderer->vertical_blur_shader, renderer->framebuffers[RENDERER_FRAMEBUFFER_BLUR2].size.y, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, renderer->framebuffer_textures[RENDERER_FRAMEBUFFER_BLUR1]);
                glBindVertexArray(renderer->render_framebuffer_vao);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(0);
                glBindTexture(GL_TEXTURE_2D, 0);
                glUseProgram(0);
                glEnable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
                
                blit_framebuffer(renderer, RENDERER_FRAMEBUFFER_BLUR2, RENDERER_FRAMEBUFFER_SEPARATED, true);
            } break;
            case RENDERER_COMMAND_BEGIN_SEPARATED: {
                current_framebuffer = RENDERER_FRAMEBUFFER_SEPARATED;
                bind_framebuffer(renderer, current_framebuffer, true);
            } break;
            case RENDERER_COMMAND_END_SEPARATED: {
                current_framebuffer = RENDERER_FRAMEBUFFER_MAIN;
                blit_framebuffer(renderer, RENDERER_FRAMEBUFFER_SEPARATED, RENDERER_FRAMEBUFFER_MAIN);
            } break;
            case RENDERER_COMMAND_SET_SETUP: {
                RendererSetup *setup = (RendererSetup *)cursor;
                cursor += sizeof(*setup);
                
                current_setup = setup;
            } break;
            case RENDERER_COMMAND_BEGIN_DEPTH_PEELING: {
                peel_header_restore = cursor;
                bind_framebuffer(renderer, RENDERER_FRAMEBUFFER_PEEL1, true);
                glDisable(GL_BLEND);
                assert(peel_count == 0);
            } break;
            case RENDERER_COMMAND_END_DEPTH_PEELING: {
                if (peel_count < 3) {
                    cursor = peel_header_restore;
                    ++peel_count;
                    
                    bind_framebuffer(renderer, RENDERER_FRAMEBUFFER_PEEL1 + peel_count, true);
                } else {
                    assert(peel_count == 3);
                    peel_count = 0;
                    
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
                    bind_framebuffer(renderer, current_framebuffer);
                    bind_shader(&renderer->depth_peel_composite_shader, 0, 1, 2, 3);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, renderer->framebuffer_textures[RENDERER_FRAMEBUFFER_PEEL1]);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, renderer->framebuffer_textures[RENDERER_FRAMEBUFFER_PEEL2]);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, renderer->framebuffer_textures[RENDERER_FRAMEBUFFER_PEEL3]);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, renderer->framebuffer_textures[RENDERER_FRAMEBUFFER_PEEL4]);
                    glBindVertexArray(renderer->render_framebuffer_vao);
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    glBindVertexArray(0);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glUseProgram(0);
                    glEnable(GL_BLEND);
                    glEnable(GL_DEPTH_TEST);
                    glDepthMask(GL_TRUE);
                }
            } break;
            INVALID_DEFAULT_CASE;
        }
    }
    
    assert(current_framebuffer == RENDERER_FRAMEBUFFER_MAIN);
    
    {DEBUG_VALUE_BLOCK("Renderer")
            DEBUG_VALUE(renderer->video_memory_used >> 20, "Video memory used");
        DEBUG_VALUE(renderer->texture_count, "Texture count");
        DEBUG_VALUE((f32)renderer->commands.index_count / renderer->commands.max_index_count * 100, "Index buffer");
        DEBUG_VALUE((f32)renderer->commands.vertex_count / renderer->commands.max_vertex_count * 100, "Vertex buffer");
    }
}

Texture renderer_create_texture_mipmaps(Renderer *renderer, void *data, u32 width, u32 height) {
    assert(width <= RENDERER_TEXTURE_DIM && height <= RENDERER_TEXTURE_DIM);
    Texture tex;
    assert(renderer->texture_count + 1 < renderer->max_texture_count);
    tex.index = (u32)renderer->texture_count++;
    tex.width  = (u16)width;
    tex.height = (u16)height;
    assert(tex.width == width && tex.height == height);
    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->texture_array);
    ITERATE(iter, iterate_mips(width, height)) {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, iter.level, 0, 0,
                        tex.index, iter.width, iter.height, 1,
                        GL_RGBA, GL_UNSIGNED_BYTE, (u32 *)data + iter.pixel_offset);    
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    return tex;
}

const RendererSettings *get_current_settings(Renderer *renderer) {
    return &renderer->settings;
}