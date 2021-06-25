#if !defined(DEVUI_HH)

#include "lib/lib.hh"

#include "framework/renderer.hh"
#include "framework/assets.hh"

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
    Assets *assets;
    
    Vec2 mouse_p;
    Vec2 mouse_d;
    bool is_mouse_pressed;
    
    DevUIID active_id;
    DevUIView view_hash[128];
    
    AssetInfo *font_info;
    FontData *font;
};

struct DevUIDrawQueueEntry {
    Vertex v[4];
    Texture texture;
};  

struct DevUILayout {
    TempMemory temp_mem;
    DevUI *dev_ui;
    
    size_t max_draw_queue_entry_count;
    size_t draw_queue_entry_count;
    DevUIDrawQueueEntry *draw_queue;
    
    DevUIID hot_id;
    DevUIID active_id;
    
    Vec2 p;
};  

void dev_ui_init(DevUI *dev_ui, Assets *assets);
DevUILayout dev_ui_begin(DevUI *dev_ui);
void dev_ui_labelv(DevUILayout *layout, const char *format, va_list args);
void dev_ui_labelf(DevUILayout *layout, const char *format, ...);
bool dev_ui_button(DevUILayout *layout);
bool dev_ui_section(DevUILayout *layout, const char *name);
void dev_ui_end_section(DevUILayout *layout);
void dev_ui_end(DevUILayout *layout, RenderGroup *render_group);

#define DEVUI_HH 1
#endif
