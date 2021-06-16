#include "framework/devui.hh"

#include "game/game.hh"

#define CHECK_CUR_WIN_IS_PRESENT if (!this->cur_win) { logprintln("Devui", "Window is not present at devui widget function call"); assert(false); }
#define CHECK_IS_ENABLED(...) if (!this->is_enabled) { return __VA_ARGS__; }
#define CHECK_WINDOW_IS_NOT_COLLAPSED(...) if (this->cur_win->is_collapsed) { return __VA_ARGS__; }
#define WIDGET_DEF_HEADER(...) CHECK_IS_ENABLED(__VA_ARGS__) CHECK_CUR_WIN_IS_PRESENT CHECK_WINDOW_IS_NOT_COLLAPSED(__VA_ARGS__)
#define HAS_INPUT (this->is_enabled && this->is_focused)

DevUI *dev_ui;

static inline Vec4 color_from_bstate(const DevUIButtonState &bstate, Vec4 held, Vec4 hot, Vec4 idle) {
    return (bstate.is_held ? held : bstate.is_hot ? hot : idle);
}

void DevUI::push_clip_rect(const Rect &rect) {
    assert(clip_rect_stack_index + 1 < ARRAY_SIZE(clip_rect_stack));
    Rect modified_rect = rect;
    if (clip_rect_stack_index) {
        modified_rect = clip_rect_stack[clip_rect_stack_index - 1].clip(rect);
    }
    modified_rect.x -= DEVUI_EPSILON;
    modified_rect.y -= DEVUI_EPSILON;
    modified_rect.w += DEVUI_EPSILON * 2;
    modified_rect.h += DEVUI_EPSILON * 2;
    clip_rect_stack[clip_rect_stack_index] = modified_rect;
    ++clip_rect_stack_index;
}

void DevUI::pop_clip_rect() {
    assert(clip_rect_stack_index);
    --clip_rect_stack_index;
}

void DevUI::push_id(const DevUIID &id) {
    assert(this->id_stack_index + 1 < ARRAY_SIZE(this->id_stack));
    this->id_stack[id_stack_index] = id;
    ++this->id_stack_index;
}

void DevUI::pop_id() {
    assert(this->id_stack_index);
    --this->id_stack_index;
}

DevUIID DevUI::make_id(const char *text, size_t count) {
    DevUIID result = {};
    if (!count) {
        count = strlen(text);
    }
    
    u32 hash_v = crc32(text, count, 0);
    result.s = hash_v;
    if (this->id_stack_index) {
        // @HACK 'join' ids for hierarchy using xors. Just hope nothing collides
        result.p = this->id_stack[id_stack_index - 1].s ^ this->id_stack[id_stack_index - 1].p;
    }
    
    return result;
}

bool DevUI::is_text_input_key_pressed(Key key) {
    bool result = false;
    f32 time_pressed = game->input.keys_down_time[(u32)key];
    if (time_pressed > DEVUI_KEY_REPEAT_DELAY) {
        f32 half_repeate_rate = DEVUI_KEY_REPEAT_RATE * 0.5f;
        f32 press_mod = fmodf(time_pressed - DEVUI_KEY_REPEAT_DELAY, DEVUI_KEY_REPEAT_RATE);
        f32 abs_mod = fmodf(time_pressed - DEVUI_KEY_REPEAT_DELAY - game->input.dt, DEVUI_KEY_REPEAT_RATE);
        if ((press_mod > half_repeate_rate) != (abs_mod > half_repeate_rate)) {
            result = true;
        }
    } else if (time_pressed == 0) {
        result = true;
    }
    
    return result;
}


