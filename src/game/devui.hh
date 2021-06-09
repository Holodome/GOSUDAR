#if !defined(DEVUI_HH)

#include "lib/lib.hh"

#include "renderer/renderer.hh"

const f32 DEVUI_EPSILON = 0.001f;
const u32 DEVUI_MAX_TITLE_SIZE =            32;
const f32 DEVUI_KEY_REPEAT_DELAY =          0.25;
const f32 DEVUI_KEY_REPEAT_RATE =           0.20;
const f32 DEVUI_MOUSE_DOUBLE_CLICK_TIME =   0.30;
const f32 DEVUI_WINDOW_TITLE_BAR_HEIGHT =   20.0;
const f32 DEVUI_TEXT_SCALE =                0.5f;
const Vec2 DEVUI_CHECKMARK_OFFSET =         Vec2(4, 4);
const Vec2 DEVUI_COLLAPSE_RECT_SIZE =       Vec2(10, 10);
const Vec2 DEVUI_MIN_WINDOW_SIZE =          Vec2(100.0, 40.0);
const Vec2 DEVUI_ITEM_SPACING =             Vec2(10.0 ,  5.0);
const Vec2 DEVUI_FRAME_PADDING =            Vec2(5.0  ,  4.0);
const Vec2 DEVUI_WINDOW_PADDING =           Vec2(8.0  ,  8.0);
const Vec2 DEVUI_RESIZE_SIZE =              Vec2(8.0f, 8.0f);
const Vec4 DEVUI_COLOR_TEXT =               Vec4(0.860, 0.930, 0.890, 0.780);
const Vec4 DEVUI_COLOR_WINDOW_BACKGROUND =  Vec4(0.130, 0.140, 0.170, 1.000);
const Vec4 DEVUI_COLOR_WINDOW_TITLEBAR =    Vec4(0.230, 0.200, 0.270, 1.000);
const Vec4 DEVUI_COLOR_WIDGET_BACKGROUND =  Vec4(0.200, 0.220, 0.270, 1.000);
const Vec4 DEVUI_COLOR_BUTTON =             Vec4(0.470, 0.770, 0.830, 0.140);
const Vec4 DEVUI_COLOR_BUTTON_HOT =         Vec4(0.460, 0.200, 0.300, 0.660);
const Vec4 DEVUI_COLOR_BUTTON_ACTIVE =      Vec4(0.460, 0.200, 0.300, 1.000);
const Vec4 DEVUI_COLOR_HEADER =             Vec4(0.460, 0.200, 0.300, 0.760);
const Vec4 DEVUI_COLOR_HEADER_HOT =         Vec4(0.500, 0.080, 0.260, 1.000);
const Vec4 DEVUI_COLOR_HEADER_ACTIVE =      Vec4(0.460, 0.200, 0.300, 0.860);

// DevUI имеет свою очередь отрисовки, чтобы вызовы функций интерйефса не мешали последовтельности отрисовки 
// игры и изображения не накладывались друг на друга. 
// На данный момент все элементы, которые рисует DevUI, могут быть представлены в виде четырехугольников
struct DevUIDrawQueueEntry {
    Vertex v[4];
    Texture *tex;
};

struct DevUIID {
    u32 p, s;
    
    operator bool() {
        return p && s;
    }
    
    bool operator==(DevUIID other) {
        return p == other.p && s == other.s;
    }
    
    static DevUIID empty() {
        return {};
    }
};

struct DevUIWindow {
    char title[DEVUI_MAX_TITLE_SIZE] = {};
    DevUIID id = DevUIID::empty();
    size_t array_idx = -1;
    
    bool is_collapsed = false;
    Rect whole_rect = {}, rect = {}, title_bar_rect = {};
    Vec2 cursor = {}, last_line_cursor = {};
    f32 line_height = 0, last_line_height = 0;
    Array<DevUIDrawQueueEntry> draw_queue = {};
};

struct DevUIButtonState {
    bool is_pressed;
    bool is_held;
    bool is_hot;  
};

struct DevUI {
    // Стек прямоугольников, ограничивающих область отрисовки
    // Например, если часть текста выходит за окна, ограничивающие прямогуольники обрежут ненужный текст
    Rect clip_rect_stack[5] = {};
    u32 clip_rect_stack_index = 0;
    // @TODO: проверить на эффективность памяти
    // Array<DevUIDrawQueueEntry> draw_queue = {};
    Array<DevUIWindow> windows = {};
    // Очередь, в которой окна рисуются на экран. Последний id в списке - верхнее окно
    Array<u32> windows_order = {};
    DevUIWindow *cur_win = 0, *hot_win = 0;
    DevUIID hot_id = DevUIID::empty(), active_id = DevUIID::empty();
    Font *font = 0;
    
    bool is_enabled = false, is_focused = false;
    
    void begin_frame();
    void end_frame();
    // Widget functions
    void window(const char *title, Rect rect);
    void window_end();
    void text(const char *text);
    void textv(const char *text, va_list args);
    void textf(const char *text, ...);
    bool button(const char *label, bool repeat_when_held = false);
    bool checkbox(const char *label, bool *value);
    
    // Utility functions
    void push_clip_rect(const Rect &rect);
    void pop_clip_rect();
    static DevUIID make_id(DevUIWindow *win, const char *text, size_t count = 0);
    void element_size(Vec2 size, Vec2 *adjust_start_offset = 0);
    void same_line(f32 spacing_w = DEVUI_ITEM_SPACING.x);
    DevUIButtonState update_button(Rect rect, DevUIID id, bool repeat_when_held = false);
    void push_rect(Rect rect, Vec4 color, Texture *tex = 0, Rect uv_rect = Rect(0, 0, 1, 1));
    void push_text(Vec2 p, Vec4 color, const char *text,  f32 scale = DEVUI_TEXT_SCALE);
};

#define DEVUI_HH 1
#endif
