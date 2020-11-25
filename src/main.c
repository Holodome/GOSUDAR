#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <glcorearb.h>
#include <wgl.h>
#include <wglext.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof(*(_a)))

typedef struct {
    PFNWGLCREATECONTEXTPROC    wglCreateContext;
    PFNWGLDELETECONTEXTPROC    wglDeleteContext;
    PFNWGLGETPROCADDRESSPROC   wglGetProcAddress;
    PFNWGLGETCURRENTDCPROC     wglGetCurrentDC;
    PFNWGLMAKECURRENTPROC      wglMakeCurrent;
    PFNWGLSWAPLAYERBUFFERSPROC wglSwapLayerBuffers;
    
    PFNWGLCHOOSEPIXELFORMATARBPROC    wglChoosePixelFormatARB;
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
	PFNWGLMAKECONTEXTCURRENTARBPROC   wglMakeContextCurrentARB;
	PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT;
	PFNWGLGETSWAPINTERVALEXTPROC      wglGetSwapIntervalEXT;
} WGL;

static bool is_running = true;
static WGL wgl;

#define GLPROC(_name, _type) \
static _type _name;
#include "gl_procs.inc"
#undef GLPROC

#define HALF_PI 1.57079632679f
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define DEG2RAD(_deg) ((_deg) * PI / 180.0f)

inline f32
rsqrtf(f32 a) {
    return 1.0f / sqrtf(a);
}

inline f32 
clamp(f32 a, f32 low, f32 high) {
    return (a < low ? low : a > high ? high : a);
}

inline f32 
unwind_rad(f32 rad) {
    // return fmodf(rad, TWO_PI);
    while (rad > TWO_PI) {
        rad -= TWO_PI;
    }
    while (rad < 0) {
        rad += TWO_PI;
    }
    return rad;
}


typedef union {
    struct {
        f32 x, y;
    };
    f32 e[2];
} Vec2;

inline Vec2 
vec2_neg(Vec2 a) {
    Vec2 result;
    result.x = -a.x;
    result.y = -a.y;
    return result;
}

