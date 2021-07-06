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
void push_quad(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
    Vec4 c00, Vec4 c01, Vec4 c10, Vec4 c11,
    Vec2 uv00 = Vec2(0, 0), Vec2 uv01 = Vec2(0, 1), Vec2 uv10 = Vec2(1, 0), Vec2 uv11 = Vec2(1, 1),
    Texture texture = INVALID_TEXTURE);
void push_quad(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
    Vec4 c = WHITE, AssetID texture_id = INVALID_ASSET_ID);
void push_quad(RenderGroup *render_group, Vec3 v[4], AssetID texture_id);
void push_rect(RenderGroup *render_group, Rect rect, Vec4 color, Rect uv_rect = Rect(0, 0, 1, 1), AssetID texture_id = INVALID_ASSET_ID);
void push_line(RenderGroup *render_group, Vec3 a, Vec3 b, Vec4 color = WHITE, f32 thickness = 1.0f);
void push_quad_outline(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11, Vec4 color = WHITE, f32 thickness = 1.0f);
void push_rect_outline(RenderGroup *render_group, Rect rect, Vec4 color = WHITE, f32 thickness = 1.0f);
void DEBUG_push_text(RenderGroup *render_group, Vec2 p, Vec4 color, const char *text, AssetID font_id, f32 scale);


#define RENDER_GROUP_HH 1
#endif
