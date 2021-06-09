#include "game/devui.hh"

#include "game/game.hh"

#define CHECK_CUR_WIN_IS_PRESENT if (!this->cur_win) { logprintln("Devui", "Window is not present at devui widget function call"); assert(false); }
#define CHECK_IS_ENABLED(...) if (!this->is_enabled) { return __VA_ARGS__; }
#define CHECK_WINDOW_IS_NOT_COLLAPSED(...) if (this->cur_win->is_collapsed) { return __VA_ARGS__; }
#define WIDGET_DEF_HEADER(...) CHECK_IS_ENABLED(__VA_ARGS__) CHECK_CUR_WIN_IS_PRESENT CHECK_WINDOW_IS_NOT_COLLAPSED(__VA_ARGS__)
#define HAS_INPUT (this->is_enabled && this->is_focused)

static inline Vec4 color_from_bstate(const DevUIButtonState &bstate, Vec4 held, Vec4 hot, Vec4 idle) {
    return (bstate.is_held ? held : bstate.is_hot ? hot : idle);
}

void DevUI::push_clip_rect(const Rect &rect) {
    assert(clip_rect_stack_index + 1 < ARRAY_SIZE(clip_rect_stack));
    Rect modified_rect = rect;
    modified_rect.x -= DEVUI_EPSILON;
    modified_rect.y -= DEVUI_EPSILON;
    modified_rect.w += DEVUI_EPSILON * 2;
    modified_rect.h += DEVUI_EPSILON * 2;
    ++clip_rect_stack_index;
    clip_rect_stack[clip_rect_stack_index] = modified_rect;
}

void DevUI::pop_clip_rect() {
    assert(clip_rect_stack_index);
    --clip_rect_stack_index;
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

void DevUI::push_rect(Rect rect, Vec4 color, Texture *tex, Rect uv_rect) {
    CHECK_CUR_WIN_IS_PRESENT;
    // Transform coordinates and uv accoring to clipping
    Rect clip_rect = clip_rect_stack[clip_rect_stack_index];
    if (!clip_rect.collide(rect)) {
        return;
    }
    
    Rect clipped = clip_rect.clip(rect);
    Rect clipped_uv = {};
    clipped_uv.x = uv_rect.x + (clipped.x - rect.x) / rect.w;
    clipped_uv.y = uv_rect.y + (clipped.y - rect.y) / rect.h;
    clipped_uv.w = uv_rect.right() + (clipped.right() - rect.right()) / rect.w * uv_rect.w - clipped_uv.x;
    clipped_uv.h = uv_rect.bottom() + (clipped.bottom() - rect.bottom()) / rect.h * uv_rect.h - clipped_uv.y;
    
    DevUIDrawQueueEntry entry = {};
    entry.v[0].p = Vec3(clipped.top_left());
    entry.v[0].uv = clipped_uv.top_left();
    entry.v[0].c = color;
    entry.v[1].p = Vec3(clipped.top_right());
    entry.v[1].uv = clipped_uv.top_right();
    entry.v[1].c = color;
    entry.v[2].p = Vec3(clipped.bottom_left());
    entry.v[2].uv = clipped_uv.bottom_left();
    entry.v[2].c = color;
    entry.v[3].p = Vec3(clipped.bottom_right());
    entry.v[3].uv = clipped_uv.bottom_right();
    entry.v[3].c = color;
    entry.tex = tex;
    this->cur_win->draw_queue.add(entry);
}

void DevUI::push_text(Vec2 p, Vec4 color, const char *text, Font *font, f32 scale) {
    f32 line_height = font->size * scale;
	f32 rwidth  = 1.0f / (f32)font->tex->size.x;
	f32 rheight = 1.0f / (f32)font->tex->size.y;
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
			f32 char_advance = glyph->x_advance * scale;
			offset.x += char_advance;
            push_rect(Rect(x1, y1, x2 - x1, y2 - y1), color, font->tex, Rect(s1, t1, s2 - s1, t2 - t1));
		}
	}    
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

void DevUI::begin_frame() {
    hot_id = DevUIID::empty();
    hot_win = 0;
    
    if (HAS_INPUT) {
        for (size_t window_id_idx = 0; window_id_idx < windows_order.len; ++window_id_idx) {
            DevUIWindow *window = &windows[windows_order[window_id_idx]];
            if (window->whole_rect.collide(game->input.mpos)) {
                hot_win = window;
            }
        }
 
        if (hot_win) {
            if (game->input.is_key_pressed(Key::MouseLeft)) {
                windows_order.remove(hot_win->array_idx);
                windows_order.add(hot_win->array_idx);
            }
        }
    }
}

