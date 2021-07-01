#if !defined(INTERFACE_HH)

#include "lib/lib.hh"

enum {
    INTERFACE_ELEMENT_NONE,   
    INTERFACE_ELEMENT_RECTANGLE,   
    INTERFACE_ELEMENT_TEXT,   
    INTERFACE_ELEMENT_BUTTON,   
    INTERFACE_ELEMENT_IMAGE,   
    INTERFACE_ELEMENT_SENTINEL,   
};

struct InterfaceElement {
    u8 kind;  
    Rect rect;
    const char *text;
    u32 value_id;
    
    InterfaceElement *next;
};

struct InterfaceStats {
    bool is_mouse_over_element;
    bool interaction_occured;
    u32 value_id;
};

struct Interface {
    u32 DEBUG_elements_count;
    InterfaceElement *first_element;
    InterfaceElement *last_element;
};

void init_interface_for_game_state(MemoryArena *arena, Interface *interface);
InterfaceStats interface_update(Interface *interface, InputManager *input);
void interface_render(Interface *interface, RenderGroup *render_group);


#define INTERFACE_HH 1
#endif
