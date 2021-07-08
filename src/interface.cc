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

UIElement *create_ui_block(MemoryArena *arena, UIElement **ll_elements, Rect rect, Vec4 color) {
    UIElement *element = new_ui_element(arena, ll_elements, UI_ELEMENT_BLOCK);
    element->rect = rect;
    element->block.color = color;    
    return element;
}

UIElement *create_ui_button(MemoryArena *arena, UIElement **ll_elements, Rect rect,
     Vec4 color_inactive, Vec4 color_active, const char *text) {
    UIElement *element = new_ui_element(arena, ll_elements, UI_ELEMENT_BUTTON);
    element->rect = rect;
    element->button.color_inactive = color_inactive;
    element->button.color_active = color_active;
    element->button.text = text;        
    return element;
}

void update_interface(UIElement *first_element, InputManager *input, RendererCommands *commands, Assets *assets) {
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
        
#if 1 
        push_rect_outline(&ui_render_group, element->rect, RED, 0.5f);
#endif 
        
        switch (element->kind) {
            case UI_ELEMENT_CONTAINER: {
                assert(depth_stack_index + 1 < UI_MAX_DEPTH);
                element_depth_stack[++depth_stack_index] = element->container.first_child;
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
                        element->button.is_pressed = true;
                        element->button.is_held = false;
                    } 
                }
                
                AssetID font_id = assets_get_first_of_type(assets, ASSET_TYPE_FONT);
                Vec2 text_size = DEBUG_get_text_size(assets, font_id, 
                    element->button.text);
                Vec2 position = element->rect.p + (element->rect.s - text_size) * 0.5f;
                Vec4 color = element->button.is_held ? element->button.color_active : element->button.color_inactive;
                DEBUG_push_text(&ui_render_group, position, color, element->button.text, font_id, 1.0f);
            } break;
        }
    }
    render_group_end(&ui_render_group);
}