void DevUI::end_frame() {
    if (!this->is_enabled) {
        // assert(draw_queue.len == 0);
    } else {
        // for (i64 window_id_idx = windows_order.len - 1; window_id_idx >= 0; --window_id_idx) {
        for (size_t window_id_idx = 0; window_id_idx < windows_order.len; ++window_id_idx) {
            DevUIWindow &window = windows[windows_order[window_id_idx]];
            game->renderer.set_renderering_2d(game->input.winsize);
            game->renderer.set_shader(game->renderer.standard_shader);
            for (u32 i = 0; i < window.draw_queue.len; ++i) {
                DevUIDrawQueueEntry *entry = &window.draw_queue[i];
                game->renderer.immediate_begin();
                game->renderer.set_texture(entry->tex);
                game->renderer.immediate_vertex(entry->v[3]);
                game->renderer.immediate_vertex(entry->v[1]);
                game->renderer.immediate_vertex(entry->v[0]);
                game->renderer.immediate_vertex(entry->v[0]);
                game->renderer.immediate_vertex(entry->v[2]);
                game->renderer.immediate_vertex(entry->v[3]);
                game->renderer.immediate_flush();
            }
            window.draw_queue.clear();
        }
    }
}

void DevUI::text(char *text) {
    WIDGET_DEF_HEADER();
    DevUIWindow *win = cur_win;
    Vec2 text_size = font->get_text_size(text, 0, DEVUI_TEXT_SCALE);
    Rect text_rect = Rect(win->cursor, text_size + DEVUI_FRAME_PADDING * 2.0f);
    element_size(text_size, &text_rect.p);
    push_text(text_rect.p, DEVUI_COLOR_TEXT, text, font, DEVUI_TEXT_SCALE);
}
void DevUI::textv(char *format, va_list args) {
    WIDGET_DEF_HEADER();
    char buffer[1024];
    Str::formatv(buffer, sizeof(buffer), format, args);
    text(buffer);
}
void DevUI::textf(char *format, ...) {
    WIDGET_DEF_HEADER();
    va_list args;
    va_start(args, format);
    textv(format, args);
    va_end(args);
}

bool DevUI::button(char *label, bool repeat_when_held) {
    WIDGET_DEF_HEADER(false);
    DevUIWindow *win = cur_win;
    Vec2 text_size = font->get_text_size(label, 0, DEVUI_TEXT_SCALE);
    Vec2 size = text_size;
    DevUIID id = make_id(win, label);
    Rect button_rect = Rect(win->cursor, size + DEVUI_FRAME_PADDING * 2.0f);
    element_size(button_rect.s, 0);
    DevUIButtonState bstate = update_button(button_rect, id, repeat_when_held);
    Vec4 color = color_from_bstate(bstate, DEVUI_COLOR_BUTTON_ACTIVE, DEVUI_COLOR_BUTTON_HOT, DEVUI_COLOR_BUTTON);
    push_rect(button_rect, color);
    // Почему не нужно добавлять y...
    push_text(button_rect.p + Vec2(DEVUI_FRAME_PADDING.x, 0), DEVUI_COLOR_TEXT, label, font, DEVUI_TEXT_SCALE);
    return bstate.is_pressed;
}

