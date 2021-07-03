#include "render_group.hh"


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
    TIMED_FUNCTION();
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

static Texture get_texture(RenderGroup *render_group, AssetID id) {
    Texture result;
    if (id.value == INVALID_ASSET_ID.value) {
        result = render_group->commands->white_texture;
    } else {
        result = *assets_get_texture(render_group->assets, id);
    }
    return result;
}

void push_quad(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
                             Vec4 c, AssetID texture_id) {
    Texture tex = get_texture(render_group, texture_id);
    push_quad(render_group, v00, v01, v10, v11, c, c, c, c, Vec2(0, 0), Vec2(0, 1), Vec2(1, 0), Vec2(1, 1), tex);
}

void push_quad(RenderGroup *render_group, Vec3 v[4], AssetID texture_id) {
    Texture tex = get_texture(render_group, texture_id);
    push_quad(render_group, v[0], v[1], v[2], v[3], WHITE, WHITE, WHITE, WHITE,
        Vec2(0, 0), Vec2(0, 1), Vec2(1, 0), Vec2(1, 1), tex);
}

void push_quad(RenderGroup *render_group, Vec3 v[4], Vec4 c, AssetID texture_id) {
    Texture tex = get_texture(render_group, texture_id);
    push_quad(render_group, v[0], v[1], v[2], v[3], c, c, c, c,
        Vec2(0, 0), Vec2(0, 1), Vec2(1, 0), Vec2(1, 1), tex);
}

void push_rect(RenderGroup *render_group, Rect rect, Vec4 color, Rect uv_rect, AssetID texture_id) {
    Vec3 v[4]; 
    rect.store_points(v);
    Vec2 uvs[4];
    uv_rect.store_points(uvs);
    Texture tex = get_texture(render_group, texture_id);
    push_quad(render_group, v[0], v[1], v[2], v[3], color, color, color, color, uvs[0], uvs[1], uvs[2], uvs[3], tex);
}

void push_text(RenderGroup *render_group, Vec2 p, Vec4 color, const char *text, AssetID font_id, f32 scale) {
    if (!text) {
        return;
    }
    
    AssetFont *font = assets_get_font(render_group->assets, font_id);
    f32 font_height = 20;
    f32 line_height = font_height * scale;
    AssetID texture_id = font_id;

	f32 rwidth  = 1.0f / (f32)font->texture.width;
	f32 rheight = 1.0f / (f32)font->texture.height;

	Vec3 offset = Vec3(p, 0);
	offset.y += line_height;
    
	for (const char *scan = text; *scan; ++scan) {
		u8 symbol = *scan;
		if ((symbol >= font->first_codepoint)) {
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
    line -= cam_z * dot(cam_z, line);
    Vec3 line_perp = cross(line, cam_z);
    line_perp = normalize(line_perp);
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