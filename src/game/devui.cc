#include "devui.hh"

const DevUIID EMPTY_ID { };

#define CHECK_CUR_WIN_IS_PRESENT if (!cur_win) { printf("Current window is not present at devui widget function call"); assert(false); }

void DevUI::draw_quad(Vec3 v0, Vec3 v1, Vec3 v2, Vec3 v3, 
                      Vec4 c0, Vec4 c1, Vec4 c2, Vec4 c3,
                      Vec2 st0, Vec2 st1, Vec2 st2, Vec2 st3,
                      RendererTexture tex) {
    DevUIDrawQueueEntry entry;
    entry.v[0] = v0;
    entry.v[1] = v1;
    entry.v[2] = v2;
    entry.v[3] = v3;
    entry.c[0] = c0;
    entry.c[1] = c1;
    entry.c[2] = c2;
    entry.c[3] = c3;
    entry.uvs[0] = st0;
    entry.uvs[1] = st1;
    entry.uvs[2] = st2;
    entry.uvs[3] = st3;
    entry.tex = tex;
    draw_queue.add(entry);
}

void DevUI::draw_rect(Rect rect, Vec4 color, RendererTexture tex) {
    Vec3 rect_pts[4];
    rect.store_points(rect_pts);
    draw_quad(rect_pts[0], rect_pts[1], rect_pts[2], rect_pts[3],
              color, color, color, color, 
              Vec2(0, 0), Vec2(0, 1), Vec2(1, 0), Vec2(0, 1),
              tex);
}

void DevUI::draw_text(Vec2 p, Vec4 c, char *text, Font *font, f32 scale) {
    f32 line_height = font->height * scale;

	f32 rwidth  = 1.0f / (f32)font->atlas.w;
	f32 rheight = 1.0f / (f32)font->atlas.h;

	Vec3 offset = Vec3(p, 0);
	offset.y += line_height;

	for (char *scan = text; *scan; ++scan) {
		char symbol = *scan;

		if ((symbol >= font->first_codepoint) && (symbol < font->first_codepoint + font->glyphs.size)) {
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

			draw_quad(Vec3(x1, y1, 0), Vec3(x1, y2, 0), Vec3(x2, y1, 0), Vec3(x2, y2, 0),
				 c, c, c, c,
				 Vec2(s1, t1), Vec2(s1, t2), Vec2(s2, t1), Vec2(s2, t2),
				 font->atlas);

			f32 char_advance = glyph->x_advance * scale;
			offset.x += char_advance;
		}
	}
}

DevUIID DevUI::make_id(DevUIWindow *win, char *text, size_t count) {
    DevUIID result = {};
    if (!count) {
        count = strlen(text);
    }
    
    if (!win) {
        u32 hash_v = crc32(text, count, 0);
        result.p = hash_v;
    } else {
        u32 hash_v = crc32(text, count, 0);
        result.p = win->id.p;
        result.s = hash_v;
    }
    
    return result;
}

void DevUI::element_size(Vec2 size, Vec2 *adjust_start_offset) {
    DevUIWindow *win = cur_win;
    CHECK_CUR_WIN_IS_PRESENT;
    f32 line_height = fmaxf(size.y, win->line_height);
    if (adjust_start_offset) {
        adjust_start_offset->y += (line_height - size.y) * 0.5f;
    }
    win->last_line_cursor = Vec2(win->cursor.x + size.x, win->cursor.y);
    win->cursor = Vec2(win->rect.x + DEVUI_WINDOW_PADDING.x,
                       win->cursor.y + line_height + DEVUI_ITEM_SPACING.y);
    win->last_line_height = line_height;
    win->line_height = 0;
}

DevUI::DevUI() {
    active_id = EMPTY_ID;
    hot_id = EMPTY_ID;
    cur_win = 0;
    hot_win = 0;
}

void DevUI::begin_frame() {
    draw_queue.clear();
    
    hot_id = EMPTY_ID;
    hot_win = 0;
    for (u32 i = 0; i < windows.size; ++i) {
        DevUIWindow *window = &windows[i];
        if (window->whole_rect.collide(game->input.mpos)) {
            hot_win = window;
        }
    }
}

void DevUI::end_frame() {
    for (u32 i = 0; i < draw_queue.size; ++i) {
        DevUIDrawQueueEntry *entry = &draw_queue[i];
        game->renderer.quad(entry->v[0], entry->v[1], entry->v[2], entry->v[3],
                            entry->c[0], entry->c[1], entry->c[2], entry->c[3],
                            entry->uvs[0], entry->uvs[1], entry->uvs[2], entry->uvs[3],
                            entry->tex);
    }
}

void DevUI::text(char *text) {
    DevUIWindow *win = cur_win;
    CHECK_CUR_WIN_IS_PRESENT;
    Vec2 text_size = font->get_text_size(text, 0, DEVUI_TEXT_SCALE);
    Rect text_rect = Rect(win->cursor, text_size + DEVUI_FRAME_PADDING * 2.0f);
    element_size(text_size, &text_rect.p);
    draw_text(text_rect.p, DEVUI_COLOR_TEXT, text, font, DEVUI_TEXT_SCALE);
}
void DevUI::textv(char *format, va_list args) {
    char buffer[1024];
    Str::formatv(buffer, sizeof(buffer), format, args);
    text(buffer);
}
void DevUI::textf(char *format, ...) {
    va_list args;
    va_start(args, format);
    textv(format, args);
    va_end(args);
}

