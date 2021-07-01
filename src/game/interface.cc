#include "game/interface.hh"

#include "game/game_state.hh"

static InterfaceElement *add_element(MemoryArena *arena, Interface *interface, u32 kind) {
    InterfaceElement *element = alloc_struct(arena, InterfaceElement);
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

static void add_rect(MemoryArena *arena, Interface *interface, Rect rect, Vec4 color) {
    InterfaceElement *element = add_element(arena, interface, INTERFACE_ELEMENT_RECTANGLE);
    element->rect = rect;
}

static void add_button(MemoryArena *arena, Interface *interface, Rect rect, u32 value_id) {
    InterfaceElement *button = add_element(arena, interface, INTERFACE_ELEMENT_BUTTON);
    button->value_id = value_id;
}

void init_interface_for_game_state(MemoryArena *arena, Interface *interface, Vec2 winsize) {
    f32 hotbar_relative_width = 0.6f;
    f32 hotbat_height = 40.0f;
    add_rect(arena, interface, Rect(winsize.x * (1.0f - hotbar_relative_width) * 0.5f, winsize.y - hotbat_height, 
        winsize.x * hotbar_relative_width, hotbat_height), Vec4(0.5, 0.5, 0.5, 1.0f));
}

InterfaceStats interface_update(Interface *interface, InputManager *input) {
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
                if (element->rect.collide(mouse_p(input)) && is_key_pressed(input, Key::MouseLeft, INPUT_ACCESS_TOKEN_GAME_INTERFACE)) {
                    assert(!stats.interaction_occured);
                    stats.value_id = element->value_id;
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
                push_rect(render_group, element->rect, Vec4(0.4, 0.4, 0.4, 1.0));
            } break;
            case INTERFACE_ELEMENT_TEXT: {
                // push_text(render_group, element->rect.p, Vec4(1, 1, 1, 1), Asset_Font, 1.0f);
            } break;
            case INTERFACE_ELEMENT_BUTTON: {
                
            } break;
            case INTERFACE_ELEMENT_IMAGE: {
            
            } break;
        }
    }
}
