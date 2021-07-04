#if !defined(DEVUI_HH)

#include "lib.hh"

#include "renderer.hh"
#include "render_group.hh"

struct DevUIID {
    u32 v;
};  

struct DevUIView {
    DevUIID id;
    bool is_opened;
    DevUIView *next_in_hash;
};

struct DevUI {
    MemoryArena arena;
    
    DevUIID active_id;
    DevUIView view_hash[128];
};

struct DevUIDrawQueueEntry {
    Vertex v[4];
    AssetID texture;
};  

struct DevUILayout {
    TempMemory temp_mem;
    DevUI *dev_ui;
    struct InputManager *input;
    struct Assets *assets;
    AssetID font_id;
    
    size_t max_draw_queue_entry_count;
    size_t draw_queue_entry_count;
    DevUIDrawQueueEntry *draw_queue;
    
    DevUIID hot_id;
    DevUIID active_id;
    
    bool is_focused;
    
    f32 horizontal_offset;
    Vec2 p;
    Vec2 last_line_p;
};  

DevUILayout dev_ui_begin(DevUI *dev_ui, struct InputManager *input, struct Assets *assets);
void dev_ui_labelv(DevUILayout *layout, const char *format, va_list args);
void dev_ui_labelf(DevUILayout *layout, const char *format, ...);
bool dev_ui_button(DevUILayout *layout, const char *label);
bool dev_ui_checkbox(DevUILayout *layout, const char *label, bool *value);
bool dev_ui_section(DevUILayout *layout, const char *name);
void dev_ui_end_section(DevUILayout *layout);
void dev_ui_last_line(DevUILayout *layout);
void dev_ui_end(DevUILayout *layout, RenderGroup *render_group);
void dev_ui_begin_sizable(DevUILayout *layout);
void dev_ui_end_sizable(DevUILayout *layout);

#define DEVUI_HH 1
#endif