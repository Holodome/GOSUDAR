#include "dev_ui.hh"

#include "game.hh"

#define DEV_UI_SECTION_OFFSET 20.0f
#define DEV_UI_VERT_PADDING 2.0f
#define DEV_UI_HORIZ_PADDING 10.0f

static DevUIID id_from_cstr(const char *cstr) {
    DevUIID result;
    result.v = crc32_cstr(cstr);
    return result;
}

static DevUIID id_empty() {
    return {};
}

static void element_size(DevUILayout *layout, Vec2 size) {
    layout->p.x += size.x + DEV_UI_HORIZ_PADDING;
    layout->last_line_p = layout->p;
    
    layout->p.x = layout->horizontal_offset;
    layout->p.y += size.y + DEV_UI_VERT_PADDING;
}

void dev_ui_last_line(DevUILayout *layout) {
    layout->p = layout->last_line_p;
}

static bool is_same(DevUIID a, DevUIID b) {
    return a.v == b.v;
}

static bool is_set(DevUIID id) {
    return id.v != 0;
}

struct ButtonState {
    bool is_pressed;
    bool is_hot;
    bool is_held;  
};

static ButtonState update_button(DevUILayout *layout, Rect rect, DevUIID id, bool repeat_when_held = false) {
    if (rect.collide(layout->input->platform->mpos)) {
        layout->is_focused = true;
    }
    
    bool is_hot = !is_set(layout->hot_id) && rect.collide(layout->input->platform->mpos);
    if (is_hot) {
        layout->hot_id = id;
        if (is_key_pressed(layout->input, KEY_MOUSE_LEFT) && !is_set(layout->dev_ui->active_id)) {
            layout->dev_ui->active_id = id;
        }
    }
    
    bool is_pressed = false;
    bool is_held = false;
    if (is_same(layout->dev_ui->active_id, id)) {
        if (is_key_held(layout->input, KEY_MOUSE_LEFT)) {
            is_held = true;
            if (repeat_when_held && is_hot) {
                is_pressed = true;
            }
        } else {
            if (is_hot) {
                is_pressed = true;
            }
            layout->dev_ui->active_id = id_empty();
        }
    }
    
    ButtonState result;
    result.is_hot = is_hot;
    result.is_pressed = is_pressed;
    result.is_held = is_held;
    return result;
}

DevUILayout dev_ui_begin(DevUI *dev_ui, InputManager *input, Assets *assets, RendererCommands *commands) {
    DevUILayout layout = {};
    layout.dev_ui = dev_ui;
    layout.input = input;
    layout.assets = assets;
    layout.font_id = assets_get_first_of_type(assets, ASSET_TYPE_FONT);
    RenderGroup interface_render_group = render_group_begin(commands, assets,
        setup_2d(RENDERER_FRAMEBUFFER_GAME_INTERFACE, Mat4x4::ortographic_2d(0, input->platform->display_size.x, input->platform->display_size.y, 0)));
    layout.render_group = interface_render_group;
    return layout;
}

void dev_ui_labelv(DevUILayout *layout, const char *format, va_list args) {
    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), format, args);
    DEBUG_push_text(&layout->render_group, layout->p, WHITE, buffer, layout->font_id, 1.0f);
    AssetInfo *font = assets_get_info(layout->assets, layout->font_id);
    element_size(layout, DEBUG_get_text_size(layout->assets, layout->font_id, buffer));
}

void dev_ui_labelf(DevUILayout *layout, const char *format, ...) {
    va_list args;
    va_start(args, format);
    dev_ui_labelv(layout, format, args);
    va_end(args);
}

bool dev_ui_button(DevUILayout *layout, const char *label) {
    Vec2 text_size = DEBUG_get_text_size(layout->assets, layout->font_id, label);
    Rect button_rect = Rect(layout->p, text_size);
    ButtonState bstate = update_button(layout, button_rect, id_from_cstr(label));
    Vec4 color = (bstate.is_held ? Vec4(1, 1, 0, 1) : bstate.is_hot ? Vec4(0.6, 0.6, 0, 1) :  Vec4(1, 1, 1, 1));    
    DEBUG_push_text(&layout->render_group, layout->p, color, label, layout->font_id, 1.0f);  
    element_size(layout, button_rect.size());
    return bstate.is_pressed;    
}

