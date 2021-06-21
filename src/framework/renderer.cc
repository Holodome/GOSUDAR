#include "framework/renderer.hh"
#include "framework/assets.hh"
#include "game/game.hh"

#define GLPROC(_name, _type) \
static _type _name;
#include "framework/gl_procs.inc"
#undef GLPROC

#include "framework/renderer_internal.cc"

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
    standard_shader = create_shader(standard_shader_code);
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
    terrain_shader = create_shader(terrain_shader_code);
    default_shader = standard_shader;
   
    // glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    this->vertex_count = 0;
    this->max_vertex_count = 1 << 16;
    this->vertices = (Vertex *)arena_alloc(&this->arena, this->max_vertex_count * sizeof(Vertex));
    logprint("Renderer", "Init end\n");
}

void Renderer::cleanup() {
}

void Renderer::begin_frame() {
    statistics = current_statistics;
    current_statistics.begin_frame();
    this->has_render_group = false;
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
    this->vertex_count = 0;
    if (immediate_vao == GL_INVALID_ID) {
        glGenVertexArrays(1, &immediate_vao);
        glBindVertexArray(immediate_vao);
        glGenBuffers(1, &immediate_vbo);
    }
}

void Renderer::imm_flush(Shader shader, Texture texture, Mat4x4 mvp, bool has_depth) {
    if (!vertex_count) { return; }
    
    assert(shader != Shader::invalid());
    bind_shader(&shader);
    glUniformMatrix4fv(glGetUniformLocation(shader.id, "mvp"), 1, false, mvp.value_ptr());
	assert(texture != Texture::invalid());
    bind_texture(&texture);
    glUniform1i(glGetUniformLocation(shader.id, "tex"), 0);
    if (has_depth) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    
    glBindVertexArray(immediate_vao);
    glBindBuffer(GL_ARRAY_BUFFER, immediate_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * this->vertex_count, this->vertices, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, p));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, n));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, c));
    glEnableVertexAttribArray(3);
    
    ++current_statistics.draw_call_count;
    glDrawArrays(GL_TRIANGLES, 0, this->vertex_count);
}

void Renderer::imm_vertex(const Vertex &v) {
    assert(this->vertex_count < this->max_vertex_count);
    this->vertices[this->vertex_count++] = v;
}

void imm_draw_v(RenderGroup *render_group, Vertex vertices[4], AssetID texture_id) {
    render_group_set_texture(render_group, texture_id);
    render_group->renderer->imm_begin();
    render_group->renderer->imm_vertex(vertices[3]);
    render_group->renderer->imm_vertex(vertices[1]);
    render_group->renderer->imm_vertex(vertices[0]);
    render_group->renderer->imm_vertex(vertices[0]);
    render_group->renderer->imm_vertex(vertices[2]);
    render_group->renderer->imm_vertex(vertices[3]);
    render_group->renderer->imm_flush(render_group->shader, render_group->texture, render_group->mvp, render_group->has_depth);                  
}

void imm_draw_quad(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
                             Vec4 c00, Vec4 c01, Vec4 c10, Vec4 c11,
                             Vec2 uv00, Vec2 uv01, Vec2 uv10, Vec2 uv11,
                             AssetID texture_id) {
    render_group_set_texture(render_group, texture_id);
    render_group->renderer->imm_begin();
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
    render_group->renderer->imm_vertex(v3);
    render_group->renderer->imm_vertex(v1);
    render_group->renderer->imm_vertex(v0);
    render_group->renderer->imm_vertex(v0);
    render_group->renderer->imm_vertex(v2);
    render_group->renderer->imm_vertex(v3);
    render_group->renderer->imm_flush(render_group->shader, render_group->texture, render_group->mvp, render_group->has_depth);                  
}

void imm_draw_quad(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
                             Vec4 c, AssetID texture_id) {
    imm_draw_quad(render_group, v00, v01, v10, v11, c, c, c, c, Vec2(0, 0), Vec2(0, 1), Vec2(1, 0), Vec2(1, 1), texture_id);
}

