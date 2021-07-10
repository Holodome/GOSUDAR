#if !defined(INTERFACE_HH)

#include "lib.hh"

#define UI_MAX_DEPTH 10

enum UIElementKind {
    UI_ELEMENT_NONE,  
    UI_ELEMENT_BLOCK,  
    UI_ELEMENT_CONTAINER,  
    UI_ELEMENT_BUTTON,  
    UI_ELEMENT_LABEL,  
    UI_ELEMENT_CHECKBOX,  
};

enum UIElementConstraintDesignation {
    UI_ELEMENT_CONSTRAINT_X,  
    UI_ELEMENT_CONSTRAINT_Y, 
    UI_ELEMENT_CONSTRAINT_W, 
    UI_ELEMENT_CONSTRAINT_H, 
};

enum UIElementConstrainKind {
    UI_ELEMENT_CONTRAINT_KIND_NONE,  
    UI_ELEMENT_CONTRAINT_KIND_CENTER,  
    UI_ELEMENT_CONTRAINT_KIND_PIXEL,  
    UI_ELEMENT_CONTRAINT_KIND_RELATIVE,  
    UI_ELEMENT_CONTRAINT_KIND_ASPECT,  
};

struct UIElementConstraint {
    UIElementConstrainKind kind;
    f32 value;
};

inline UIElementConstraint relative_constraint(f32 value) {
    return { UI_ELEMENT_CONTRAINT_KIND_RELATIVE, value };
}

inline UIElementConstraint pixel_constraint(f32 value) {
    return { UI_ELEMENT_CONTRAINT_KIND_PIXEL, value };
}

inline UIElementConstraint aspect_constraint(f32 value) {
    return { UI_ELEMENT_CONTRAINT_KIND_ASPECT, value };
}

inline UIElementConstraint center_constraint() {
    return { UI_ELEMENT_CONTRAINT_KIND_CENTER, 0 };
}

struct UIListener {
    bool is_pressed;  
};

struct UIElement {
    UIElementKind kind;
    // UIElementConstraint constraints[4];
    Rect rect;
    UIListener listener;
    union {
        struct {
            UIElement *first_child;
        } container;
        struct {
            Vec4 color;
        } block;
        struct {
            Vec4 color_inactive;
            Vec4 color_active;
            const char *text;
            
            bool is_held;
            bool is_pressed;
        } button;
        struct {
            Vec4 color;
            const char *text;
        } label;
        struct {
            Vec4 color_active;
            Vec4 color_inactive;
            const char *text;
            
            bool is_held;
            bool *value;
        } checkbox;
    };
    
    UIElement *next;
};

UIElement *new_ui_element(MemoryArena *arena, UIElement **ll_elements, UIElementKind kind);
UIElement *create_ui_block(MemoryArena *arena, UIElement **ll_elements, Rect rect, Vec4 color);
UIElement *create_ui_label(MemoryArena *arena, UIElement **ll_elements, Rect rect, Vec4 color, const char *text);
UIElement *create_ui_button(MemoryArena *arena, UIElement **ll_elements, Rect rect,
     Vec4 color_inactive, Vec4 color_active, const char *text);
UIElement *create_ui_button_background(MemoryArena *arena, UIElement **ll_elements, Rect rect,
     Vec4 color_inactive, Vec4 color_active, const char *text, Vec4 background);
UIElement *create_ui_checkbox(MemoryArena *arena, UIElement **ll_elements, Rect rect,
    Vec4 color_inactive, Vec4 color_active, const char *text, bool *value);
UIElement *create_ui_checkbox_background(MemoryArena *arena, UIElement **ll_elements, Rect rect,
    Vec4 color_inactive, Vec4 color_active, const char *text, bool *value, Vec4 background);
UIListener *get_listener(UIElement *element);
void interface_recalculate_rects(UIElement *first_element, InputManager *input);
void update_and_render_interface(UIElement *first_element, InputManager *input, RendererCommands *commands, Assets *assets);

static bool draw_ui_frames = false;

#define INTERFACE_HH 1
#endif