void DevUI::push_rect(Rect rect, Vec4 color, Texture *tex, Rect uv_rect) {
    CHECK_CUR_WIN_IS_PRESENT;
    // Transform coordinates and uv accoring to clipping
    Rect clip_rect = clip_rect_stack[clip_rect_stack_index - 1];
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

void DevUI::push_text(Vec2 p, const char *text, Vec4 color, f32 scale) {
    // @HACK 
    if (*text == '$') {
        return;
    }
    
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

Vec2 DevUI::get_text_size(const char *text, size_t count) {
    Vec2 size = this->font->get_text_size(text, count, DEVUI_TEXT_SCALE);
    // @HACK
    if (*text == '$') {
        size.x = 0;
    }
    return size;
}

void DevUI::element_size(Vec2 size, Vec2 *adjust_start_offset) {
    CHECK_CUR_WIN_IS_PRESENT;
    DevUIWindow *win = cur_win;
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

void DevUI::same_line(f32 spacing_w) {
    CHECK_CUR_WIN_IS_PRESENT;
    DevUIWindow *win = cur_win;
    if (spacing_w < 0) {
        spacing_w = DEVUI_ITEM_SPACING.x;
    }
    f32 x = win->last_line_cursor.x + spacing_w;
    f32 y = win->last_line_cursor.y;
    win->cursor = Vec2(x, y);
    win->line_height = win->last_line_height;
}

void DevUI::label(const char *label) {
    if (*label != '$') {
        this->same_line();
        this->text(label);
    }
}

void DevUI::begin_frame() {
    assert(this->font);
    // @HACK probably call font function for height 
    // @TODO change this to be set only when font is set
    this->text_height = this->font->size * DEVUI_TEXT_SCALE;
    this->hot_id = DevUIID::empty();
    this->hot_win = 0;
    
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
        for (size_t window_id_idx = 0; window_id_idx < windows_order.len; ++window_id_idx) {
            DevUIWindow &window = windows[windows_order[window_id_idx]];
            renderer->set_renderering_2d(game->input.winsize);
            renderer->set_shader(renderer->standard_shader);
            for (u32 i = 0; i < window.draw_queue.len; ++i) {
                DevUIDrawQueueEntry *entry = &window.draw_queue[i];
                renderer->imm_begin();
                renderer->set_texture(entry->tex);
                renderer->imm_vertex(entry->v[3]);
                renderer->imm_vertex(entry->v[1]);
                renderer->imm_vertex(entry->v[0]);
                renderer->imm_vertex(entry->v[0]);
                renderer->imm_vertex(entry->v[2]);
                renderer->imm_vertex(entry->v[3]);
                renderer->imm_flush();
            }
            window.draw_queue.clear();
        }
    }
}

void DevUI::text(const char *text) {
    WIDGET_DEF_HEADER();
    DevUIWindow *win = cur_win;
    Vec2 text_size = this->get_text_size(text);
    Rect text_rect = Rect(win->cursor, text_size + DEVUI_FRAME_PADDING * 2.0f);
    element_size(text_size, &text_rect.p);
    push_text(text_rect.p, text);
}
void DevUI::textv(const char *format, va_list args) {
    WIDGET_DEF_HEADER();
    char buffer[1024];
    Str::formatv(buffer, sizeof(buffer), format, args);
    text(buffer);
}
void DevUI::textf(const char *format, ...) {
    WIDGET_DEF_HEADER();
    va_list args;
    va_start(args, format);
    textv(format, args);
    va_end(args);
}

bool DevUI::button(const char *label, bool repeat_when_held) {
    WIDGET_DEF_HEADER(false);
    Vec2 text_size = this->get_text_size(label);
    DevUIID id = make_id(label);
    Rect button_rect = Rect(this->cur_win->cursor, text_size + DEVUI_FRAME_PADDING * 2.0f);
    element_size(button_rect.s);
    DevUIButtonState bstate = update_button(button_rect, id, repeat_when_held);
    Vec4 color = color_from_bstate(bstate, DEVUI_COLOR_BUTTON_ACTIVE, DEVUI_COLOR_BUTTON_HOT, DEVUI_COLOR_BUTTON);
    push_rect(button_rect, color);
    // Почему не нужно добавлять y...
    push_text(button_rect.p + Vec2(DEVUI_FRAME_PADDING.x, 0), label);
    return bstate.is_pressed;
}

bool DevUI::checkbox(const char *label, bool *value) {
    assert(value);
    WIDGET_DEF_HEADER(false);
    DevUIID id = make_id(label);
    Rect checkbox_rect = Rect(this->cur_win->cursor, Vec2(this->text_height + DEVUI_FRAME_PADDING.y * 2, 
                                                          this->text_height + DEVUI_FRAME_PADDING.y * 2));
    element_size(checkbox_rect.size());
    
    push_rect(checkbox_rect, DEVUI_COLOR_BUTTON);
    DevUIButtonState checkbox_state = update_button(checkbox_rect, id);
    bool value_changed = false;
    if (checkbox_state.is_pressed) {
        value_changed = true;
        *value = !(*value);
    }
    
    if (*value || checkbox_state.is_hot) {
        Vec4 checkmark_color = (*value ? DEVUI_COLOR_BUTTON_ACTIVE : DEVUI_COLOR_BUTTON_HOT);
        Rect checkmark_rect = Rect(checkbox_rect.p + DEVUI_CHECKMARK_OFFSET, checkbox_rect.size() - DEVUI_CHECKMARK_OFFSET * 2);
        push_rect(checkmark_rect, checkmark_color);
    }
    this->label(label);
    return value_changed;
}

bool DevUI::input_text(const char *label, void *buffer, size_t buffer_size, u32 flags) {
    assert(buffer && buffer_size);
    assert(buffer_size < sizeof(DevUITextEditState::text));
    WIDGET_DEF_HEADER(false);
    DevUIID id = make_id(label);
    Rect frame_rect = Rect(this->cur_win->cursor, Vec2(this->cur_win->item_width, this->text_height) + DEVUI_FRAME_PADDING * 2);
    element_size(frame_rect.size());
    
    bool is_ctrl_down = game->input.is_key_held(Key::Ctrl);
    bool is_shift_down = game->input.is_key_held(Key::Shift);
    // @TODO make this use update_button or something
    bool is_hot = this->hot_win == this->cur_win && !hot_id && frame_rect.collide(game->input.mpos);
    if (is_hot) {
        this->hot_id = id;
    }
    
    DevUITextEditState *te = &this->text_edit;
    if (game->input.is_key_pressed(Key::MouseLeft)) {
        if (is_hot) {
            if (this->active_id != id) {
                memset(te, 0, sizeof(*te));
                strncpy(te->text, (const char *)buffer, sizeof(te->text));
                strncpy(te->initial_text, (const char *)buffer, sizeof(te->text));
                te->max_length = Math::min(sizeof(te->text), buffer_size);
            }
            this->active_id = id;
        } else if (this->active_id == id) {
            this->active_id = DevUIID::empty();
        }
    }
    
    bool is_value_changed = false;
    bool is_enter_pressed = false;
    if (active_id == id) {
        size_t length = strlen(te->text);
        if (is_text_input_key_pressed(Key::Enter)) {
            is_enter_pressed = true;
        } else if (is_text_input_key_pressed(Key::Escape)) {
            this->active_id = DevUIID::empty();
        } else if (is_text_input_key_pressed(Key::Backspace)) {
            if (te->cursor) {
                memmove(te->text + te->cursor - 1, te->text + te->cursor, length - te->cursor);
                te->text[length - 1] = 0;
                --te->cursor;
            }
        } else if (is_text_input_key_pressed(Key::Delete)) {
            if (te->cursor < length) {
                memmove(te->text + te->cursor, te->text + te->cursor + 1, length - te->cursor);
                te->text[length] = 0;
            }
        } else if (is_text_input_key_pressed(Key::Home)) {
            te->cursor = 0;
        } else if (is_text_input_key_pressed(Key::End)) {
            te->cursor = length;
        } else if (is_text_input_key_pressed(Key::Left)) {
            if (te->cursor) {
                --te->cursor;
            }
        } else if (is_text_input_key_pressed(Key::Right)) {
            if (te->cursor < length) {
                ++te->cursor;
            }
        } else if (game->input.utf32) {
            bool is_valid = true;
            if (flags & DEVUI_INPUT_FLAG_DECIMAL) {
                u32 c = game->input.utf32;
                is_valid = (isdigit(c) || c == '.' || c == '-' || c == '+');
            }
            
            if (is_valid) {
                char *buffer_end = te->text + te->max_length;
                if (buffer_end - (te->text + length + 1) >= 1) {
                    memmove(te->text + te->cursor + 1, te->text + te->cursor, length - te->cursor);
                    te->text[te->cursor] = game->input.utf32;
                    ++te->cursor;
                }
            }
        }
        if (this->active_id != id) {
            // memcpy(buffer, te->initial_text, te->max_length);
            memcpy(te->text, te->initial_text, te->max_length);
            // is_value_changed = true;
        } else if (is_enter_pressed) {
            memcpy(buffer, te->text, te->max_length);
            is_value_changed = true;
            this->active_id = DevUIID::empty();
        }
    }
    
    this->push_rect(frame_rect, DEVUI_COLOR_WIDGET_BACKGROUND);
    if (active_id == id) {
        Vec2 pre_cursor_text_size = this->get_text_size(te->text, te->cursor);
        // @CLEAN
        if (!te->cursor) {
            pre_cursor_text_size.x = 0;
        } 
        Vec2 cursor_pos = frame_rect.p + DEVUI_FRAME_PADDING + Vec2(pre_cursor_text_size.x, 0);
        Rect cursor_rect = Rect(cursor_pos, Vec2(DEVUI_TEXT_CURSOR_WIDTH, pre_cursor_text_size.y));
        this->push_rect(cursor_rect, Colors::white);
    }
    this->push_clip_rect(frame_rect);
    const char *text_to_draw = (const char *)buffer;
    if (this->active_id == id) {
        text_to_draw = te->text;
    }
    this->push_text(frame_rect.p + Vec2(DEVUI_FRAME_PADDING.x, 0), text_to_draw);    
    this->pop_clip_rect();
    this->label(label);
    return is_value_changed;
}


bool DevUI::input_float(const char *label, f32 *value) {
    WIDGET_DEF_HEADER(false);
    
    bool is_value_changed = false;
    char buffer[64];
    Str::format(buffer, sizeof(buffer), "%.3f", *value);
    if (input_text(label, buffer, sizeof(buffer), DEVUI_INPUT_FLAG_DECIMAL)) {
        *value = atof(buffer);
        is_value_changed = true;
    }
    
    return is_value_changed;
}

bool DevUI::slider_float(const char *label, f32 *value, f32 minv, f32 maxv) {
    assert(maxv > minv);
    WIDGET_DEF_HEADER(false);
    DevUIID id = this->make_id(label);
    Rect frame_rect = Rect(this->cur_win->cursor, Vec2(this->cur_win->item_width, this->text_height) + DEVUI_FRAME_PADDING * 2);
    Rect slider_zone_rect = Rect(frame_rect.p + DEVUI_FRAME_PADDING, frame_rect.size() - DEVUI_FRAME_PADDING * 2);
    
    f32 slider_workzone_width = slider_zone_rect.w - DEVUI_SLIDER_GRAB_WIDTH;
    f32 slider_workzone_min_x = slider_zone_rect.x + DEVUI_SLIDER_GRAB_WIDTH * 0.5f;
    f32 slider_workzone_max_x = slider_workzone_min_x + slider_workzone_width;
    DevUIButtonState slider_state = this->update_button(slider_zone_rect, id, true);
    this->element_size(frame_rect.size());
    
    bool is_value_changed = false;
    if (slider_state.is_held) {
        f32 slider_pos = Math::clamp((game->input.mpos.x - slider_workzone_min_x) / slider_workzone_width, 0.0f, 1.0f);
        f32 new_value = Math::lerp(minv, maxv, slider_pos);
        if (*value != new_value) {
            *value = new_value;
            is_value_changed = true;
        }
    }
    
    f32 grab_pos_normalized = (*value - minv) / (maxv - minv);
    f32 grab_x = Math::lerp(slider_workzone_min_x, slider_workzone_max_x, grab_pos_normalized);
    Rect grab_rect = Rect(Vec2(grab_x - DEVUI_SLIDER_GRAB_WIDTH * 0.5f, slider_zone_rect.y),
                          Vec2(DEVUI_SLIDER_GRAB_WIDTH, slider_zone_rect.h));
    Vec4 color = color_from_bstate(slider_state, DEVUI_COLOR_BUTTON_ACTIVE, DEVUI_COLOR_BUTTON_HOT, DEVUI_COLOR_BUTTON);
    
    this->push_rect(frame_rect, DEVUI_COLOR_WIDGET_BACKGROUND);
    this->push_rect(grab_rect, color);
    
    char buffer[64];
    Str::format(buffer, sizeof(buffer), "%.3f", *value);
    Vec2 value_size = this->get_text_size(buffer);
    Vec2 value_pos = Vec2(slider_zone_rect.middle_x() - value_size.x * 0.5f, frame_rect.y);
    this->push_text(value_pos, buffer);
    this->label(label);
    return is_value_changed;
}

bool DevUI::drag_float(const char *label, f32 *value, f32 speed) {
    WIDGET_DEF_HEADER(false);
    DevUIID id = this->make_id(label);
    Rect frame_rect = Rect(this->cur_win->cursor, Vec2(this->cur_win->item_width, this->text_height) + DEVUI_FRAME_PADDING * 2);
    Rect slider_zone_rect = Rect(frame_rect.p + DEVUI_FRAME_PADDING, frame_rect.size() - DEVUI_FRAME_PADDING * 2);
    element_size(frame_rect.size());
    
    bool is_value_changed = false;
    DevUIButtonState slider_state = this->update_button(slider_zone_rect, id, true);
    if (slider_state.is_held) {
        f32 new_value = *value + game->input.mdelta.x * speed;
        if (*value != new_value) {
            *value = new_value;
            is_value_changed = true;
        }
    }
    
    this->push_rect(frame_rect, DEVUI_COLOR_WIDGET_BACKGROUND);
    this->push_clip_rect(frame_rect);
    char buffer[64];
    Str::format(buffer, sizeof(buffer), "%.3f", *value);
    Vec2 value_size = this->get_text_size(buffer);
    Vec2 value_pos = Vec2(slider_zone_rect.middle_x() - value_size.x * 0.5f, frame_rect.y);
    this->push_text(value_pos, buffer);
    this->pop_clip_rect();
    this->label(label);
    return is_value_changed;
}

void DevUI::value(const char *label, f32 value) {
    WIDGET_DEF_HEADER();
    this->textf("%s: %.2f", label, value);
}

void DevUI::value(const char *label, Vec2 value) {
    WIDGET_DEF_HEADER();
    this->textf("%s: %.2f, %.2f", label, value.x, value.y);
}

void DevUI::value(const char *label, Vec3 value) {
    WIDGET_DEF_HEADER();
    this->textf("%s: %.2f, %.2f, %.2f", label, value.x, value.y, value.z);
}

bool DevUI::drag_float3(const char *label, f32 value[3], f32 speed) {
    WIDGET_DEF_HEADER(false);
    DevUIID id = this->make_id(label);
    this->push_id(id);
    this->cur_win->item_width = (this->cur_win->item_width - (DEVUI_FRAME_PADDING.x * 2.0f + DEVUI_ITEM_SPACING.x) * 2) / 3.0f;
    bool is_x_changed = this->drag_float("$X", value + 0, speed);
    same_line();
    bool is_y_changed = this->drag_float("$Y", value + 1, speed);
    same_line();
    bool is_z_changed = this->drag_float("$Z", value + 2, speed);
    this->cur_win->item_width = this->cur_win->default_item_width;
    this->label(label);
    this->pop_id();
    return is_x_changed || is_y_changed || is_z_changed;
}

DevUIButtonState DevUI::update_button(Rect rect, DevUIID id, bool repeat_when_held) {
    if (!HAS_INPUT) {
        return {};
    }
    
    CHECK_CUR_WIN_IS_PRESENT;
    DevUIWindow *win = cur_win;
    bool is_hot = (this->hot_win == win) && !this->hot_id && rect.collide(game->input.mpos);
    if (is_hot) {
        this->hot_id = id;
        if (game->input.is_key_held(Key::MouseLeft) && !this->active_id) {
            this->active_id = id;
        } 
    }
    
    bool is_pressed = false;
    bool is_held = false;
    if (this->active_id == id) {
        if (game->input.is_key_held(Key::MouseLeft)) {
            is_held = true;
            if (repeat_when_held && is_hot) {
                is_pressed = true;
            }
        } else  { 
            if (is_hot) {
                is_pressed = true;
            }
            this->active_id = DevUIID::empty();
        }
    }
    
    DevUIButtonState result;
    result.is_hot = is_hot;
    result.is_held = is_held;
    result.is_pressed = is_pressed;
    return result;
}

Rect DevUI::get_new_window_rect() {
    // @TODO some algorithm for creating rects that dont collide with previous
    Rect rects[] = {
        Rect(0, 0, 400, 400),
        Rect(450, 0, 400, 400),
        Rect(0, 450, 400, 400),
        Rect(450, 450, 400, 400),
    };
    assert(this->windows.len <= ARRAY_SIZE(rects));
    return rects[this->windows.len - 1];
}
    
void DevUI::window(const char *title) {
    CHECK_IS_ENABLED();
    assert(!this->cur_win);
    
    size_t title_len = strlen(title);
    assert(title_len < sizeof(DevUIWindow::title));
    
    DevUIWindow *win = 0;
    bool use_new_slot = true;
    for (u32 i = 0; i < windows.len; ++i) {
        DevUIWindow *test_win = &windows[i];
        if (test_win->title.cmp(title)) {
            assert(!win);
            win = test_win;
            use_new_slot = false;
        }
    }
    
    if (use_new_slot) {
        assert(!win);
        win = &windows[windows.add(DevUIWindow())];
        win->id = make_id(title, title_len);
        win->array_idx = windows.len - 1;
        win->title = Str(title);
        Rect rect = this->get_new_window_rect();
        win->whole_rect = rect;
        
        windows_order.add(win->array_idx);
    }
    this->cur_win = win;
    push_id(win->id);
    
    DevUIID move_id = make_id("$MOVE");
    if (this->active_id == move_id) {
        if (game->input.is_key_held(Key::MouseLeft) && HAS_INPUT) {
            win->whole_rect = Rect::move(win->whole_rect, game->input.mdelta);
        } else {
            this->active_id = DevUIID::empty();
        }
    }
    
    DevUIID resize_id = make_id("$RESIZE");
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
    
    DevUIID collapse_id = make_id("$COLLAPSE");
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
    push_text(win->title_bar_rect.p + Vec2(DEVUI_FRAME_PADDING.x, 0), win->title.data);
    push_rect(collapse_rect, collapse_color);
    
    win->cursor = win->rect.p + DEVUI_WINDOW_PADDING;
    Rect widget_zone = Rect(win->cursor, win->rect.size() - DEVUI_WINDOW_PADDING * 2);
    push_clip_rect(widget_zone);
    win->item_width = win->default_item_width = widget_zone.w * 0.65f;
}

void DevUI::window_end() {
    CHECK_IS_ENABLED();    
    CHECK_CUR_WIN_IS_PRESENT;
    // @CLEAN
    if (!active_id && !hot_id && this->windows_order[this->windows_order.len - 1] == cur_win->array_idx && cur_win->title_bar_rect.collide(game->input.mpos)
        && game->input.is_key_held(Key::MouseLeft) && HAS_INPUT) {
        active_id = make_id("$MOVE");
    }
    cur_win = 0;
    
    pop_id();
    pop_clip_rect(); // widget zone
    pop_clip_rect(); // window
}

void DevUI::init() {
    assert(!::dev_ui);
    ::dev_ui = this;
}