bool dev_ui_checkbox(DevUILayout *layout, const char *label, bool *value) {
    Vec2 text_size = DEBUG_get_text_size(layout->assets, layout->font_id, label);
    Rect button_rect = Rect(layout->p, text_size);
    ButtonState bstate = update_button(layout, button_rect, id_from_cstr(label));
    if (bstate.is_pressed) {
        *value = !*value;
    }
    Vec4 color = (*value ? Vec4(1, 1, 0, 1) : bstate.is_hot ? Vec4(0.6, 0.6, 0, 1) :  Vec4(1, 1, 1, 1));    
    DEBUG_push_text(&layout->render_group, layout->p, color, label, layout->font_id, 1.0f);
    element_size(layout, button_rect.size());
    return bstate.is_pressed;    
}

static DevUIView *get_dev_ui_view(DevUI *ui, DevUIID id) {
    assert(is_set(id));
    u32 hash_slot_init = id.v % ARRAY_SIZE(ui->view_hash);
    u32 hash_slot = hash_slot_init;
    DevUIView *view = ui->view_hash + hash_slot;
    for (;;) {
        if (is_same(view->id, id)) {
            break;
        }
        if (view->next_in_hash) {
            view = view->next_in_hash;
        } else {
            DevUIView *new_view = ui->view_hash + (++hash_slot % ARRAY_SIZE(ui->view_hash));
            if (hash_slot_init == hash_slot) {
                assert(!"Out of space");
                view = 0;
                break;
            }
            if (is_same(new_view->id, id_empty())) {
                view->next_in_hash = new_view;
                view = new_view;
                view->id = id;
                break;
            }
        }
    }
    return view;
}

bool dev_ui_drag(DevUILayout *layout, const char *label, f32 *value) {
    bool result = false;
    char value_buffer[128];
    snprintf(value_buffer, sizeof(value_buffer), "%s: %.2f", label, *value);
    Vec2 value_rect_size = DEBUG_get_text_size(layout->assets, layout->font_id, value_buffer);
    Rect whole_rect = Rect(layout->p, value_rect_size);
    
    ButtonState bstate = update_button(layout, whole_rect, id_from_cstr(label));
    if (bstate.is_held) {
#define SPEED 0.1f
        f32 new_value = *value + layout->input->platform->mdelta.x * SPEED;
        if (*value != new_value) {
            *value = new_value;
            result = true;
        }
    }
    Vec4 color = (bstate.is_held ? Vec4(1, 1, 0, 1) : bstate.is_hot ? Vec4(0.6, 0.6, 0, 1) :  Vec4(1, 1, 1, 1));    
    DEBUG_push_text(&layout->render_group, layout->p, color, value_buffer, layout->font_id, 1.0f);
    element_size(layout, whole_rect.size());
    return result;
}

bool dev_ui_section(DevUILayout *layout, const char *label) {
    Vec2 text_size = DEBUG_get_text_size(layout->assets, layout->font_id, label);
    Rect button_rect = Rect(layout->p, text_size);
    DevUIID id = id_from_cstr(label);
    ButtonState bstate = update_button(layout, button_rect, id);
    DevUIView *view = get_dev_ui_view(layout->dev_ui, id);
    if (bstate.is_pressed) {
        view->is_opened = !view->is_opened;
    }
    Vec4 color = (view->is_opened ? Vec4(1, 1, 0, 1) : Vec4(1, 1, 1, 1));    
    DEBUG_push_text(&layout->render_group, layout->p, color, label, layout->font_id, 1.0f);
    element_size(layout, button_rect.size());
    if (view->is_opened) {
        layout->p.x += DEV_UI_SECTION_OFFSET;
        layout->horizontal_offset += DEV_UI_SECTION_OFFSET;
    }
    return view->is_opened;
}

void dev_ui_end_section(DevUILayout *layout) {
    layout->p.x -= DEV_UI_SECTION_OFFSET;
    layout->horizontal_offset -= DEV_UI_SECTION_OFFSET;
}

void dev_ui_end(DevUILayout *layout) {
}

void dev_ui_begin_sizable(DevUILayout *layout) {
    
}
void dev_ui_end_sizable(DevUILayout *layout) {
    
}