DevUIButtonState DevUI::update_button(Rect rect, DevUIID id, bool repeat_when_held) {
    if (!HAS_INPUT) {
        DevUIButtonState result = {};
        return result;
    }
    
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
            active_id = DevUIID::empty();
        }
    }
    
    DevUIButtonState result;
    result.is_hot = is_hot;
    result.is_held = is_held;
    result.is_pressed = is_pressed;
    return result;
}

    
void DevUI::window(char *title, Rect rect) {
    CHECK_IS_ENABLED();
    
    size_t title_len = strlen(title);
    assert(title_len < sizeof(DevUIWindow::title));
    
    DevUIWindow *win = 0;
    bool use_new_slot = true;
    for (u32 i = 0; i < windows.len; ++i) {
        DevUIWindow *test_win = &windows[i];
        if (!Str::cmp(title, test_win->title)) {
            win = test_win;
            use_new_slot = false;
        }
    }
    
    if (use_new_slot) {
        assert(!win);
        win = &windows[windows.add(DevUIWindow())];
        win->id = make_id(0, title, title_len);
        win->array_idx = windows.len - 1;
        memcpy(win->title, title, title_len);
        win->whole_rect = rect;
        
        windows_order.add(win->array_idx);
    }
    this->cur_win = win;
    
    DevUIID move_id = make_id(win, "$MOVE$");
    if (this->active_id == move_id) {
        if (game->input.is_key_held(Key::MouseLeft) && HAS_INPUT) {
            win->whole_rect = Rect::move(win->whole_rect, game->input.mdelta);
        } else {
            this->active_id = DevUIID::empty();
        }
    }
    
    DevUIID resize_id = make_id(win, "$RESIZE$");
    Rect resize_rect = Rect(win->whole_rect.bottom_right() - DEVUI_RESIZE_SIZE, DEVUI_RESIZE_SIZE);
    DevUIButtonState resize_state = update_button(resize_rect, resize_id, true);
    Vec4 resize_color = color_from_bstate(resize_state, DEVUI_COLOR_BUTTON_ACTIVE, DEVUI_COLOR_BUTTON_HOT, DEVUI_COLOR_BUTTON); 
    if (resize_state.is_held) {
        Vec2 size_change = Vec2(Math::max(win->whole_rect.w + game->input.mdelta.x, DEVUI_MIN_WINDOW_SIZE.x), 
                                Math::max(win->whole_rect.h + game->input.mdelta.y, DEVUI_MIN_WINDOW_SIZE.y))
            - win->whole_rect.s;
        resize_rect.p += size_change;
        win->whole_rect.s += size_change;
    }
    
    win->title_bar_rect = Rect(win->whole_rect.p, Vec2(win->whole_rect.w, DEVUI_WINDOW_TITLE_BAR_HEIGHT));
    win->rect = Rect(win->whole_rect.x, win->whole_rect.y + DEVUI_WINDOW_TITLE_BAR_HEIGHT, 
                     win->whole_rect.w, win->whole_rect.h - DEVUI_WINDOW_TITLE_BAR_HEIGHT);
    
    DevUIID collapse_id = make_id(win, "$COLLAPSE$");
    Rect collapse_rect = Rect(Vec2(win->title_bar_rect.right() - DEVUI_COLLAPSE_RECT_SIZE.x, win->title_bar_rect.y), DEVUI_COLLAPSE_RECT_SIZE);
    DevUIButtonState collapse_state = update_button(collapse_rect, collapse_id);
    Vec4 collapse_color = color_from_bstate(collapse_state, DEVUI_COLOR_BUTTON_ACTIVE, DEVUI_COLOR_BUTTON_HOT, DEVUI_COLOR_BUTTON);
    if (collapse_state.is_pressed) {
        win->is_collapsed = !win->is_collapsed;
    }
    
    assert(!clip_rect_stack_index);
    push_clip_rect(win->whole_rect);
    
    if (!win->is_collapsed) {
        push_rect(win->rect, DEVUI_COLOR_WINDOW_BACKGROUND);
        push_rect(resize_rect, resize_color);
    }
    push_rect(win->title_bar_rect, DEVUI_COLOR_WINDOW_TITLEBAR);
    push_text(win->title_bar_rect.p + Vec2(DEVUI_FRAME_PADDING.x, 0), DEVUI_COLOR_TEXT, win->title, font, DEVUI_TEXT_SCALE);
    push_rect(collapse_rect, collapse_color);
    
    win->cursor = win->rect.p + DEVUI_WINDOW_PADDING;
    Rect widget_zone = Rect(win->cursor, win->rect.size() - DEVUI_WINDOW_PADDING * 2);
    push_clip_rect(widget_zone);
}

void DevUI::window_end() {
    CHECK_IS_ENABLED();    
    CHECK_CUR_WIN_IS_PRESENT;
    if (!active_id && !hot_id && cur_win->title_bar_rect.collide(game->input.mpos)
        && game->input.is_key_held(Key::MouseLeft) && HAS_INPUT) {
        active_id = make_id(cur_win, "$MOVE$");
    }
    cur_win = 0;
    
    pop_clip_rect(); // widget zone
    pop_clip_rect(); // window
}