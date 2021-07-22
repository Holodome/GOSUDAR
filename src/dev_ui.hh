#if !defined(DEVUI_HH)

#include "lib.hh"

#include "renderer.hh"
#include "renderer_api.hh"

#define DEVUI_HASH_SIZE 128
CT_ASSERT(IS_POW2(DEVUI_HASH_SIZE));

struct DevUIID {
    u32 value;
};  

struct DevUIView {
    DevUIID id;
    bool is_opened;
    vec2 size;
    // @TODO remove this
    DevUIView *next_in_hash;
};

struct DevUI {
    DevUIID active_id;
    DevUIView view_hash[DEVUI_HASH_SIZE];
};

struct DevUIDrawQueueEntry {
    Vertex v[4];
    AssetID texture;
};  

struct DevUILayout {
    DevUI *dev_ui;
    struct InputManager *input;
    struct Assets *assets;
    AssetID font_id;
    
    RenderGroup render_group;
    
    DevUIID hot_id;
    
    bool is_focused;
    
    f32 horizontal_offset;
    vec2 p;
    vec2 last_line_p;
    
    vec2 current_section_end_p;
};  

DevUILayout dev_ui_begin(DevUI *dev_ui, struct InputManager *input, struct Assets *assets, RendererCommands *commands);
void dev_ui_labelv(DevUILayout *layout, const char *format, va_list args);
void dev_ui_labelf(DevUILayout *layout, const char *format, ...);
bool dev_ui_button(DevUILayout *layout, const char *label);
bool dev_ui_checkbox(DevUILayout *layout, const char *label, bool *value);
bool dev_ui_drag(DevUILayout *layout, const char *label, f32 *value);
bool dev_ui_section(DevUILayout *layout, const char *name);
void dev_ui_end_section(DevUILayout *layout);
void dev_ui_last_line(DevUILayout *layout);
void dev_ui_end(DevUILayout *layout);
void dev_ui_begin_sizable(DevUILayout *layout, const char *label);
void dev_ui_end_sizable(DevUILayout *layout);

#define DEVUI_HH 1
#endif
