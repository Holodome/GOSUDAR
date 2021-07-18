#if !defined(RENDER_GROUP_HH)

#include "assets.hh"

// Render group is public interface for rendering.
// It is repsonsible for drawing with making sure renderer gets correct information about 
// additional settings, like camera transform.
// So multiple render groups can use different camera transforms, but calls to every one of them is 
// guaranteed to produce correct result in the end
struct RenderGroup {
    RendererCommands *commands;
    struct Assets *assets;
    RendererSetup setup;  
};

RenderGroup render_group_begin(struct RendererCommands *commands, Assets *assets, RendererSetup setup);
void render_group_end(RenderGroup *group);
void push_quad(RenderGroup *render_group, vec3 v00, vec3 v01, vec3 v10, vec3 v11,
               vec4 c00, vec4 c01, vec4 c10, vec4 c11,
               vec2 uv00 = Vec2(0, 0), vec2 uv01 = Vec2(0, 1), vec2 uv10 = Vec2(1, 0), vec2 uv11 = Vec2(1, 1),
               Texture texture = INVALID_TEXTURE);
void push_quad(RenderGroup *render_group, vec3 v00, vec3 v01, vec3 v10, vec3 v11,
               vec4 c = WHITE, AssetID texture_id = INVALID_ASSET_ID);
void push_quad(RenderGroup *render_group, vec3 v[4], AssetID texture_id = INVALID_ASSET_ID);
void push_rect(RenderGroup *render_group, Rect rect, vec4 color, Rect uv_rect = Rect(0, 0, 1, 1), AssetID texture_id = INVALID_ASSET_ID);
void push_line(RenderGroup *render_group, vec3 a, vec3 b, vec4 color = WHITE, f32 thickness = 1.0f);
void push_quad_outline(RenderGroup *render_group, vec3 v00, vec3 v01, vec3 v10, vec3 v11, vec4 color = WHITE, f32 thickness = 1.0f);
void push_rect_outline(RenderGroup *render_group, Rect rect, vec4 color = WHITE, f32 thickness = 1.0f);
void DEBUG_push_text(RenderGroup *render_group, vec2 p, vec4 color, const char *text, AssetID font_id, f32 scale);


#define RENDER_GROUP_HH 1
#endif