bool DevUI::button(char *label, bool repeat_when_held) {
    DevUIWindow *win = cur_win;
    CHECK_CUR_WIN_IS_PRESENT;
    Vec2 text_size = font->get_text_size(label, 0, DEVUI_TEXT_SCALE);
    Vec2 size = text_size;
    DevUIID id = make_id(win, label);
    Rect button_rect = Rect(win->cursor, size + DEVUI_FRAME_PADDING * 2.0f);
    element_size(button_rect.s, 0);
    DevUIButtonState bstate = update_button(button_rect, id, repeat_when_held);
    Vec4 color = (bstate.is_held ? DEVUI_COLOR_BUTTON_ACTIVE :
        bstate.is_hot ? DEVUI_COLOR_BUTTON_HOT :
        DEVUI_COLOR_BUTTON);
    draw_rect(button_rect, color);
    draw_text(button_rect.p + DEVUI_FRAME_PADDING, DEVUI_COLOR_TEXT, label, font, DEVUI_TEXT_SCALE);
    return bstate.is_pressed;
}

DevUIButtonState DevUI::update_button(Rect rect, DevUIID id, bool repeat_when_held) {
    DevUIWindow *win = cur_win;
    CHECK_CUR_WIN_IS_PRESENT;
    bool is_hot = (hot_win == win) && !hot_id && rect.collide(game->input.mpos);
    bool is_pressed = false;
    if (is_hot) {
        hot_id = id;
        if (game->input.is_key_held(Key::MouseLeft)) {
            active_id = id;
        } else if (repeat_when_held && game->input.is_key_held(Key::MouseLeft) && active_id == id) {
            is_pressed = true;
        }
    }
    bool is_held = false;
    if (active_id == id) {
        if (game->input.is_key_held(Key::MouseLeft)) {
            is_held = true;
        } else {
            if (is_hot) {
                is_pressed = true;
            }
            active_id = EMPTY_ID;
        }
    }
    
    DevUIButtonState result;
    result.is_hot = is_hot;
    result.is_held = is_held;
    result.is_pressed = is_pressed;
    return result;
}

    
void DevUI::window(char *title, Rect rect) {
    size_t title_len = strlen(title);
    assert(title_len < sizeof(DevUIWindow::title));
    
    DevUIWindow *win = 0;
    bool used_new_slot = true;
    for (u32 i = 0; i < windows.size; ++i) {
        DevUIWindow *test_win = &windows[i];
        if (!Str::cmp(title, test_win->title)) {
            win = test_win;
            used_new_slot = false;
        }
    }
    
    if (!win) {
        win = windows.add({});
    }
    
    cur_win = win;
    if (used_new_slot) {
        memcpy(win->title, title, title_len);
        win->whole_rect = rect;
        win->id = make_id(0, title, title_len);
    }
    
    DevUIID move_id = make_id(win, "$MOVE$");
    if (active_id == move_id) {
        if (game->input.is_key_held(Key::MouseLeft)) {
            win->whole_rect = Rect::move(win->whole_rect, game->input.mdelta);
        } else {
            active_id = EMPTY_ID;
        }
    }
    
    DevUIID resize_id = make_id(win, "$RESIZE$");
    Rect resize_rect = Rect(win->whole_rect.bottom_right() - DEVUI_RESIZE_SIZE, DEVUI_RESIZE_SIZE);
    DevUIButtonState bstate = update_button(resize_rect, resize_id, true);
    Vec4 resize_color = (bstate.is_held ? DEVUI_COLOR_BUTTON_ACTIVE :
        bstate.is_hot  ? DEVUI_COLOR_BUTTON_HOT :
        DEVUI_COLOR_BUTTON);
    if (bstate.is_held) {
        Vec2 size_change = Vec2(fmaxf(win->whole_rect.w + game->input.mdelta.x, DEVUI_MIN_WINDOW_SIZE.x), 
                                fmaxf(win->whole_rect.h + game->input.mdelta.y, DEVUI_MIN_WINDOW_SIZE.y))
             - win->whole_rect.s;
        resize_rect.p += size_change;
        win->whole_rect.s += size_change;
        // win->whole_rect.normalize();
    }
    
    win->title_bar_rect = Rect(win->whole_rect.p, Vec2(win->whole_rect.w, DEVUI_WINDOW_TITLE_BAR_HEIGHT));
    win->rect = Rect(win->whole_rect.x, win->whole_rect.y + DEVUI_WINDOW_TITLE_BAR_HEIGHT, 
                     win->whole_rect.w, win->whole_rect.h - DEVUI_WINDOW_TITLE_BAR_HEIGHT);
    draw_rect(win->title_bar_rect, DEVUI_COLOR_WINDOW_TITLEBAR);
    draw_rect(win->rect, DEVUI_COLOR_WINDOW_BACKGROUND);
    draw_text(win->title_bar_rect.p + Vec2(2, 0), DEVUI_COLOR_TEXT, win->title, font, DEVUI_TEXT_SCALE);
    draw_quad(Vec3(resize_rect.top_right()), Vec3(resize_rect.bottom_left()),
              Vec3(resize_rect.top_right()), Vec3(resize_rect.bottom_right()),
              resize_color, resize_color, resize_color, resize_color);
    
    win->cursor = win->rect.p + DEVUI_WINDOW_PADDING;
}

void DevUI::window_end() {
    CHECK_CUR_WIN_IS_PRESENT;
    if (!active_id && !hot_id && cur_win->title_bar_rect.collide(game->input.mpos)
        && game->input.is_key_held(Key::MouseLeft)) {
        active_id = make_id(cur_win, "$MOVE$");
    }
    cur_win = 0;
}