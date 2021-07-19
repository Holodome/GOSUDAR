#include "render_group.hh"

#include "renderer.hh"

RendererSetup setup_3d(Mat4x4 view, Mat4x4 projection) {
    RendererSetup result;
    result.view = view;
    result.projection = projection;
    result.mvp = projection * view;
    return result;
}

RendererSetup setup_2d(Mat4x4 projection) {
    RendererSetup result;
    result.projection = projection;
    result.view = Mat4x4::identity();
    result.mvp = projection;
    return result;
}

// Data size is size of full request - with header and additional structures
static RendererCommandHeader *get_command_buffer_memory(RendererCommands *commands, size_t data_size) {
    assert(data_size >= sizeof(RendererCommandHeader));
    RendererCommandHeader *result = 0;
    if (commands->command_memory_used + data_size <= commands->command_memory_size) {
        result = (RendererCommandHeader *)(commands->command_memory + commands->command_memory_used);
        commands->command_memory_used += data_size;
    }
    
    return result;
}

#define push_command_no_storage(_commands, _type) (push_command_(_commands, 0, _type), 0)
#define push_command(_commands, _struct, _type) (_struct *)push_command_(_commands, sizeof(_struct), _type)
static u8 *push_command_(RendererCommands *commands, size_t data_size, u32 type) {
    u8 *result = 0;
    
    data_size += sizeof(RendererCommandHeader);
    RendererCommandHeader *header = get_command_buffer_memory(commands, data_size);
    if (header) {
        header->type = type;
        commands->last_header = header;
        
        result = (u8 *)(header + 1);
    }
    
    return result;
}

static RendererCommandQuads *get_current_quads(RendererCommands *commands) {
    RendererCommandQuads *result = 0;
    if (commands->last_header && commands->last_header->type == RENDERER_COMMAND_QUADS) {
        result = (RendererCommandQuads *)(commands->last_header + 1);
        ++result->quad_count;
    } else {
        result = push_command(commands, RendererCommandQuads, RENDERER_COMMAND_QUADS);
        if (result) {
            result->index_array_offset = commands->index_count;
            result->vertex_array_offset = commands->vertex_count;
            result->quad_count = 1;
        }
    }
    return result;
}

void begin_separated_rendering(RendererCommands *commands) {
    push_command_no_storage(commands, RENDERER_COMMAND_BEGIN_SEPARATED);
}

void end_separated_rendering(RendererCommands *commands) {
    push_command_no_storage(commands, RENDERER_COMMAND_END_SEPARATED);
}

void do_blur(RendererCommands *commands) {
    push_command_no_storage(commands, RENDERER_COMMAND_BLUR);
}

void set_setup(RendererCommands *commands, RendererSetup *src) {
    RendererSetup *dst = push_command(commands, RendererSetup, RENDERER_COMMAND_SET_SETUP);
    *dst = *src;
    commands->last_setup = dst;
}


void begin_depth_peel(RendererCommands *commands) {
    push_command_no_storage(commands, RENDERER_COMMAND_BEGIN_DEPTH_PEELING);
}

void end_depth_peel(RendererCommands *commands) {
    push_command_no_storage(commands, RENDERER_COMMAND_END_DEPTH_PEELING);
}

void push_quad(RendererCommands *commands, vec3 v00, vec3 v01, vec3 v10, vec3 v11,
               vec4 c00, vec4 c01, vec4 c10, vec4 c11,
               vec2 uv00, vec2 uv01, vec2 uv10, vec2 uv11,
               Texture texture) {
    TIMED_FUNCTION();
    RendererCommandQuads *quads = get_current_quads(commands);
    if (quads) {
        vec2 uv_scale = Vec2(texture.width, texture.height) * RENDERER_RECIPROCAL_TEXTURE_SIZE;
        uv00 = uv00 * uv_scale;
        uv01 = uv01 * uv_scale;
        uv10 = uv10 * uv_scale;
        uv11 = uv11 * uv_scale;
        
        u16 texture_index = (u16)texture.index;
        // Vertex buffer
        assert(commands->vertex_count + 4 <= commands->max_vertex_count);
        Vertex *vertex_buffer = commands->vertices + commands->vertex_count;
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
        assert(commands->index_count + 6 <= commands->max_index_count);
        RENDERER_INDEX_TYPE *index_buffer = commands->indices + commands->index_count;
        RENDERER_INDEX_TYPE  base_index   = (RENDERER_INDEX_TYPE)(commands->vertex_count - quads->vertex_array_offset);
        index_buffer[0] = base_index + 0;
        index_buffer[1] = base_index + 2;
        index_buffer[2] = base_index + 3;
        index_buffer[3] = base_index + 0;
        index_buffer[4] = base_index + 1;
        index_buffer[5] = base_index + 3;
        
        // Update buffer sizes after we are finished.
        commands->vertex_count += 4;
        commands->index_count  += 6;
    }
}

