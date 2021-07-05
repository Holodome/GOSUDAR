#include "interface.hh"

#include "game_state.hh"

static InterfaceElement *add_element(MemoryArena *arena, Interface *inter, u32 kind) {
    InterfaceElement *element = alloc_struct(arena, InterfaceElement);
    ++inter->DEBUG_elements_count;
    if (!inter->first_element) {
        inter->first_element = element;
    }
    if (inter->last_element) {
        inter->last_element->next = element;    
    }
    inter->last_element = element;
    
    element->kind = kind;
    
    return element;
}

static InterfaceElement *add_rect(MemoryArena *arena, Interface *inter, Rect rect, Vec4 color) {
    InterfaceElement *element = add_element(arena, inter, INTERFACE_ELEMENT_RECTANGLE);
    element->rect = rect;
    element->color = color;
    return element;
}

static InterfaceElement *add_button(MemoryArena *arena, Interface *inter, Vec4 color_active, Vec4 color_inactive, 
    Rect rect, bool is_state_saving, const char *text) {
    InterfaceElement *button = add_element(arena, inter, INTERFACE_ELEMENT_BUTTON);
    button->color_button_active = color_active;
    button->color_button_inactive = color_inactive;
    button->rect = rect;
    button->is_button_state_saving = is_state_saving;
    button->text = text;
    return button;
}

static InterfaceElement *add_label(MemoryArena *arena, Interface *inter, Vec2 p, Vec4 color, const char *text = 0) {
    InterfaceElement *label = add_element(arena, inter, INTERFACE_ELEMENT_TEXT);
    label->rect.p = p;
    label->text = text;
    label->color = color;
    return label;
}

void init_interface_for_game_state(MemoryArena *arena, GameStateInterface *inter, Vec2 winsize) {
    f32 hotbar_relative_width = 1.0f;
    f32 hotbat_height = 120.0f;
    // Down menu rect
    Rect down_menu_rect = Rect(winsize.x * (1.0f - hotbar_relative_width) * 0.5f, winsize.y - hotbat_height, 
        winsize.x * hotbar_relative_width, hotbat_height);
    add_rect(arena, &inter->inter, down_menu_rect,  Vec4(0.5, 0.5, 0.5, 0.8f));
    f32 info_padding = 10.0f;
    f32 info_width = 100.0f;
    Rect info_menu_rect = Rect(down_menu_rect.x + info_padding, down_menu_rect.y + info_padding, info_width, hotbat_height - info_padding * 2.0f);
    add_rect(arena, &inter->inter, info_menu_rect, Vec4(0.3, 0.3, 0.3, 0.9));
    Vec2 wood_text_p = info_menu_rect.top_left() + Vec2(info_padding);
    inter->text_for_wood_count = add_label(arena, &inter->inter, wood_text_p, Vec4(0.9f, 0.9f, 0.9f, 1.0f));
    // @TODO get font height!!
    Vec2 gold_text_p = wood_text_p + Vec2(0, 40);
    inter->text_for_gold_count = add_label(arena, &inter->inter, gold_text_p, Vec4(0.9f, 0.9f, 0.9f, 1.0f));
    
    Rect camera_controls_button_rect = Rect(info_menu_rect.top_right() + Vec2(info_padding, 0), Vec2(50, 25));
    inter->button_camera_controls = add_button(arena, &inter->inter, Vec4(1, 0, 0, 1), Vec4(0.4, 0, 0, 1), 
        camera_controls_button_rect, true, "Cam");
    Rect build_button_rect = Rect(camera_controls_button_rect.bottom_left() + Vec2(0, info_padding), Vec2(50, 25));
    inter->button_build_mode = add_button(arena, &inter->inter, Vec4(1, 0, 0, 1), Vec4(0.4, 0, 0, 1), 
        build_button_rect, true, "Build");
    Rect select_building1_rect = Rect(camera_controls_button_rect.top_right() + Vec2(info_padding, 0), Vec2(100, 25));
    inter->button_selected_building1 = add_button(arena, &inter->inter, Vec4(1, 0, 0, 1), Vec4(0.4, 0, 0, 1),
        select_building1_rect, true, "Building 1");
    Rect select_building2_rect = Rect(select_building1_rect.top_right() + Vec2(info_padding, 0), Vec2(100, 25));
    inter->button_selected_building1 = add_button(arena, &inter->inter, Vec4(1, 0, 0, 1), Vec4(0.4, 0, 0, 1),
        select_building2_rect, true, "Building 2");
    Vec2 building_state_p = select_building2_rect.bottom_left() + Vec2(0, info_padding);
    inter->building_state = add_label(arena, &inter->inter, building_state_p, Vec4(1), "Not building");
}

InterfaceStats interface_update(Interface *inter, InputManager *input) {
    TIMED_FUNCTION();
    InterfaceStats stats = {};
    for (InterfaceElement *element = inter->first_element;
         element;
         element = element->next) {
        if (element->rect.collide(mouse_p(input))) {
            if (input->access_token != INPUT_ACCESS_TOKEN_GAME_INTERFACE) {
                lock_input(input, INPUT_ACCESS_TOKEN_GAME_INTERFACE);
            }
            stats.is_mouse_over_element = true;
        }
        switch (element->kind) {
            case INTERFACE_ELEMENT_BUTTON: {
                if (!element->is_button_state_saving && !is_key_held(input, KEY_MOUSE_LEFT, INPUT_ACCESS_TOKEN_GAME_INTERFACE)) {
                    element->is_button_pressed = false;
                }
                
                if (element->rect.collide(mouse_p(input)) && is_key_pressed(input, KEY_MOUSE_LEFT, INPUT_ACCESS_TOKEN_GAME_INTERFACE)) {
                    element->is_button_pressed = !element->is_button_pressed;
                    assert(!stats.interaction_occured);
                    stats.interaction_occured = true;
                } 
            } break;
        }
        
        if (stats.interaction_occured) {
            break;
        }
    }
    
    if (!stats.is_mouse_over_element && input->access_token == INPUT_ACCESS_TOKEN_GAME_INTERFACE) {
        unlock_input(input);
    }
    
    return stats;
}

void interface_render(Interface *inter, RenderGroup *render_group) {
    for (InterfaceElement *element = inter->first_element;
         element;
         element = element->next) {
        switch (element->kind) {
            case INTERFACE_ELEMENT_RECTANGLE: {
                push_rect(render_group, element->rect, element->color);
            } break;
            case INTERFACE_ELEMENT_TEXT: {
                // push_text(render_group, element->rect.p, Vec4(1, 1, 1, 1), element->text, Asset_Font, 1.0f);
            } break;
            case INTERFACE_ELEMENT_BUTTON: {
                // Vec4 color = element->is_button_pressed ? element->color_button_active : element->color_button_inactive;
                // push_rect(render_group, element->rect, color);
                // Vec2 text_size = render_group->assets->DEBUG_get_text_size(Asset_Font, element->text);
                // Vec2 text_p = element->rect.top_left() + (element->rect.size() - text_size) * 0.5f;
                // push_text(render_group, text_p, Vec4(1), element->text, Asset_Font, 1.0f);
            } break;
            case INTERFACE_ELEMENT_IMAGE: {
            
            } break;
        }
    }
}
