#if !defined(RENDER_GROUP_HH)

#include "assets.hh"

//
// Renderer API
//

enum {
    RENDERER_COMMAND_NONE,
    // Set framebuffer to separated
    RENDERER_COMMAND_BEGIN_SEPARATED,
    // Blit separated framebuffer on default
    RENDERER_COMMAND_END_SEPARATED,
    // Set setup for quads
    RENDERER_COMMAND_SET_SETUP,
    // Draw call
    RENDERER_COMMAND_QUADS,
    // Blur framebuffer and blit on current
    // @TODO API is not clear - just because it forces use of separated framebuffer
    // Also what if we want to control the amounth of blur or whatever
    RENDERER_COMMAND_BLUR,
    RENDERER_COMMAND_BEGIN_DEPTH_PEELING,
    RENDERER_COMMAND_END_DEPTH_PEELING,
    RENDERER_COMMAND_SENTINEL,
};

struct RendererSetup {
    mat4x4 view;
    mat4x4 projection;
    mat4x4 mvp;
};

inline RendererSetup setup_3d(u32 framebuffer, mat4x4 view, mat4x4 projection);
inline RendererSetup setup_2d(u32 framebuffer, mat4x4 projection);

struct RendererCommandHeader {
    u32 type;
};

struct RendererCommandQuads {
    uptr quad_count;
    uptr vertex_array_offset;
    uptr index_array_offset;  
};

// Set rendering to be done on separate framebuffer - useful when need to postprocess
void begin_separated_rendering(RendererCommands *commands);
// Render separated framebuffef to default one
void end_separated_rendering(RendererCommands *commands);
// Set setup to be used by further draw calls 
void set_setup(RendererCommands *commands, RendererSetup *setup);
// Actual draw call
void push_quad(RendererCommands *commands, vec3 v00, vec3 v01, vec3 v10, vec3 v11,
               vec4 c00, vec4 c01, vec4 c10, vec4 c11,
               vec2 uv00 = Vec2(0, 0), vec2 uv01 = Vec2(0, 1), vec2 uv10 = Vec2(1, 0), vec2 uv11 = Vec2(1, 1),
               Texture texture = INVALID_TEXTURE);
// Perfoms blurring and renders it on same framebuffer
void do_blur(RendererCommands *commands);
void begin_depth_peel(RendererCommands *commands);
void end_depth_peel(RendererCommands *commands);

//
// Additional rendering helpers
//
#define MAX_CLIP_RECT_STACK_SIZE 16

// Render group is layer above renderer draw calls
// for example it can store missing resource count, 
// but its primary usage is saving time writing assets accessing code
struct RenderGroup {
    RendererCommands *commands;
    struct Assets *assets;
    // This only affects push_rect calls and their derivatives!
    // @TODO Probably should rename calls using this in push_rect_clip_constrained or something
    u32 clip_rect_stack_idx;
    aarect clip_rect_stack[MAX_CLIP_RECT_STACK_SIZE];
};

inline RenderGroup create_render_group(RendererCommands *commands, struct Assets *assets) {
    RenderGroup result = {};
    result.commands = commands;
    result.assets = assets;
    result.clip_rect_stack[0] = AARectInfinite();
    return result;
}

void push_quad(RenderGroup *render_group, vec3 v00, vec3 v01, vec3 v10, vec3 v11,
               vec4 c = WHITE, AssetID texture_id = INVALID_ASSET_ID);
void push_quad(RenderGroup *render_group, vec3 v[4], vec4 c = WHITE, AssetID texture_id = INVALID_ASSET_ID);
void push_quad(RenderGroup *render_group, vec3 v[4], AssetID texture_id = INVALID_ASSET_ID);

void push_rect(RenderGroup *render_group, aarect rect, vec4 color, aarect uv_rect = AARectUnit(), AssetID texture_id = INVALID_ASSET_ID);

void push_clip_rect(RenderGroup *render_group, aarect rect);
void pop_clip_rect(RenderGroup *render_group);
#define DEFAULT_THICKNESS 0.05f
void DEBUG_push_line(RenderGroup *render_group, vec3 a, vec3 b, vec4 color = BLACK, f32 thickness = DEFAULT_THICKNESS);
void DEBUG_push_quad_outline(RenderGroup *render_group, vec3 v00, vec3 v01, vec3 v10, vec3 v11, vec4 color = BLACK, f32 thickness = DEFAULT_THICKNESS);
void DEBUG_push_quad_outline(RenderGroup *render_group, vec3 v[4], vec4 color = BLACK, f32 thickness = DEFAULT_THICKNESS);
void DEBUG_push_rect_outline(RenderGroup *render_group, aarect rect, vec4 color = BLACK, f32 thickness = DEFAULT_THICKNESS);
void DEBUG_push_text(RenderGroup *render_group, vec2 p, vec4 color, const char *text, AssetID font_id, f32 scale = 1.0f);

#define RENDER_GROUP_HH 1
#endif
