#include "game/interface.hh"

#include "game/game_state.hh"

static InterfaceElement *add_element(MemoryArena *arena, Interface *interface, u32 kind) {
    InterfaceElement *element = alloc_struct(arena, InterfaceElement);
    ++interface->DEBUG_elements_count;
    if (!interface->first_element) {
        interface->first_element = element;
    }
    if (interface->last_element) {
        interface->last_element->next = element;    
    }
    interface->last_element = element;
    
    element->kind = kind;
    
    return element;
}

static InterfaceElement *add_rect(MemoryArena *arena, Interface *interface, Rect rect, Vec4 color) {
    InterfaceElement *element = add_element(arena, interface, INTERFACE_ELEMENT_RECTANGLE);
    element->rect = rect;
    element->color = color;
    return element;
}

static InterfaceElement *add_button(MemoryArena *arena, Interface *interface, Vec4 color_active, Vec4 color_inactive, 
    Rect rect, bool is_state_saving, const char *text) {
    InterfaceElement *button = add_element(arena, interface, INTERFACE_ELEMENT_BUTTON);
    button->color_button_active = color_active;
    button->color_button_inactive = color_inactive;
    button->rect = rect;
    button->is_button_state_saving = is_state_saving;
    button->text = text;
    return button;
}

static InterfaceElement *add_label(MemoryArena *arena, Interface *interface, Vec2 p, Vec4 color, const char *text = 0) {
    InterfaceElement *label = add_element(arena, interface, INTERFACE_ELEMENT_TEXT);
    label->rect.p = p;
    label->text = text;
    label->color = color;
    return label;
}

void init_interface_for_game_state(MemoryArena *arena, GameStateInterface *interface, Vec2 winsize) {
    f32 hotbar_relative_width = 1.0f;
    f32 hotbat_height = 120.0f;
    // Down menu rect
    Rect down_menu_rect = Rect(winsize.x * (1.0f - hotbar_relative_width) * 0.5f, winsize.y - hotbat_height, 
        winsize.x * hotbar_relative_width, hotbat_height);
    add_rect(arena, &interface->interface, down_menu_rect,  Vec4(0.5, 0.5, 0.5, 0.8f));
    f32 info_padding = 10.0f;
    f32 info_width = 100.0f;
    Rect info_menu_rect = Rect(down_menu_rect.x + info_padding, down_menu_rect.y + info_padding, info_width, hotbat_height - info_padding * 2.0f);
    add_rect(arena, &interface->interface, info_menu_rect, Vec4(0.3, 0.3, 0.3, 0.9));
    Vec2 wood_text_p = info_menu_rect.top_left() + Vec2(info_padding);
    interface->text_for_wood_count = add_label(arena, &interface->interface, wood_text_p, Vec4(0.9f, 0.9f, 0.9f, 1.0f));
    // @TODO get font height!!
    Vec2 gold_text_p = wood_text_p + Vec2(0, 40);
    interface->text_for_gold_count = add_label(arena, &interface->interface, gold_text_p, Vec4(0.9f, 0.9f, 0.9f, 1.0f));
    
    Rect camera_controls_button_rect = Rect(info_menu_rect.top_right() + Vec2(info_padding, 0), Vec2(50, 25));
    interface->button_camera_controls = add_button(arena, &interface->interface, Vec4(1, 0, 0, 1), Vec4(0.4, 0, 0, 1), 
        camera_controls_button_rect, true, "Cam");
    Rect build_button_rect = Rect(camera_controls_button_rect.bottom_left() + Vec2(0, info_padding), Vec2(50, 25));
    interface->button_build_mode = add_button(arena, &interface->interface, Vec4(1, 0, 0, 1), Vec4(0.4, 0, 0, 1), 
        build_button_rect, true, "Build");
}

InterfaceStats interface_update(Interface *interface, InputManager *input) {
    TIMED_FUNCTION();
    InterfaceStats stats = {};
    for (InterfaceElement *element = interface->first_element;
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
                if (!element->is_button_state_saving && !is_key_held(input, Key::MouseLeft, INPUT_ACCESS_TOKEN_GAME_INTERFACE)) {
                    element->is_button_pressed = false;
                }
                
                if (element->rect.collide(mouse_p(input)) && is_key_pressed(input, Key::MouseLeft, INPUT_ACCESS_TOKEN_GAME_INTERFACE)) {
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

void interface_render(Interface *interface, RenderGroup *render_group) {
    for (InterfaceElement *element = interface->first_element;
         element;
         element = element->next) {
        switch (element->kind) {
            case INTERFACE_ELEMENT_RECTANGLE: {
                push_rect(render_group, element->rect, element->color);
            } break;
            case INTERFACE_ELEMENT_TEXT: {
                push_text(render_group, element->rect.p, Vec4(1, 1, 1, 1), element->text, Asset_Font, 1.0f);
            } break;
            case INTERFACE_ELEMENT_BUTTON: {
                Vec4 color = element->is_button_pressed ? element->color_button_active : element->color_button_inactive;
                push_rect(render_group, element->rect, color);
                Vec2 text_size = render_group->assets->get_text_size(Asset_Font, element->text);
                Vec2 text_p = element->rect.top_left() + (element->rect.size() - text_size) * 0.5f;
                push_text(render_group, text_p, Vec4(1), element->text, Asset_Font, 1.0f);
            } break;
            case INTERFACE_ELEMENT_IMAGE: {
            
            } break;
        }
    }
}
