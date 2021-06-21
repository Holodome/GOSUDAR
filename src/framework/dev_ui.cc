#include "framework/dev_ui.hh"

#include "game/game.hh"

static DevUIID id_from_cstr(const char *cstr) {
    size_t count = strlen(cstr);
    DevUIID result;
    result.v = crc32(cstr, count);
    return result;
}

static DevUIID id_empty() {
    return {};
}

static void push_rect(DevUILayout *layout, Rect rect, Vec4 color = Colors::white, AssetID tex_id = Asset_White, Rect uv_rect = Rect(0, 0, 1, 1)) {
    DevUIDrawQueueEntry entry = {};
    entry.v[0].p = Vec3(rect.top_left());
    entry.v[0].uv = uv_rect.top_left();
    entry.v[0].c = color;
    entry.v[1].p = Vec3(rect.top_right());
    entry.v[1].uv = uv_rect.top_right();
    entry.v[1].c = color;
    entry.v[2].p = Vec3(rect.bottom_left());
    entry.v[2].uv = uv_rect.bottom_left();
    entry.v[2].c = color;
    entry.v[3].p = Vec3(rect.bottom_right());
    entry.v[3].uv = uv_rect.bottom_right();
    entry.v[3].c = color;
    entry.tex_id = tex_id;
    
    assert(layout->draw_queue_entry_count < layout->max_draw_queue_entry_count);
    layout->draw_queue[layout->draw_queue_entry_count++] = entry;
}

static void push_text(DevUILayout *layout, Vec2 p, const char *text, Vec4 color = Colors::white) {
    FontData *font = layout->dev_ui->font;
    f32 line_height = layout->dev_ui->font_info->height;
	f32 rwidth  = 1.0f / (f32)font->tex_size.x;
	f32 rheight = 1.0f / (f32)font->tex_size.y;
	Vec3 offset = Vec3(p, 0);
	offset.y += line_height;
	for (const char *scan = text; *scan; ++scan) {
		char symbol = *scan;
		if ((symbol >= font->first_codepoint) && (symbol < font->first_codepoint + font->glyphs.len)) {
			FontGlyph *glyph = &font->glyphs[symbol - font->first_codepoint];
			f32 glyph_width  = (glyph->offset2_x - glyph->offset1_x);
			f32 glyph_height = (glyph->offset2_y - glyph->offset1_y);
			f32 y1 = offset.y + glyph->offset1_y;
			f32 y2 = y1 + glyph_height;
			f32 x1 = offset.x + glyph->offset1_x;
			f32 x2 = x1 + glyph_width;
			f32 s1 = glyph->min_x * rwidth;
			f32 t1 = glyph->min_y * rheight;
			f32 s2 = glyph->max_x * rwidth;
			f32 t2 = glyph->max_y * rheight;
			f32 char_advance = glyph->x_advance;
			offset.x += char_advance;
            push_rect(layout, Rect(x1, y1, x2 - x1, y2 - y1), color, font->texture_id, Rect(s1, t1, s2 - s1, t2 - t1));
		}
	}    
}

static Vec2 get_text_size(DevUILayout *layout, const char *text) {
    AssetInfo *info = layout->dev_ui->font_info;
    FontData *font = layout->dev_ui->font;
    
    size_t count = strlen(text);
    Vec2 result = {};
    for (u32 i = 0; i < count; ++i) {
        char s = text[i];
        if (s >= font->first_codepoint && s < (font->first_codepoint + font->glyphs.len)) {
            FontGlyph *glyph = &font->glyphs[s - font->first_codepoint];
            // FontGlyph *glyph = &glyphs[first_codepoint];
            result.x += glyph->x_advance;
        }
    }
    result.y = info->height;
    return result;
}

static void element_size(DevUILayout *layout, Vec2 size) {
    layout->p.y += size.y + 2.0f;
}

void dev_ui_init(DevUI *dev_ui, Assets *assets) {
    dev_ui->active_id = id_empty();
    dev_ui->font_info = assets->get_info(Asset_Font);
    dev_ui->font = assets->get_font(Asset_Font);
}

DevUILayout dev_ui_begin(DevUI *dev_ui) {
    DevUILayout layout = {};
    layout.dev_ui = dev_ui;
    layout.active_id = dev_ui->active_id;
    layout.temp_mem = temp_memory_begin(&dev_ui->arena);
    layout.max_draw_queue_entry_count = 4096;
    layout.draw_queue = alloc_arr(&dev_ui->arena, layout.max_draw_queue_entry_count, DevUIDrawQueueEntry);
    return layout;
}

void dev_ui_labelv(DevUILayout *layout, const char *format, va_list args) {
    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), format, args);
    push_text(layout, layout->p, buffer);
    element_size(layout, get_text_size(layout, buffer));
}

void dev_ui_labelf(DevUILayout *layout, const char *format, ...) {
    va_list args;
    va_start(args, format);
    dev_ui_labelv(layout, format, args);
    va_end(args);
}

bool dev_ui_button(DevUILayout *layout) {
    return false;    
}

void dev_ui_end(DevUILayout *layout, RenderGroup *render_group) {
    for (size_t i = 0; i < layout->draw_queue_entry_count; ++i) {
        DevUIDrawQueueEntry *entry = layout->draw_queue + i;
        imm_draw_v(render_group, entry->v, entry->tex_id);
    }
    temp_memory_end(layout->temp_mem);
}