void imm_draw_quad(RenderGroup *render_group, Vec3 v[4], AssetID texture_id) {
    imm_draw_quad(render_group, v[0], v[1], v[2], v[3], Colors::white, Colors::white, Colors::white, Colors::white,
                        Vec2(0, 0), Vec2(0, 1), Vec2(1, 0), Vec2(1, 1), texture_id);
}

void imm_draw_rect(RenderGroup *render_group, Rect rect, Vec4 color, Rect uv_rect, AssetID texture_id) {
    Vec3 v[4]; 
    rect.store_points(v);
    Vec2 uvs[4];
    uv_rect.store_points(uvs);
    imm_draw_quad(render_group, v[0], v[1], v[2], v[3], color, color, color, color, uvs[0], uvs[1], uvs[2], uvs[3], texture_id);
}

void imm_draw_text(RenderGroup *render_group, Vec2 p, Vec4 color, const char *text, AssetID font_id, f32 scale) {
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
            
            imm_draw_rect(render_group, Rect(x1, y1, x2 - x1, y2 - y1), color, Rect(s1, t1, s2 - s1, t2 - t1), texture_id);
			f32 char_advance = glyph->x_advance * scale;
			offset.x += char_advance;
		}
	}
}

void imm_draw_line(RenderGroup *render_group, Vec3 a, Vec3 b, Vec4 color, f32 thickness) {
    // @TODO not behaving properly when ab is close to parallel with cam_z
    // Vec3 cam_z = this->imvp.v[2].xyz;
    Vec3 cam_z = render_group->mvp.get_z();
    Vec3 line = (b - a);
    line -= cam_z * Math::dot(cam_z, line);
    Vec3 line_perp = Math::cross(line, cam_z);
    // Vec3 other_perp = Math::cross(line_perp, line);
    line_perp = Math::normalize(line_perp);
    // other_perp = Math::normalize(other_perp);
    line_perp *= thickness;
    // other_perp *= thickness;
    imm_draw_quad(render_group, a - line_perp, a + line_perp, b - line_perp, b + line_perp, color);
    // this->imm_draw_quad(a - other_perp, a + other_perp, b - other_perp, b + other_perp, color);
    
}

void imm_draw_quad_outline(RenderGroup *render_group, Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11, Vec4 color, f32 thickness) {
    imm_draw_line(render_group, v00, v01, color, thickness);
    imm_draw_line(render_group, v01, v11, color, thickness);
    imm_draw_line(render_group, v11, v10, color, thickness);
    imm_draw_line(render_group, v10, v00, color, thickness);
}

void imm_draw_rect_outline(RenderGroup *render_group, Rect rect, Vec4 color, f32 thickness) {
    Vec3 v[4]; 
    rect.store_points(v);
    imm_draw_quad_outline(render_group, v[0], v[1], v[2], v[3], color, thickness);
}

Texture Renderer::create_texture(void *buffer, Vec2i size) {
    Texture tex = create_texture_internal(buffer, size);
    return tex;
}

RenderGroup render_group_begin(Renderer *renderer, Assets *assets, Mat4x4 mvp) {
    RenderGroup result = {};
    assert(!renderer->has_render_group);
    renderer->has_render_group = true;
    
    result.renderer = renderer;
    result.assets = assets;
    
    result.has_depth = true;
    result.mvp = mvp;
    result.imvp = Mat4x4::inverse(mvp);    
    result.texture = renderer->white_texture;
    result.shader = renderer->default_shader;
    return result;
}

void render_group_set_texture(RenderGroup *group, AssetID texture_id) {
    // @CLEAN
    if (texture_id == INVALID_ASSET_ID) {
        texture_id = Asset_White;
    }
    Texture texture = group->assets->get_tex(texture_id);
    if (texture == Texture::invalid()) {
        texture = group->renderer->white_texture;
    }
    group->texture = texture;    
}

void render_group_end(RenderGroup *group) {
    assert(group->renderer->has_render_group);
    group->renderer->has_render_group = false;
}
