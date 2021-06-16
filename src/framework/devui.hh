#if !defined(DEVUI_HH)

#include "lib/lib.hh"

#include "framework/renderer.hh"
#include "framework/assets.hh"

const f32 DEVUI_EPSILON =                   0.001f;
const u32 DEVUI_MAX_TITLE_SIZE =            32;
const f32 DEVUI_KEY_REPEAT_DELAY =          0.25;
const f32 DEVUI_KEY_REPEAT_RATE =           0.20;
const f32 DEVUI_MOUSE_DOUBLE_CLICK_TIME =   0.30;
const f32 DEVUI_WINDOW_TITLE_BAR_HEIGHT =   20.0;
const f32 DEVUI_TEXT_SCALE =                0.5f;
const f32 DEVUI_TEXT_CURSOR_WIDTH =         2.0f;
const f32 DEVUI_SLIDER_GRAB_WIDTH =         10.0f;
const Vec2 DEVUI_CHECKMARK_OFFSET =         Vec2(4, 4);
const Vec2 DEVUI_COLLAPSE_RECT_SIZE =       Vec2(10, 10);
const Vec2 DEVUI_MIN_WINDOW_SIZE =          Vec2(100.0, 40.0);
const Vec2 DEVUI_ITEM_SPACING =             Vec2(10.0 ,  5.0);
const Vec2 DEVUI_FRAME_PADDING =            Vec2(5.0  ,  4.0);
const Vec2 DEVUI_WINDOW_PADDING =           Vec2(8.0  ,  8.0);
const Vec2 DEVUI_RESIZE_SIZE =              Vec2(8.0f, 8.0f);
const Vec4 DEVUI_COLOR_TEXT =               Vec4(0.860, 0.930, 0.890, 0.780);
const Vec4 DEVUI_COLOR_WINDOW_BACKGROUND =  Vec4(0.130, 0.140, 0.170, 0.800);
const Vec4 DEVUI_COLOR_WINDOW_TITLEBAR =    Vec4(0.230, 0.200, 0.270, 1.000);
const Vec4 DEVUI_COLOR_WIDGET_BACKGROUND =  Vec4(0.200, 0.220, 0.270, 1.000);
const Vec4 DEVUI_COLOR_BUTTON =             Vec4(0.470, 0.770, 0.830, 0.140);
const Vec4 DEVUI_COLOR_BUTTON_HOT =         Vec4(0.460, 0.200, 0.300, 0.660);
const Vec4 DEVUI_COLOR_BUTTON_ACTIVE =      Vec4(0.460, 0.200, 0.300, 1.000);
const Vec4 DEVUI_COLOR_HEADER =             Vec4(0.460, 0.200, 0.300, 0.760);
const Vec4 DEVUI_COLOR_HEADER_HOT =         Vec4(0.500, 0.080, 0.260, 1.000);
const Vec4 DEVUI_COLOR_HEADER_ACTIVE =      Vec4(0.460, 0.200, 0.300, 0.860);

const u32 DEVUI_INPUT_FLAG_DECIMAL = 0x1;

// DevUI имеет свою очередь отрисовки, чтобы вызовы функций интерйефса не мешали последовтельности отрисовки 
// игры и изображения не накладывались друг на друга. 
// На данный момент все элементы, которые рисует DevUI, могут быть представлены в виде четырехугольников
struct DevUIDrawQueueEntry {
    Vertex v[4];
    Texture *tex;
};

struct DevUIID {
    // p is parent id
    // s is child id
    // p should contain information about parent, while s contains information about child
    // When need to nest ids, new parent id is pushed on stack and is used to make children ids
    // This way complex widgets can have children with same names but no collisions
    // @TODO record all ids to detect collisions. Since it is dev tool, we can just assert on duplicate id and developer will have to change it
    u32 p, s;
    
    operator bool() {
        return p && s;
    }
    
    bool operator==(DevUIID other) {
        return p == other.p && s == other.s;
    }
    bool operator!=(DevUIID other) {
        return !(*this == other);
    }
    
    static DevUIID empty() {
        return {};
    }
};

struct DevUIWindow {
    Str title = {};
    DevUIID id = DevUIID::empty();
    size_t array_idx = -1;
    
    bool is_collapsed = false;
    Rect whole_rect = {}, rect = {}, title_bar_rect = {};
    Vec2 cursor = {}, last_line_cursor = {};
    f32 line_height = 0, last_line_height = 0;
    Array<DevUIDrawQueueEntry> draw_queue = {};
    // Used whem making complex widgets, like single widget edit for 3-component vector
    f32 item_width = 0, default_item_width = 0;
};

struct DevUIButtonState {
    bool is_pressed;
    bool is_held;
    bool is_hot;  
};

struct DevUITextEditState {
    char text[1024];
    char initial_text[1024];
    u32 max_length;
    u32 cursor;
    bool has_selection;
    u32 selection_start;
    u32 selection_end;
};  

struct DevUI {
    // Стек ID, который используется для создания вложеных систем
    DevUIID id_stack[5] = {};
    u32 id_stack_index = 0;
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
    FontData *font = 0;
    f32 text_height = 0;
    DevUITextEditState text_edit = {};
    
    bool is_enabled = false, is_focused = false;
    
    void init(); 
    void begin_frame();
    void end_frame();
    // Widget functions
    void window(const char *title);
    void window_end();
    void text(const char *text);
    void textv(const char *text, va_list args);
    void textf(const char *text, ...);
    bool button(const char *label, bool repeat_when_held = false);
    bool checkbox(const char *label, bool *value);
    bool input_text(const char *label, void *buffer, size_t buffer_size, u32 flags = 0);
    bool input_float(const char *label, f32 *value);
    bool slider_float(const char *label, f32 *value, f32 minv = 0.0f, f32 maxv = 1.0f);
    // speed is value/px change
    bool drag_float(const char *label, f32 *value, f32 speed = 0.2f);
    bool drag_float3(const char *label, f32 value[3], f32 speed = 0.2f);
    
    void value(const char *label, f32 value);
    void value(const char *label, Vec2 value);
    void value(const char *label, Vec3 value);
    
    // Utility functions
    void label(const char *label);
    Vec2 get_text_size(const char *text, size_t count = 0);
    void push_clip_rect(const Rect &rect);
    void pop_clip_rect();
    void push_id(const DevUIID &id);
    void pop_id();
    DevUIID make_id(const char *text, size_t count = 0);
    void element_size(Vec2 size, Vec2 *adjust_start_offset = 0);
    void same_line(f32 spacing_w = DEVUI_ITEM_SPACING.x);
    DevUIButtonState update_button(Rect rect, DevUIID id, bool repeat_when_held = false);
    void push_rect(Rect rect, Vec4 color, Texture *tex = 0, Rect uv_rect = Rect(0, 0, 1, 1));
    void push_text(Vec2 p, const char *text, Vec4 color = DEVUI_COLOR_TEXT, f32 scale = DEVUI_TEXT_SCALE);
    bool is_text_input_key_pressed(Key key);
    Rect get_new_window_rect();
};

extern DevUI *dev_ui;

#define DEVUI_HH 1
#endif
