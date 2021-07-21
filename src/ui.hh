//
// UI system ui game is a very complex system due to a lot of parameters it must act according to,
// ability to scale and react to changes well on different DPIs and window sizes, 
// and, most importantly, ease of use
//
// Starting from the end, ease of use has always been a problem for ui systems.
// Naive retained-mode uis, like Java swing, QT require a lot of boilerplate code to set up 
// ui, which is not suitable for flexible system required-in game
// These retained-mode systems also share the problem of integrating logic in larger systems.
// Usage of this UIs is based around them - they are the _main_ component in the app.
// Not only does this introduce problems with multithreading, but also breaks the logic 
// of the game, that should be standing above the ui system, and use it as a tool, but not 
// the other way. 
// Thus, things like callback functions common for retained mode are negative in our case.
// System described above is a common example of OOP.
//
// Having callbacks can be avoided with having some kind of listeners for elements. This is when
// each element, when updated notifies list of listeners, that, at their time use information of update
// to do logic. 
// This approach is better than callback stuff, but is still is not ideal. Though it behaves really
// well with buttons, problems start with checkboxes. 
// They often correspond to some other value, that can be accessed with pointer. So each checkbox,
// in addition to listener, has implicit listener via pointer. This breaks the logic of ui,
// when listeners are not the only thing standing between the game and ui, but ui has direct access to
// game.
// If one decides to avoid having this inconsistency problem with not using pointers, he may face 
// another issue. This is having to update in-game value each time checkbox is updates - 
// mimicing the behaviour of pointer. This may face problmes when this in-game value can be changed
// in other ways. For example, when game value is changed, ui system knows nothing about that
// and stores its own state of value. So having state within ui is not a solution for problem 
// of game interacting with ui.
// System described above can be implemented in data-oriented way.
//
// There are s substen of UI called immediate mode. Immediate mode uis, in contrast to retained-mode,
// don't have stage of creation involving bolirplate code. This makes them more flexible, but also 
// harder to controll, which is needed for games. Also, immediate mode uis generally use some kind
// of id to access state of widgets during frame. This id can be extracted from widget label (while
// forcing use of labels), pointer value or anything else. Use of labels involves strings that 
// are better be avoided. Not having strings for ids can be achieved with having constant enumeration
// for all game widgets, which is inconvenient although perormant and cache-friendly. Inconvenience
// can be miminimes with storing ids somewhere besides ui, which makes this system retained-mode
//
// So here comes an idea of hybrid ui system that has pluses of retained mode, like great ability 
// to controll behaviours of widgets, while also having scalability and straighforward logic of 
// immediate mode-systems. 
// Core idea is close to data-oriented retained mode. At creation stage we assign element descriptions
// to ids, and store this ids somewhere.
// At updation stage, logic is strictly from immedate mode. Positions of all widgets are set
// dynamically according to their descriptions. (It also is possible to set descriptions here
// and avoid having creation stage at all - but there is an assumption that ui descriptions can 
// become massive due to different resolutions and settings)
// So game at each times describes in its own way in what order widgets are placed. This avoids 
// having widget hierarchies that are hard to reuse in different places (what if we want to add 
// some widgets dynamiclly to other - we have to modify the storage of them, which involves 
// adding and deleting from lists)
//
// 
// Game uis should be suitable for different window sizes, dpis, aspect ratios. This can be achieved 
// in naive way, with having precomputed positions for all widgets. Although this is robust,
// it is not suitable for iterative development. 
// Needed functionality can be achieved with having 'constraint' system. Instead of having postions
// and size of widgets stored somewhere, we store only rules on which they are later computed.
// And although this is not a solution for whole problem (due to different DPIS and aspect raios),
// having constrains simplifies protyping and when it actullay comes to tweaking, there can just 
// be several different possible constraints per aspect ratio, window size, or dpi.
// But it very possibly seems what even won't be the case (take a look at Factorio)
//
#if !defined(UI_HH)

#include "lib.hh"

#define UI_HASH_SIZE 1024
#define UI_MAX_DEPTH 16

struct UIID {
    u32 value;
};

enum {
    CONSTRAINT_TYPE_X,
    CONSTRAINT_TYPE_Y,
    CONSTRAINT_TYPE_W,
    CONSTRAINT_TYPE_H,
};

enum {
    CONSTRAINT_KIND_ASPECT,
    CONSTRAINT_KIND_RELATIVE,
    CONSTRAINT_KIND_CENTER,
    CONSTRAINT_KIND_PX,
};

struct UIConstraint {
    u32 type;
    u32 kind;
    f32 value;
};

struct UIConstraintSlot {
    UIConstraint constraint;
    UIConstraintSlot *next;
};

enum {
    UI_POLICY_TYPE_TEXT,
};

struct UIPolicy {
    
};

struct UIPolicySlot {
    UIPolicy policy;
    UIPolicySlot *next;
};

struct UIElement {
    
    UIPolicySlot *policy_list;
    UIConstraintSlot *constraint_list;
};

struct UIHash {
    UIID id;
    UIElement *ptr;
};

struct UI {
    MemoryArena *arena;
    u32 ui_cursor;
    
    UIHash hash[UI_HASH_SIZE];
    
    UIID depth_stack[UI_MAX_DEPTH];
    u32 depth_stack_idx;
    UIID current_described_id;
};

UI *ui_init();
UIID describe_ui_element(UI *ui);
void add_constraint(UI *ui, UIConstraint constraint);
void add_policy(UI *ui, UIPolicy policy);
void end_ui_element_description(UI *ui);

void push_ui_element(UI *ui, UIID id);

void pop_ui_element(UI *ui);

#define UI_HH 1
#endif 