inline Vec2 
vec2_add(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

inline Vec2 
vec2_sub(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

inline Vec2 
vec2_div(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    return result;
}

inline Vec2 
vec2_mul(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

inline Vec2
vec2(f32 x, f32 y) {
    Vec2 result;
    result.x = x;
    result.y = y;
    return result;
}

inline Vec2
vec2s(f32 s) {
    Vec2 result;
    result.x = s;
    result.y = s;
    return result;
}

typedef union {
    struct {
        f32 x, y, z;   
    }; 
    Vec2 xy;
    f32 e[3];
} Vec3;

inline Vec3 
vec3_neg(Vec3 a) {
    Vec3 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    return result;
}

inline Vec3 
vec3_add(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

inline Vec3 
vec3_sub(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

inline Vec3 
vec3_div(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    return result;
}

inline Vec3 
vec3_mul(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}

inline Vec3 
vec3(f32 x, f32 y, f32 z) {
    Vec3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

inline Vec3 
vec3s(f32 s) {
    Vec3 result;
    result.x = s;
    result.y = s;
    result.z = s;
    return result;
}

typedef union {
    struct {
        f32 x, y, z, w;
    };
    Vec3 xyz;
    Vec2 xy;
    f32 e[4];
} Vec4;

inline Vec4 
vec4_neg(Vec4 a) {
    Vec4 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = -a.w;
    return result;
}

inline Vec4 
vec4_add(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

inline Vec4 
vec4_sub(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}

inline Vec4 
vec4_div(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    result.w = a.w / b.w;
    return result;
}

inline Vec4 
vec4_mul(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    return result;
}

inline Vec4 
vec4(f32 x, f32 y, f32 z, f32 w) {
    Vec4 result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return result;
}

inline Vec4 
vec4s(f32 s) {
    Vec4 result;
    result.x = s;
    result.y = s;
    result.z = s;
    result.w = s;
    return result;
}

inline f32
vec3_dot_product(Vec3 a, Vec3 b) {
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}
inline Vec3
vec3_normalize(Vec3 v) {
    Vec3 result = vec3_mul(v, vec3s(rsqrtf(vec3_dot_product(v, v))));
    return result;
}

inline Vec4
vec4_from_vec2(Vec2 xy, f32 z, f32 w) {
    Vec4 result;
    result.xy = xy;
    result.z = z;
    result.w = w;
    return result;
}

inline Vec3
vec3_from_vec2(Vec2 xy, f32 z) {
    Vec3 result;
    result.xy = xy;
    result.z = z;
    return result;
}

#define COLOR_RED vec4(1, 0, 0, 1)
#define COLOR_BLUE vec4(0, 1, 0, 1)
#define COLOR_GREEN vec4(0, 0, 1, 1)
#define COLOR_YELLOW vec4(1, 1, 0, 1)

typedef union {
    f32  e[4][4];
    Vec4 v[4];
    struct {
        f32 m00; f32 m01; f32 m02; f32 m03;
        f32 m10; f32 m11; f32 m12; f32 m13;
        f32 m20; f32 m21; f32 m22; f32 m23;
        f32 m30; f32 m31; f32 m32; f32 m33;
    };
} Mat4x4;

const Mat4x4 MAT4X4_IDENTITIY = { 
    .m00 = 1,
    .m11 = 1,
    .m22 = 1,
    .m33 = 1
};

inline Mat4x4
mat4x4_translate(Vec3 t) {
	Mat4x4 result = MAT4X4_IDENTITIY;
    result.e[3][0] = t.x;
    result.e[3][1] = t.y;
    result.e[3][2] = t.z;
	return result;
}

inline Mat4x4
mat4x4_scale(Vec3 s) {
	Mat4x4 result = {{
        {s.x,   0,   0,  0},
        {0,   s.y,   0,  0},
        {0,     0, s.z,  0},
        {0,     0,   0,  1},
    }};
	return result;
}

inline Mat4x4
mat4x4_rotation_x(f32 angle) {
	const f32 c = cosf(angle);
	const f32 s = sinf(angle);
	Mat4x4 r = {{
		{1, 0, 0, 0},
		{0, c,-s, 0},
		{0, s, c, 0},
		{0, 0, 0, 1}
	}};
	return(r);
}

inline Mat4x4
mat4x4_rotation_y(f32 angle) {
	const f32 c = cosf(angle);
	const f32 s = sinf(angle);
	Mat4x4 r = {{
		{ c, 0, s, 0},
		{ 0, 1, 0, 0},
		{-s, 0, c, 0},
		{ 0, 0, 0, 1}
	}};
	return(r);
}

inline Mat4x4
mat4x4_rotation_z(f32 angle) {
	const f32 c = cosf(angle);
	const f32 s = sinf(angle);
	Mat4x4 r = {{
		{c,-s, 0, 0},
		{s, c, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1}
	}};
	return(r);
}

inline Mat4x4
mat4x4_rotation(f32 angle, Vec3 a) {
	const f32 c = cosf(angle);
	const f32 s = sinf(angle);
	a = vec3_normalize(a);

	const f32 tx = (1.0f - c) * a.x;
	const f32 ty = (1.0f - c) * a.y;
	const f32 tz = (1.0f - c) * a.z;

	Mat4x4 r = {{
		{c + tx * a.x, 		     tx * a.y - s * a.z, tx * a.z - s * a.y, 0},
		{    ty * a.x - a.z * s, ty * a.y + c,       ty * a.z + s * a.x, 0},
		{    tz * a.x + s * a.y, tz * a.y - s * a.x, tz * a.z + c,       0},
		{0, 0, 0, 1}
	}};
	return(r);
}

inline Mat4x4
mat4x4_ortographic_2d(f32 l, f32 r, f32 b, f32 t) {
	Mat4x4 result =	{{
		{2.0f / (r - l),    0,                   0, 0},
		{0,                 2.0f / (t - b),      0, 0},
		{0,                 0,                  -1, 0},
		{-(r + l) / (r - l), -(t + b) / (t - b), 0, 1}
	}};
	return result;
}

inline Mat4x4
mat4x4_ortographic_3d(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
	Mat4x4 result =	{{
		{2.0f / (r - l),    0,                   0,                 0},
		{0,                 2.0f / (t - b),      0,                 0},
		{0,                 0,                  -2.0f / (f - n),    0},
		{-(r + l) / (r - l), -(t + b) / (t - b),-(f + n) / (f - n), 1}
	}};
	return result;
}

inline Mat4x4
mat4x4_perspective(f32 fov, f32 aspect, f32 n, f32 f) {
	const f32 toHf = tanf(fov * 0.5f);

	Mat4x4 r = {{
		{1.0f / (aspect * toHf), 0,           0,                         0},
		{0,                      1.0f / toHf, 0,                         0},
		{0,                      0,          -       (f + n) / (f - n), -1},
		{0,                      0,          -2.0f * (f * n) / (f - n),  0}
	}};
	return(r);
}

inline Mat4x4
mat4x4_mul(Mat4x4 a, Mat4x4 b) {
	Mat4x4 result;
	for(int r = 0; r < 4; ++r) {
		for(int c = 0; c < 4; ++c) {
            result.e[r][c] = a.e[0][c] * b.e[r][0]
                           + a.e[1][c] * b.e[r][1]
                           + a.e[2][c] * b.e[r][2]
                           + a.e[3][c] * b.e[r][3];
		}
	}
	return result;
}

inline Vec4
mat4x4_mul_vec4(Mat4x4 a, Vec4 v) {
	Vec4 result;
	result.x = a.e[0][0] * v.x + a.e[1][0] * v.y + a.e[2][0] * v.z + a.e[3][0] * v.w;
	result.y = a.e[0][1] * v.x + a.e[1][1] * v.y + a.e[2][1] * v.z + a.e[3][1] * v.w;
	result.z = a.e[0][2] * v.x + a.e[1][2] * v.y + a.e[2][2] * v.z + a.e[3][2] * v.w;
	result.w = a.e[0][3] * v.x + a.e[1][3] * v.y + a.e[2][3] * v.z + a.e[3][3] * v.w;
	return result;
}

typedef struct {
    Vec3 min;
    Vec3 max;
} Rect2;

typedef struct {
    Vec3 min;
    Vec3 max;  
} Box3;

typedef u32 Key;
enum {
    Key_None = 0x0,
    Key_W,
    Key_A,
    Key_S,
    Key_D,
    Key_Shift,
    Key_Ctrl,
    Key_Space,
    Key_MouseLeft,
    Key_MouseRight,
    Key_Count,
};

typedef struct {
    bool is_down;
    u8   transition_count;
} KeyState;

inline void 
update_key_state(KeyState *key, bool is_down) {
    if (key->is_down != is_down) {
        key->is_down = is_down;
        ++key->transition_count;
    }
}

inline bool 
is_key_held(KeyState *key) {
    return key->is_down;
}

inline bool 
is_key_pressed(KeyState *key) {
    return key->is_down && key->transition_count;
}

typedef struct {
    union {
        Vec2 mouse_pos;
        struct {
            f32 mouse_x;
            f32 mouse_y;
        };
    };
    union {
        Vec2 mouse_delta;
        struct {
            f32 mouse_delta_x;
            f32 mouse_delta_y;
        };
    };
    
    Vec2 window_size;
    f32 mouse_wheel;
    
    KeyState keys[Key_Count];
    
    f32 dt;
} Input;

static LRESULT WINAPI
main_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    switch (message) {
        case WM_QUIT:
        case WM_DESTROY:
        case WM_CLOSE: {
            is_running = false;
        } break;
        default: {
            result = DefWindowProcA(window, message, wparam, lparam);
        } break;
    }
    
    return result;
}

static void 
set_pixel_format(HDC hdc) {
    i32 suggested_pixel_format_index = 0;
    u32 extended_pick = 0;

    if (wgl.wglChoosePixelFormatARB) {
        int attributes[] = {
            WGL_DRAW_TO_WINDOW_ARB,				GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB,				GL_TRUE,
			WGL_PIXEL_TYPE_ARB,					WGL_TYPE_RGBA_ARB,
			WGL_ACCELERATION_ARB,				WGL_FULL_ACCELERATION_ARB,
			WGL_DOUBLE_BUFFER_ARB,				GL_TRUE,
			WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB,	GL_TRUE,
			WGL_COLOR_BITS_ARB,					32,
			WGL_DEPTH_BITS_ARB,					24,
			WGL_STENCIL_BITS_ARB,				8,
			WGL_ALPHA_BITS_ARB,					8,
			WGL_SAMPLE_BUFFERS_ARB,				GL_TRUE,
			WGL_SAMPLES_ARB,					4,
			0
        };
        wgl.wglChoosePixelFormatARB(hdc, attributes, 0, 1,
									&suggested_pixel_format_index, &extended_pick);
    }

    if (!extended_pick) {
        PIXELFORMATDESCRIPTOR desired_pixel_format = {
            .nSize = sizeof(desired_pixel_format),
            .nVersion = 1,
            .iPixelType = PFD_TYPE_RGBA,
            .dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
            .cColorBits = 32,
            .cAlphaBits = 8,
            .cDepthBits = 24,
            .iLayerType = PFD_MAIN_PLANE
        };
        
        suggested_pixel_format_index = ChoosePixelFormat(hdc, &desired_pixel_format);
    }

    PIXELFORMATDESCRIPTOR suggested_pixel_format;
    DescribePixelFormat(hdc, suggested_pixel_format_index,
                        sizeof(suggested_pixel_format), &suggested_pixel_format);
    SetPixelFormat(hdc, suggested_pixel_format_index, &suggested_pixel_format);
}

static void
update_mouse_button_state(KeyState *key, DWORD vk) {
    bool is_down = GetKeyState(vk) & (1 << 31);
    update_key_state(key, is_down);
}

typedef struct {
    Vec3 pos;
    Vec2 uv;
    u32  rgba;
    u16  texture_index;
} Vertex;

typedef struct {
    u32 index;
    u32 w;
    u32 h;
} RendererTexture;

typedef struct {
    u64 vertex_array_offset;
    u64 index_array_offset;
    
    u32 quad_count;
    
    RendererTexture tex;
} RenderQuads;

typedef struct {
    RenderQuads *render_quads;
    u64 render_quads_count;
    u64 render_quads_capacity;
    
    Vertex *vertex_buffer;
    u64 vertex_buffer_size;
    u64 vertex_buffer_capacity;
    
    u16 *index_buffer;
    u64 index_buffer_size;
    u64 index_buffer_capacity;
    
    GLuint gl_vertex_array;
    GLuint gl_vertex_buffer;
    GLuint gl_index_buffer;
    
    GLuint texture_array;
    u32    texture_array_size;
    u32    texture_array_capacity;
    
    GLuint shader;
    
    RendererTexture white_tex;
    
    
    Mat4x4 view_proj;
} Renderer;

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

char *
read_file_and_null_terminate(char *filename) {
    char *result = 0;
    
    FILE *file = fopen(filename, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        u64 size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        result = (char *)malloc(size + 1);
        fread(result, 1, size, file);
        result[size] = 0;
        
        fclose(file);
    }
    
    return result;
}

static RendererTexture
renderer_make_texture(Renderer *renderer, u32 width, u32 height, void *pixels) {
    RendererTexture tex = {0};
    tex.w = width;
    tex.h = height;
    tex.index = renderer->texture_array_size++;
    assert(renderer->texture_array_size < renderer->texture_array_capacity);
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->texture_array);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0,
                    tex.index, width, height, 1, 
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
    return tex;
}

#define rgba_pack_4x8_linear1_vec4(_v) rgba_pack_4x8_linear1(_v.x, _v.y, _v.z, _v.w) 
static u32
rgba_pack_4x8_linear1(f32 r, f32 g, f32 b, f32 a) {
    u32 ri = (u32)roundf(r * 255.0f);
    u32 gi = (u32)roundf(g * 255.0f);
    u32 bi = (u32)roundf(b * 255.0f);
    u32 ai = (u32)roundf(a * 255.0f);
    u32 result = (ri << 0) | (gi << 8) | (bi << 16) | (ai << 24);
    return result;
}

static void
renderer_push_quad(Renderer *renderer,
                   Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
				   Vec4 c00, Vec4 c01, Vec4 c10, Vec4 c11,
				   Vec2 uv00, Vec2 uv01, Vec2 uv10, Vec2 uv11,
                   RendererTexture texture) {
    assert(renderer->render_quads_count + 1 <= renderer->render_quads_capacity);
    RenderQuads *quads = renderer->render_quads + renderer->render_quads_count++;
    quads->quad_count = 1;
    quads->index_array_offset = renderer->index_buffer_size;
    quads->vertex_array_offset = renderer->vertex_buffer_size;

    u32 texture_index = texture.index;
    // @NOTE(hl): Transpose uvs from normalized space, to texture in texture array space
    // Vec2 array_size = renderer_settings.texture_size;
    Vec2 array_texture_size   = vec2(512, 512);
    Vec2 current_texture_size = vec2(texture.w, texture.h);
    Vec2 uv_scale = vec2_div(current_texture_size, array_texture_size);
    uv00 = vec2_mul(uv00, uv_scale);
    uv01 = vec2_mul(uv01, uv_scale);
    uv10 = vec2_mul(uv10, uv_scale);
    uv11 = vec2_mul(uv11, uv_scale);

    u32 packed_color00 = rgba_pack_4x8_linear1_vec4(c00);
    u32 packed_color01 = rgba_pack_4x8_linear1_vec4(c01);
    u32 packed_color10 = rgba_pack_4x8_linear1_vec4(c10);
    u32 packed_color11 = rgba_pack_4x8_linear1_vec4(c11);

    // Vertex buffer
    Vertex *vertex_buffer = renderer->vertex_buffer + renderer->vertex_buffer_size;
    vertex_buffer[0].pos = v00;
    vertex_buffer[0].uv  = uv00;
    vertex_buffer[0].rgba = packed_color00;
    vertex_buffer[0].texture_index = texture_index;

    vertex_buffer[1].pos = v01;
    vertex_buffer[1].uv  = uv01;
    vertex_buffer[1].rgba = packed_color01;
    vertex_buffer[1].texture_index = texture_index;

    vertex_buffer[2].pos = v10;
    vertex_buffer[2].uv  = uv10;
    vertex_buffer[2].rgba = packed_color10;
    vertex_buffer[2].texture_index = texture_index;

    vertex_buffer[3].pos = v11;
    vertex_buffer[3].uv  = uv11;
    vertex_buffer[3].rgba = packed_color11;
    vertex_buffer[3].texture_index = texture_index;

    // Index buffer
    u16 *index_buffer = renderer->index_buffer + renderer->index_buffer_size;
    u16  base_index   = renderer->vertex_buffer_size - quads->vertex_array_offset;
    index_buffer[0] = base_index + 0;
    index_buffer[1] = base_index + 2;
    index_buffer[2] = base_index + 3;
    index_buffer[3] = base_index + 0;
    index_buffer[4] = base_index + 1;
    index_buffer[5] = base_index + 3;

    // Update buffer sizes after we are finished.
    renderer->vertex_buffer_size += 4;
    renderer->index_buffer_size  += 6;
}

typedef u32 BlockID;
enum {
    BlockID_None = 0x0,
    BlockID_Air,
    BlockID_Grass,
    BlockID_Count,
};

typedef struct {
    i64 x;
    i64 y;
    i64 z;
    
    BlockID id;  
} Block;

#define CHUNK_X 16
#define CHUNK_Y 64
#define CHUNK_Z 16

typedef struct {
    i64 x;
    i64 y;
    i64 z;
    
    Block blocks[CHUNK_X * CHUNK_Z * CHUNK_Y];    
} Chunk;

typedef struct {
    Chunk chunk;
} World;

static void
renderer_push_cube(Renderer *renderer, Box3 box) {
    Vec3 vertices[8] = {
        vec3(box.min.e[0], box.min.e[1], box.min.e[2]),
        vec3(box.min.e[0], box.max.e[1], box.min.e[2]),
        vec3(box.max.e[0], box.min.e[1], box.min.e[2]),
        vec3(box.max.e[0], box.max.e[1], box.min.e[2]),
        vec3(box.min.e[0], box.min.e[1], box.max.e[2]),
        vec3(box.min.e[0], box.max.e[1], box.max.e[2]),
        vec3(box.max.e[0], box.min.e[1], box.max.e[2]),
        vec3(box.max.e[0], box.max.e[1], box.max.e[2])
    };  
    
#define standard_uv_list vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1)
#define stupid_colors COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW
    renderer_push_quad(renderer,
                       vertices[0], vertices[1], vertices[2], vertices[3],
                       stupid_colors,
                       standard_uv_list,
                       renderer->white_tex);
    renderer_push_quad(renderer,
                       vertices[4], vertices[5], vertices[6], vertices[7],
                       stupid_colors,
                       standard_uv_list,
                       renderer->white_tex);
                       
    renderer_push_quad(renderer,
                       vertices[0], vertices[1], vertices[4], vertices[5],
                       stupid_colors,
                       standard_uv_list,
                       renderer->white_tex);
    renderer_push_quad(renderer,
                       vertices[2], vertices[3], vertices[6], vertices[7],
                       stupid_colors,
                       standard_uv_list,
                       renderer->white_tex);

    renderer_push_quad(renderer,
                       vertices[2], vertices[0], vertices[6], vertices[4],
                       stupid_colors,
                       standard_uv_list,
                       renderer->white_tex);
    renderer_push_quad(renderer,
                       vertices[3], vertices[1], vertices[7], vertices[5],
                       stupid_colors,
                       standard_uv_list,
                       renderer->white_tex);
}

typedef struct {
    f32 vfov;
    f32 near_plane;
    f32 far_plane;
    
    Vec3 pos;
    f32 pitch;
    f32 yaw;
    
    Mat4x4 proj;
    Mat4x4 view;
    Mat4x4 view_proj;
} Camera;

typedef struct {
    Renderer *renderer;
    Input *input;
    
    Camera camera;
    
    World world;
} GameState;

static void
game_init(GameState *game) {
    game->camera.vfov = DEG2RAD(90.0f);
    game->camera.near_plane = 0.1f;
    game->camera.far_plane = 1000.0f;
}

static void 
update_game(GameState *game) {
    // Camera
    f32 view_coef = 1.0f * game->input->dt;
    if (is_key_held(game->input->keys + Key_MouseLeft)) {
        f32 x_angle_change = game->input->mouse_delta_x * view_coef;
        f32 y_angle_change = game->input->mouse_delta_y * view_coef;
        game->camera.yaw += x_angle_change;
        game->camera.pitch = clamp(game->camera.pitch + y_angle_change, -HALF_PI, HALF_PI);
        game->camera.yaw = unwind_rad(game->camera.yaw);
    }
    
    f32 move_coef = 4.0f * game->input->dt;
    
    f32 z_speed = 0;
    if (is_key_held(game->input->keys + Key_W)) {
        z_speed = move_coef;
    } else if (is_key_held(game->input->keys + Key_S)) {
        z_speed = -move_coef;
    }
    game->camera.pos.x += z_speed *  sinf(game->camera.yaw);
    game->camera.pos.z += z_speed * -cosf(game->camera.yaw);
    
    f32 x_speed = 0;
    if (is_key_held(game->input->keys + Key_D)) {
        x_speed = move_coef;
    } else if (is_key_held(game->input->keys + Key_A)) {
        x_speed = -move_coef;
    }
    game->camera.pos.x += x_speed *  sinf(game->camera.yaw + HALF_PI);
    game->camera.pos.z += x_speed * -cosf(game->camera.yaw + HALF_PI);
    
    f32 y_speed = 0;
    if (is_key_held(game->input->keys + Key_Ctrl)) {
        y_speed = -move_coef;
    } else if (is_key_held(game->input->keys + Key_Space)) {
        y_speed = move_coef;
    }
    game->camera.pos.y += y_speed;
    
    game->camera.view = MAT4X4_IDENTITIY;
    game->camera.view = mat4x4_mul(game->camera.view, mat4x4_rotation(game->camera.pitch, vec3(1, 0, 0)));
    game->camera.view = mat4x4_mul(game->camera.view, mat4x4_rotation(game->camera.yaw,   vec3(0, 1, 0)));
    game->camera.view = mat4x4_mul(game->camera.view, mat4x4_translate(vec3_neg(game->camera.pos)));
    
    f32 aspect_ratio = game->input->window_size.x / game->input->window_size.y;
    game->camera.proj = mat4x4_perspective(game->camera.vfov, aspect_ratio, game->camera.near_plane, game->camera.far_plane);
    
    game->camera.view_proj = mat4x4_mul(game->camera.proj, game->camera.view);
    game->renderer->view_proj = game->camera.view_proj;
    
    Box3 box = {
        .min = vec3(-1, -1, -1),
        .max = vec3(1, 1, 1)
    };
    renderer_push_cube(game->renderer, box);
}

int 
main(int argc, char **argv) {
    // Create window
    HINSTANCE instance = GetModuleHandle(0);
    WNDCLASSA wndclss = { 
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = main_window_proc,
        .hInstance = instance,
        .lpszClassName = "WINDOW",
        .hCursor = LoadCursorA(0, IDC_ARROW),
        .hbrBackground = (HBRUSH)(COLOR_WINDOW)
    };
    RegisterClassA(&wndclss);
    
    u32 window_width = 1280;
    u32 window_height = 720;
    HWND hwnd = CreateWindowExA(WS_EX_APPWINDOW, wndclss.lpszClassName, "MineCraft", 
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                                window_width, window_height, 0, 0, instance, 0);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    // Load opengl
    HMODULE opengl_dll = LoadLibraryA("opengl32.dll");
    assert(opengl_dll);
    
    *(FARPROC *)&wgl.wglCreateContext = GetProcAddress(opengl_dll, "wglCreateContext");
    *(FARPROC *)&wgl.wglDeleteContext = GetProcAddress(opengl_dll, "wglDeleteContext");
    *(FARPROC *)&wgl.wglGetProcAddress = GetProcAddress(opengl_dll, "wglGetProcAddress");
    *(FARPROC *)&wgl.wglGetCurrentDC = GetProcAddress(opengl_dll, "wglGetCurrentDC");
    *(FARPROC *)&wgl.wglMakeCurrent = GetProcAddress(opengl_dll, "wglMakeCurrent");
    *(FARPROC *)&wgl.wglSwapLayerBuffers = GetProcAddress(opengl_dll, "wglSwapLayerBuffers");
    
    WNDCLASSA temp_wndclss = {
        .lpfnWndProc = DefWindowProcA,
        .hInstance = instance,
        .lpszClassName = "GLLOADER"
    };
    RegisterClassA(&temp_wndclss);
    HWND temp_hwnd = CreateWindowA(temp_wndclss.lpszClassName, "", 0,
                                   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                   0, 0, instance, 0);
    HDC temp_hwnd_dc = GetDC(temp_hwnd);
    set_pixel_format(temp_hwnd_dc);
    
    HGLRC temp_gl_rc = wgl.wglCreateContext(temp_hwnd_dc);
    wgl.wglMakeCurrent(temp_hwnd_dc, temp_gl_rc);
    
    *(FARPROC *)&wgl.wglChoosePixelFormatARB = wgl.wglGetProcAddress("wglChoosePixelFormatARB");
    *(FARPROC *)&wgl.wglCreateContextAttribsARB = wgl.wglGetProcAddress("wglCreateContextAttribsARB");
    *(FARPROC *)&wgl.wglMakeContextCurrentARB = wgl.wglGetProcAddress("wglMakeContextCurrentARB");
    *(FARPROC *)&wgl.wglSwapIntervalEXT = wgl.wglGetProcAddress("wglSwapIntervalEXT");
    *(FARPROC *)&wgl.wglGetSwapIntervalEXT = wgl.wglGetProcAddress("wglGetSwapIntervalEXT");
    
    wgl.wglDeleteContext(temp_gl_rc);
    
    ReleaseDC(temp_hwnd, temp_hwnd_dc);
    DestroyWindow(temp_hwnd);
    
    HDC hwnd_dc = GetDC(hwnd);
    set_pixel_format(hwnd_dc);
    
    int opengl_attributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_FLAGS_ARB, 
        WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
	    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
	    0
    };
    HGLRC gl_rc = wgl.wglCreateContextAttribsARB(hwnd_dc, 0, opengl_attributes);
    wgl.wglMakeCurrent(hwnd_dc, gl_rc);
    
#define GLPROC(_name, _type)                                        \
    _name = (void *)wgl.wglGetProcAddress(#_name);                  \
    if (!_name) _name = (void *)GetProcAddress(opengl_dll, #_name); \
    if (!_name) fprintf(stderr, "[ERROR] Failed to load " #_name " OGL procedure.\n");
#include "gl_procs.inc"
#undef GLPROC
    
    wgl.wglSwapIntervalEXT(1);
    
    // Initialize renderer 
    Renderer renderer = {0};
    
    renderer.render_quads_capacity = 1024;
    renderer.render_quads = (RenderQuads *)calloc(renderer.render_quads_capacity, sizeof(RenderQuads));
    renderer.index_buffer_capacity = 1 << 20;
    renderer.index_buffer = (u16 *)calloc(renderer.index_buffer_capacity, sizeof(u16));
    renderer.vertex_buffer_capacity = 1 << 20;
    renderer.vertex_buffer = (Vertex *)calloc(renderer.vertex_buffer_capacity, sizeof(Vertex));
    
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallback(opengl_error_callback, 0);
    
    glGenVertexArrays(1, &renderer.gl_vertex_array);
    glBindVertexArray(renderer.gl_vertex_array);
    
    glGenBuffers(1, &renderer.gl_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.gl_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, renderer.vertex_buffer_capacity * sizeof(Vertex), 0, GL_STREAM_DRAW);
    
    glGenBuffers(1, &renderer.gl_index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.gl_index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, renderer.index_buffer_capacity * sizeof(u16), 0, GL_STREAM_DRAW);
    
    // GLboolean OpenGLNormalized = (normalized ? GL_TRUE : GL_FALSE);

    // gl->glEnableVertexAttribArray(index);
    // gl->glVertexAttribPointer(index, size, type, OpenGLNormalized, stride, (void *)offset);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (void *)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, sizeof(Vertex), (void *)offsetof(Vertex, rgba));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_SHORT, sizeof(Vertex), (void *)offsetof(Vertex, texture_index));
    
    u32 texture_array_w = 512;
    u32 texture_array_h = 512;
    renderer.texture_array_capacity = 256;
    
    glGenTextures(1, &renderer.texture_array);
    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer.texture_array);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1,
                   GL_RGBA8,
                   texture_array_w,
                   texture_array_h,
                   renderer.texture_array_capacity);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    char *shader_code = read_file_and_null_terminate("shader.glsl");

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const char * const vertex_source[] = { "#version 330\n", "#define VERTEX_SHADER\n", shader_code };
    const char * const fragment_source[] = { "#version 330\n", "#define FRAGMENT_SHADER\n", shader_code };
    glShaderSource(vertex_shader, ARRAY_SIZE(vertex_source), vertex_source, 0);
    glShaderSource(fragment_shader, ARRAY_SIZE(fragment_source), fragment_source, 0);
    glCompileShader(vertex_shader);
    glCompileShader(fragment_shader);
    
    GLint vertex_compiled, fragment_compiled;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_compiled);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_compiled);
    
    if (!(vertex_compiled && fragment_compiled)) {
        char shader_log[4096];
        if (!vertex_compiled) {
            glGetShaderInfoLog(vertex_shader, sizeof(shader_log), 0, shader_log);
            fprintf(stderr, "[ERROR] OpenGL vertex shader compilation failed: %s\n", shader_log);
        }
        if (!fragment_compiled) {
            glGetShaderInfoLog(fragment_shader, sizeof(shader_log), 0, shader_log);
            fprintf(stderr, "[ERROR] OpenGL fragment shader compilation failed: %s\n", shader_log);
        }
    }
    
    renderer.shader = glCreateProgram();
    glAttachShader(renderer.shader, vertex_shader);
    glAttachShader(renderer.shader, fragment_shader);
    glLinkProgram(renderer.shader);
    
    GLint link_success;
    glGetProgramiv(renderer.shader, GL_LINK_STATUS, &link_success);
    if (!link_success) {
        char program_log[4096];
        glGetProgramInfoLog(renderer.shader, sizeof(program_log), 0, program_log);
        fprintf(stderr, "[ERROR] OpenGL shader compilation failed: %s\n", program_log);
    }
    
    free(shader_code);
    
    u8 *white_texture = malloc(512 * 512 * 4);
    memset(white_texture, 0xFF, 512 * 512 * 4);
    renderer.white_tex = renderer_make_texture(&renderer, 512, 512, white_texture);
    free(white_texture);
    // Main loop
    Input input = {0};
    GameState game = {
        .renderer = &renderer,
        .input = &input
    };
    
    game_init(&game);
    
    LONGLONG perf_count_frequency;
    LARGE_INTEGER pf;
    QueryPerformanceFrequency(&pf);
    perf_count_frequency = pf.QuadPart;
    
    LARGE_INTEGER last_frame_time;
    QueryPerformanceCounter(&last_frame_time);
    
    while (is_running) {
        // Parse messages
        input.mouse_wheel = 0;
        
        for (u32 key_index = 0;
             key_index < Key_Count;
             ++key_index) {
            (input.keys + key_index)->transition_count = 0;        
        }
        
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
            switch (msg.message) {
                default: {
                    TranslateMessage(&msg);
                    DispatchMessageA(&msg);
                } break;
                case WM_MOUSEWHEEL: {
                    i16 wheel_delta = HIWORD(msg.wParam);
                    input.mouse_wheel += (f32)wheel_delta / WHEEL_DELTA;
                } break;
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP: {
                    bool is_down = ((msg.lParam & (1 << 31)) == 0);
                    i32  scancode = (HIWORD(msg.lParam) & (KF_EXTENDED | 0xFF));

                    if (!scancode) {
                        scancode = MapVirtualKeyW(msg.wParam, MAPVK_VK_TO_VSC);
                    }

                    Key key = Key_None;
                    switch (scancode) {
                        case 0x011: {
                            key = Key_W;
                        } break;
                        case 0x01E: {
                            key = Key_A;
                        } break;
                        case 0x01F: {
                            key = Key_S;
                        } break;
                        case 0x020: {
                            key = Key_D;
                        } break;
                        case 0x02A: {
                            key = Key_Shift;
                        } break;
                        case 0x01D: {
                            key = Key_Ctrl;
                        } break;
                        case 0x039: {
                            key = Key_Space;
                        } break;
                    }

                    if (key) {
                        update_key_state(input.keys + key, is_down);
                    }
                } break;
            }
        }
        
        update_mouse_button_state(input.keys + Key_MouseLeft, VK_LBUTTON);
        update_mouse_button_state(input.keys + Key_MouseRight, VK_RBUTTON);
        
        POINT mp;
        GetCursorPos(&mp);
        ScreenToClient(hwnd, &mp);
        Vec2 mouse = vec2(mp.x, mp.y);
        
        input.mouse_delta_x = mouse.x - input.mouse_x;
        input.mouse_delta_y = mouse.y - input.mouse_y;
        input.mouse_pos = mouse;
        
        RECT wr;
        GetClientRect(hwnd, &wr);
        input.window_size = vec2(wr.right - wr.left, wr.bottom - wr.top);
        
        LARGE_INTEGER current_time;
        QueryPerformanceCounter(&current_time);
        f32 delta_time = (f32)(current_time.QuadPart - last_frame_time.QuadPart) / (f32)perf_count_frequency;
        last_frame_time = current_time;
        input.dt = delta_time;
        
        // Prepare renderer
        renderer.render_quads_count = 0;
        renderer.vertex_buffer_size = 0;
        renderer.index_buffer_size = 0;
        
        // Game
        update_game(&game);
        
        // Draw
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, input.window_size.x, input.window_size.y);
        glScissor(0, 0, input.window_size.x, input.window_size.y);
        
        glBindVertexArray(renderer.gl_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.gl_vertex_array);
        glBufferSubData(GL_ARRAY_BUFFER, 0, renderer.vertex_buffer_size * sizeof(Vertex), renderer.vertex_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.gl_index_buffer);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, renderer.index_buffer_size * sizeof(u16), renderer.index_buffer);

        Vec4 clear_color = vec4(0.2, 0.2, 0.2, 1);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(renderer.shader);
        glUniform1i(glGetUniformLocation(renderer.shader, "texture_sampler"), 0);
        glUniformMatrix4fv(glGetUniformLocation(renderer.shader, "mvp_matrix"), 1, GL_FALSE, renderer.view_proj.e[0]);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, renderer.texture_array);
        for (u32 render_quads_index = 0;
             render_quads_index < renderer.render_quads_count;
             ++render_quads_index) {
            RenderQuads quads = renderer.render_quads[render_quads_index];
            
            glDrawElementsBaseVertex(GL_TRIANGLES, 6 * quads.quad_count, GL_UNSIGNED_SHORT,
                                     (GLvoid *)(sizeof(u16) * quads.index_array_offset),
                                     quads.vertex_array_offset);
        }
        
        wgl.wglSwapLayerBuffers(wgl.wglGetCurrentDC(), WGL_SWAP_MAIN_PLANE);
    }
    
    return 0;
}