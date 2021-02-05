#if !defined(DEVUI_HH)

#include "lib/lib.hh"

#include "renderer/renderer.hh"

const f32 DEVUI_MEMORY_ARENA_SIZE =         MEGABYTES(16);
const u32 DEVUI_MAX_TITLE_SIZE =            32;
const f32 DEVUI_KEY_REPEAT_DELAY =          0.25;
const f32 DEVUI_KEY_REPEAT_RATE =           0.20;
const f32 DEVUI_MOUSE_DOUBLE_CLICK_TIME =   0.30;
const f32 DEVUI_WINDOW_TITLE_BAR_HEIGHT =   20.0;
const f32 DEVUI_TEXT_SCALE =                0.5f;
const Vec2 DEVUI_MIN_WINDOW_SIZE =          Vec2(100.0, 40.0);
const Vec2 DEVUI_ITEM_SPACING =             Vec2(10.0 ,  5.0);
const Vec2 DEVUI_FRAME_PADDING =            Vec2(5.0  ,  4.0);
const Vec2 DEVUI_WINDOW_PADDING =           Vec2(8.0  ,  8.0);
const Vec2 DEVUI_RESIZE_SIZE =              Vec2(16.0f, 16.0f);
const Vec4 DEVUI_COLOR_TEXT =               Vec4(0.860, 0.930, 0.890, 0.780);
const Vec4 DEVUI_COLOR_WINDOW_BACKGROUND =  Vec4(0.130, 0.140, 0.170, 1.000);
const Vec4 DEVUI_COLOR_WINDOW_TITLEBAR =    Vec4(0.230, 0.200, 0.270, 1.000);
const Vec4 DEVUI_COLOR_WIDGET_BACKGROUND =  Vec4(0.200, 0.220, 0.270, 1.000);
const Vec4 DEVUI_COLOR_CHECKBOX_ACTIVE =    Vec4(0.710, 0.220, 0.270, 1.000);
const Vec4 DEVUI_COLOR_BUTTON =             Vec4(0.470, 0.770, 0.830, 0.140);
const Vec4 DEVUI_COLOR_BUTTON_HOT =         Vec4(0.460, 0.200, 0.300, 0.860);
const Vec4 DEVUI_COLOR_BUTTON_ACTIVE =      Vec4(0.460, 0.200, 0.300, 1.000);
const Vec4 DEVUI_COLOR_HEADER =             Vec4(0.460, 0.200, 0.300, 0.760);
const Vec4 DEVUI_COLOR_HEADER_HOT =         Vec4(0.500, 0.080, 0.260, 1.000);
const Vec4 DEVUI_COLOR_HEADER_ACTIVE =      Vec4(0.460, 0.200, 0.300, 0.860);

struct DevUIDrawQueueEntry {
    Vec3 v[4];
    Vec4 c[4];  
    Vec2 uvs[4];
    RendererTexture tex;
};

struct DevUIID {
    u32 p, s;
    
    operator bool() {
        return p && s;
    }
    
    bool operator==(DevUIID other) {
        return p == other.p && s == other.s;
    }
};

struct DevUIWindow {
    char title[DEVUI_MAX_TITLE_SIZE];
    DevUIID id;
    
    Rect whole_rect, rect, title_bar_rect;
    Vec2 cursor, last_line_cursor;
    f32 line_height, last_line_height;
};

struct DevUIButtonState {
    bool is_pressed;
    bool is_held;
    bool is_hot;  
};

struct DevUI {
    Array<DevUIDrawQueueEntry> draw_queue;
    Array<DevUIWindow> windows;
    DevUIWindow *cur_win, *hot_win;
    DevUIID hot_id, active_id;
    Font *font;
    
    DevUI();
    ~DevUI() {}
    void begin_frame();
    void end_frame();
    
    void window(char *title, Rect rect);
    void window_end();
    void text(char *text);
    void textv(char *text, va_list args);
    void textf(char *text, ...);
    bool button(char *label, bool repeat_when_held = false);
    
    // Utility functions
    // @NOTE these are directly copied from renderer
    void draw_quad(Vec3 v0, Vec3 v1, Vec3 v2, Vec3 v3, 
              Vec4 c0, Vec4 c1, Vec4 c2, Vec4 c3,
              Vec2 st0 = Vec2(0, 0), Vec2 st1 = Vec2(0, 1), Vec2 st2 = Vec2(1, 0), Vec2 st3 = Vec2(1, 1),
              RendererTexture tex = EMPTY_TEXTURE);
    void draw_rect(Rect rect, Vec4 color = Vec4(1), RendererTexture tex = EMPTY_TEXTURE);    
    void draw_text(Vec2 p, Vec4 c, char *text, Font *font, f32 scale = 1.0f);
    static DevUIID make_id(DevUIWindow *win, char *text, size_t count = 0);
    void element_size(Vec2 size, Vec2 *adjust_start_offset = 0);
    DevUIButtonState update_button(Rect rect, DevUIID id, bool repeat_when_held = false);
};

#define DEVUI_HH 1
#endif
