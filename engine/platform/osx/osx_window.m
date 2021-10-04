#include "platform/osx/osx.h"

#include "memory.h"

#include <AppKit/AppKit.h>

@interface OSX_Window_Delegate : NSObject<NSWindowDelegate> {
    Window_State *window_state;
}
   
- (instancetype)init_with_window_state:(Window_State *)window_state;
@end

@interface OSX_Content_View : NSView<NSTextInputClient>{
    Window_State *window_state;
}
- (instancetype)init_with_window_state:(Window_State *)window_state;
@end

typedef struct OSX_Window_State_Internal {
    NSWindow *win;
    OSX_Window_Delegate *win_delegate;
    OSX_Content_View *win_view;
    CALayer *layer;
} OSX_Window_State_Internal;

@implementation OSX_Window_Delegate
- (instancetype)init_with_window_state:(Window_State *)window_state_init {
    self = [super init];
    if (self != nil) {
        window_state = window_state_init;
    }    
    return self;
}

-(BOOL)windowShouldClose:(id)sender {
    window_state->is_quit_requested = true;
    return NO;
}
@end


@implementation OSX_Content_View
- (instancetype)init_with_window_state:(Window_State *)window_state_init {
    self = [super init];
    if (self != nil) {
        window_state = window_state_init;
    }    
    return self;
}

- (void)mouseDown:(NSEvent *)event {
    update_key_state(window_state, KEY_MOUSE_LEFT, true);
}

- (void)mouseUp:(NSEvent *)event {
    update_key_state(window_state, KEY_MOUSE_LEFT, false);
}

- (void)rightMouseDown:(NSEvent *)event {
    update_key_state(window_state, KEY_MOUSE_RIGHT, true);
}

- (void)rightMouseUp:(NSEvent *)event {
    update_key_state(window_state, KEY_MOUSE_RIGHT, false);
}

// - (void)otherMouseDown:(NSEvent *)event
// {
//     update_key_state(window_state, &input.keys[FIRST_MOUSE_BUTTON + (int) [event buttonNumber]], true);
// }

// - (void)otherMouseUp:(NSEvent *)event
// {
//     update_key_state(window_state, &input.keys[FIRST_MOUSE_BUTTON + (int) [event buttonNumber]], false);
// }

- (void)keyDown:(NSEvent *)event {
    u32 key = osx_scancode_to_key([event keyCode]);
    update_key_state(window_state, key, true);
}

- (void)keyUp:(NSEvent *)event {
    u32 key = osx_scancode_to_key([event keyCode]);
    update_key_state(window_state, key, true);
}

- (void)scrollWheel:(NSEvent *)event {
    f64 delta_y = [event scrollingDeltaY];
    if ([event hasPreciseScrollingDeltas]) {
        delta_y *= 0.1;
    }
    // @TODO(hl): Check for deadzone
    window_state->mwheel = delta_y;
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
}

- (void)doCommandBySelector:(SEL)selector {
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
}

- (void)unmarkText {
}

- (NSRange)selectedRange {
    NSRange range = { NSNotFound, 0 };
    return range;
}

- (NSRange)markedRange {
    NSRange range = { NSNotFound, 0 };
    return range;
}

- (BOOL)hasMarkedText {
    return NO;
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    return nil;
}

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText {
    return [NSArray array];
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    NSRect result = NSMakeRect(0, 0, 0, 0);
    return result;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
    return 0;
}

- (BOOL)canBecomeKeyView {
    return YES;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)wantsUpdateLayer {
    return YES;
}

@end 

void 
osx_create_window_internal(Window_State *state, u32 width, u32 height) {
    // @LEAK
    OSX_Window_State_Internal *osx_state = mem_alloc(sizeof(OSX_Window_State_Internal));
    state->internal = osx_state;
    NSRect screen_rect = [[NSScreen mainScreen] frame];
    NSRect window_rect = NSMakeRect((screen_rect.size.width - width) / 2,
        (screen_rect.size.height - height) / 2,
        width,
        height);

    osx_state->win = [[NSWindow alloc]
        initWithContentRect: window_rect
        styleMask: NSWindowStyleMaskTitled
                    | NSWindowStyleMaskClosable
                    | NSWindowStyleMaskMiniaturizable
                    | NSWindowStyleMaskResizable
        backing: NSBackingStoreBuffered
        defer: NO
    ];
    [osx_state->win setBackgroundColor: NSColor.blackColor];
    [osx_state->win setTitle: @"GOSUDAR"];
    [osx_state->win makeKeyAndOrderFront: nil];
    [osx_state->win setRestorable: NO];

    osx_state->win_delegate = [[OSX_Window_Delegate alloc] 
        init_with_window_state: state];
    [osx_state->win setDelegate: osx_state->win_delegate];

    osx_state->win_view = [[OSX_Content_View alloc] 
        init_with_window_state: state];
    [osx_state->win setContentView: osx_state->win_view];
    [osx_state->win_view setWantsLayer: YES];
    // @NOTE(hl): Load dll for metal layer
    // NSBundle *bundle = [NSBundle bundleWithPath:@"/System/Library/Frameworks/QuartzCore.framework"];
    // assert(bundle);
    // osx_state->layer = [[bundle classNamed:@"CAMetalLayer"] layer];
    // [osx_state->win_view setLayer: osx_state->layer];
}

void 
osx_poll_window_events_internal(Window_State *state) {
    OSX_Window_State_Internal *osx_state = (OSX_Window_State_Internal *)state->internal;
    for (;;) {
        NSEvent *event = [NSApp nextEventMatchingMask: NSEventMaskAny
            untilDate: nil
            inMode: NSDefaultRunLoopMode
            dequeue: YES];
        if (event == nil) {
            break;
        }
        [NSApp sendEvent: event];
    }
    
    NSSize osx_window_size = [[osx_state->win contentView] frame].size;
    Vec2 winsize = v2(osx_window_size.width,
        osx_window_size.height);
    state->display_size = winsize;

    NSPoint mouse_pos = osx_state->win.mouseLocationOutsideOfEventStream;
    Vec2 mouse = v2((i32)mouse_pos.x,
        (i32)(state->display_size.y - mouse_pos.y));
    state->mdelta = v2sub(mouse, state->mpos);
    state->mpos = mouse;
}