static Texture get_texture(RenderGroup *render_group, AssetID id) {
    Texture result;
    if (IS_SAME(id, INVALID_ASSET_ID)) {
        result = render_group->commands->white_texture;
    } else {
        Texture *requested = assets_get_texture(render_group->assets, id);
        assert(requested);
        result = *requested;
    }
    return result;
}
#define DEFAULT_UV_LIST Vec2(0, 0), Vec2(0, 1), Vec2(1, 0), Vec2(1, 1)

void push_quad(RenderGroup *render_group, vec3 v00, vec3 v01, vec3 v10, vec3 v11,
               vec4 c, AssetID texture_id) {
    Texture texture = get_texture(render_group, texture_id);
    push_quad(render_group->commands, v00, v01, v10, v11, c, c, c, c, DEFAULT_UV_LIST, texture);
}

void push_quad(RenderGroup *render_group, vec3 v[4], vec4 c, AssetID texture_id) {
    push_quad(render_group, v[0], v[1], v[2], v[3], c, texture_id);
}

void push_quad(RenderGroup *render_group, vec3 v[4], AssetID texture_id) {
    push_quad(render_group, v, WHITE, texture_id);
}

void push_rect(RenderGroup *render_group, Rect rect, vec4 color, Rect uv_rect, AssetID texture_id) {
    vec3 v[4]; 
    rect.store_points(v);
    vec2 uvs[4];
    uv_rect.store_points(uvs);
    Texture tex = get_texture(render_group, texture_id);
    push_quad(render_group->commands, v[0], v[1], v[2], v[3], color, color, color, color, uvs[0], uvs[1], uvs[2], uvs[3], tex);
}

void DEBUG_push_line(RenderGroup *render_group, vec3 a, vec3 b, vec4 color, f32 thickness) {
    vec3 cam_z = render_group->commands->last_setup->mvp.get_z();
    vec3 line = (b - a);
    line -= cam_z * dot(cam_z, line);
    vec3 line_perp = cross(line, cam_z);
    line_perp = normalize(line_perp);
    line_perp *= thickness;
    push_quad(render_group, a - line_perp, a + line_perp, b - line_perp, b + line_perp, color);
}

void DEBUG_push_quad_outline(RenderGroup *render_group, vec3 v00, vec3 v01, vec3 v10, vec3 v11, vec4 color, f32 thickness) {
    DEBUG_push_line(render_group, v00, v01, color, thickness);
    DEBUG_push_line(render_group, v01, v11, color, thickness);
    DEBUG_push_line(render_group, v11, v10, color, thickness);
    DEBUG_push_line(render_group, v10, v00, color, thickness);
}

void DEBUG_push_quad_outline(RenderGroup *render_group, vec3 v[4], vec4 color, f32 thickness) {
    DEBUG_push_quad_outline(render_group, v[0], v[1], v[2], v[3], color, thickness);
}

void DEBUG_push_rect_outline(RenderGroup *render_group, Rect rect, vec4 color, f32 thickness) {
    vec3 v[4]; 
    rect.store_points(v);
    DEBUG_push_quad_outline(render_group, v[0], v[1], v[2], v[3], color, thickness);
}

void DEBUG_push_text(RenderGroup *render_group, vec2 p, vec4 color, const char *text, AssetID font_id, f32 scale) {
    if (!text) {
        return;
    }
    
    AssetInfo *info = assets_get_info(render_group->assets, font_id);
    AssetFont *font = assets_get_font(render_group->assets, font_id);
    f32 font_height = info->size;
    f32 line_height = font_height * scale;
    AssetID texture_id = font_id;
    
	f32 rwidth  = 1.0f / (f32)font->texture.width;
	f32 rheight = 1.0f / (f32)font->texture.height;
    
	vec3 offset = Vec3(p, 0);
	offset.y += line_height;
    
	for (const char *scan = text; *scan; ++scan) {
		u8 symbol = *scan;
		if ((symbol >= info->first_codepoint)) {
			FontGlyph *glyph = &font->glyphs[symbol - info->first_codepoint];
            
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

