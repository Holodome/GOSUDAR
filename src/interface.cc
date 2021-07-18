#include "interface.hh"

UIElement *new_ui_element(MemoryArena *arena, UIElement **ll_elements, UIElementKind kind) {
    UIElement *element = alloc_struct(arena, UIElement);
    element->kind = kind;
    if (*ll_elements) {
        LLIST_ADD(*ll_elements, element);
    } else {
        *ll_elements = element;
    }
    return element;    
}

UIElement *create_ui_block(MemoryArena *arena, UIElement **ll_elements, Rect rect, vec4 color) {
    UIElement *element = new_ui_element(arena, ll_elements, UI_ELEMENT_BLOCK);
    element->rect = rect;
    element->block.color = color;    
    return element;
}

UIElement *create_ui_label(MemoryArena *arena, UIElement **ll_elements, Rect rect, vec4 color, const char *text) {
    UIElement *element = new_ui_element(arena, ll_elements, UI_ELEMENT_LABEL);
    element->rect = rect;
    element->label.color = color;
    element->label.text = text;
    return element;
}

UIElement *create_ui_button(MemoryArena *arena, UIElement **ll_elements, Rect rect,
    vec4 color_inactive, vec4 color_active, const char *text) {
    UIElement *element = new_ui_element(arena, ll_elements, UI_ELEMENT_BUTTON);
    element->rect = rect;
    element->button.color_inactive = color_inactive;
    element->button.color_active = color_active;
    element->button.text = text;        
    return element;
}

UIElement *create_ui_button_background(MemoryArena *arena, UIElement **ll_elements, Rect rect,
     vec4 color_inactive, vec4 color_active, const char *text, vec4 background) {
    UIElement *result = create_ui_button(arena, ll_elements, rect, color_inactive, color_active, text);
    create_ui_block(arena, ll_elements, rect, background);
    return result;    
}

UIElement *create_ui_checkbox(MemoryArena *arena, UIElement **ll_elements, Rect rect,
    vec4 color_inactive, vec4 color_active, const char *text, bool *value) {
    UIElement *element = new_ui_element(arena, ll_elements, UI_ELEMENT_CHECKBOX);
    element->rect = rect;
    element->checkbox.color_inactive = color_inactive;
    element->checkbox.color_active = color_active;
    element->checkbox.value = value;       
    element->checkbox.text = text;
    return element;
}

UIElement *create_ui_checkbox_background(MemoryArena *arena, UIElement **ll_elements, Rect rect,
    vec4 color_inactive, vec4 color_active, const char *text, bool *value, vec4 background) {
    UIElement *result = create_ui_checkbox(arena, ll_elements, rect, color_inactive, color_active, text, value);
    create_ui_block(arena, ll_elements, rect, background);
    return result;
}

UIListener *get_listener(UIElement *element) {
    return &element->listener;
}

void update_and_render_interface(UIElement *first_element, InputManager *input, RendererCommands *commands, Assets *assets) {
    RenderGroup ui_render_group = render_group_begin(commands, assets, setup_2d(RENDERER_FRAMEBUFFER_GAME_INTERFACE, 
        Mat4x4::ortographic_2d(0, input->platform->display_size.x, input->platform->display_size.y, 0)));
    UIElement *element_depth_stack[UI_MAX_DEPTH] = {};
    u32 depth_stack_index = 0;
    element_depth_stack[0] = first_element;
    for (;;) {
        while (depth_stack_index && !element_depth_stack[depth_stack_index]) {
            --depth_stack_index;
        } 
        if (!depth_stack_index && !element_depth_stack[depth_stack_index]) {
            break;
        }
        
        UIElement *element = element_depth_stack[depth_stack_index];
        element_depth_stack[depth_stack_index] = element->next;
        memset(&element->listener, 0, sizeof(element->listener));
        
        switch (element->kind) {
            case UI_ELEMENT_CONTAINER: {
                assert(depth_stack_index + 1 < UI_MAX_DEPTH);
                element_depth_stack[++depth_stack_index] = element->container.first_child;
            } break;
            case UI_ELEMENT_BLOCK: {
                push_rect(&ui_render_group, element->rect, element->block.color);
            } break;
            case UI_ELEMENT_BUTTON: {
                bool collides = element->rect.collide(input->platform->mpos);
                if (collides && is_key_pressed(input, KEY_MOUSE_LEFT)) {
                    element->button.is_held = true;
                } else {
                    element->button.is_pressed = false;
                    if (element->button.is_held && is_key_held(input, KEY_MOUSE_LEFT)) {
                        element->button.is_held = true;
                    } else if (element->button.is_held) {
                        element->listener.is_pressed = true;
                        element->button.is_pressed = true;
                        element->button.is_held = false;
                    } 
                }
                
                AssetID font_id = assets_get_first_of_type(assets, ASSET_TYPE_FONT);
                vec2 text_size = DEBUG_get_text_size(assets, font_id, 
                    element->button.text);
                vec2 position = element->rect.p + (element->rect.s - text_size) * 0.5f;
                vec4 color = element->button.is_held ? element->button.color_active : element->button.color_inactive;
                DEBUG_push_text(&ui_render_group, position, color, element->button.text, font_id, 1.0f);
            } break;
            case UI_ELEMENT_CHECKBOX: {
                bool collides = element->rect.collide(input->platform->mpos);
                if (collides && is_key_pressed(input, KEY_MOUSE_LEFT)) {
                    element->checkbox.is_held = true;
                } else {
                    if (element->checkbox.is_held && is_key_held(input, KEY_MOUSE_LEFT)) {
                        element->checkbox.is_held = true;
                    } else if (element->checkbox.is_held) {
                        *element->checkbox.value = !*element->checkbox.value;
                        element->checkbox.is_held = false;
                    } 
                }
                
                AssetID font_id = assets_get_first_of_type(assets, ASSET_TYPE_FONT);
                vec2 text_size = DEBUG_get_text_size(assets, font_id, 
                    element->button.text);
                vec2 position = element->rect.p + (element->rect.s - text_size) * 0.5f;
                vec4 color = *element->checkbox.value ? element->checkbox.color_active : element->checkbox.color_inactive;
                DEBUG_push_text(&ui_render_group, position, color, element->checkbox.text, font_id, 1.0f);
            } break;
            case UI_ELEMENT_LABEL: {
                AssetID font_id = assets_get_first_of_type(assets, ASSET_TYPE_FONT);
                vec2 text_size = DEBUG_get_text_size(assets, font_id, 
                    element->label.text);
                vec2 position = element->rect.p + (element->rect.s - text_size) * 0.5f;
                DEBUG_push_text(&ui_render_group, position, element->label.color, element->label.text, font_id, 1.0f);
            } break;
        }
      
        if (draw_ui_frames) {  
            push_rect_outline(&ui_render_group, element->rect, RED, 0.5f);
        }
    
    }
    render_group_end(&ui_render_group);
}
