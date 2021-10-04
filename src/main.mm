#include "lib/filesystem.hh"
#include "lib/strings.hh"
#include "lib/logging.hh"
#include "lib/lists.hh"

#include "platform/platform_interface.hh"

#include "renderer/vulkan_interface.hh"

#include <AppKit/AppKit.h>
#include <MoltenVK/mvk_vulkan.h>

static PlatformInterface platform;

static KeyKind 
osx_scancode_to_key(u32 scancode) {
    KeyKind result = KEY_NONE;

    switch (scancode)
    {
#define KEYDEF(_a, _b) case _a: result = _b; break;
    // KEYDEF(0x1D, KEY_0)
    // KEYDEF(0x12, KEY_1)
    // KEYDEF(0x13, KEY_2)
    // KEYDEF(0x14, KEY_3)
    // KEYDEF(0x15, KEY_4)
    // KEYDEF(0x17, KEY_5)
    // KEYDEF(0x16, KEY_6)
    // KEYDEF(0x1A, KEY_7)
    // KEYDEF(0x1C, KEY_8)
    // KEYDEF(0x19, KEY_9)
    KEYDEF(0x00, KEY_A)
    KEYDEF(0x0B, KEY_B)
    // KEYDEF(0x08, KEY_C)
    KEYDEF(0x02, KEY_D)
    // KEYDEF(0x0E, KEY_E)
    // KEYDEF(0x03, KEY_F)
    // KEYDEF(0x05, KEY_G)
    // KEYDEF(0x04, KEY_H)
    // KEYDEF(0x22, KEY_I)
    // KEYDEF(0x26, KEY_J)
    // KEYDEF(0x28, KEY_K)
    // KEYDEF(0x25, KEY_L)
    // KEYDEF(0x2E, KEY_M)
    // KEYDEF(0x2D, KEY_N)
    // KEYDEF(0x1F, KEY_O)
    // KEYDEF(0x23, KEY_P)
    // KEYDEF(0x0C, KEY_Q)
    // KEYDEF(0x0F, KEY_R)
    KEYDEF(0x01, KEY_S)
    // KEYDEF(0x11, KEY_T)
    // KEYDEF(0x20, KEY_U)
    // KEYDEF(0x09, KEY_V)
    KEYDEF(0x0D, KEY_W)
    KEYDEF(0x07, KEY_X)
    // KEYDEF(0x10, KEY_Y)
    KEYDEF(0x06, KEY_Z)
    // KEYDEF(0x27, KEY_Apostrophe)
    // KEYDEF(0x2A, KEY_Backslash)
    // KEYDEF(0x2B, KEY_Comma)
    // KEYDEF(0x18, KEY_Equal)
    // KEYDEF(0x32, KEY_GraveAccent)
    // KEYDEF(0x21, KEY_LeftBracket)
    // KEYDEF(0x1B, KEY_Minus)
    // KEYDEF(0x2F, KEY_Period)
    // KEYDEF(0x1E, KEY_RightBracket)
    // KEYDEF(0x29, KEY_Semicolon)
    // KEYDEF(0x2C, KEY_Slash)
    // KEYDEF(0x0A, KEY_World_1)
    // KEYDEF(0x33, KEY_Backspace)
    // KEYDEF(0x39, KEY_CapsLock)
    // KEYDEF(0x75, KEY_Delete)
    // KEYDEF(0x7D, KEY_Down)
    // KEYDEF(0x77, KEY_End)
    // KEYDEF(0x24, KEY_Enter)
    // KEYDEF(0x35, KEY_Escape)
    KEYDEF(0x7A, KEY_F1)
    KEYDEF(0x78, KEY_F2)
    KEYDEF(0x63, KEY_F3)
    KEYDEF(0x76, KEY_F4)
    KEYDEF(0x60, KEY_F5)
    KEYDEF(0x61, KEY_F6)
    KEYDEF(0x62, KEY_F7)
    KEYDEF(0x64, KEY_F8)
    KEYDEF(0x65, KEY_F9)
    KEYDEF(0x6D, KEY_F10)
    KEYDEF(0x67, KEY_F11)
    KEYDEF(0x6F, KEY_F12)
    // KEYDEF(0x69, KEY_F13)
    // KEYDEF(0x6B, KEY_F14)
    // KEYDEF(0x71, KEY_F15)
    // KEYDEF(0x6A, KEY_F16)
    // KEYDEF(0x40, KEY_F17)
    // KEYDEF(0x4F, KEY_F18)
    // KEYDEF(0x50, KEY_F19)
    // KEYDEF(0x5A, KEY_F20)
    // KEYDEF(0x73, KEY_Home)
    // KEYDEF(0x72, KEY_Insert)
    // KEYDEF(0x7B, KEY_Left)
    // KEYDEF(0x3A, KEY_LeftAlt)
    // KEYDEF(0x3B, KEY_LeftControl)
    // KEYDEF(0x38, KEY_LeftShift)
    // KEYDEF(0x37, KEY_LeftSuper)
    // KEYDEF(0x6E, KEY_Menu)
    // KEYDEF(0x47, KEY_NumLock)
    // KEYDEF(0x79, KEY_PageDown)
    // KEYDEF(0x74, KEY_PageUp)
    // KEYDEF(0x7C, KEY_Right)
    // KEYDEF(0x3D, KEY_RightAlt)
    // KEYDEF(0x3E, KEY_RightControl)
    // KEYDEF(0x3C, KEY_RightShift)
    // KEYDEF(0x36, KEY_RightSuper)
    // KEYDEF(0x31, KEY_Space)
    // KEYDEF(0x30, KEY_Tab)
    // KEYDEF(0x7E, KEY_Up)
    // KEYDEF(0x52, KEY_KP_0)
    // KEYDEF(0x53, KEY_KP_1)
    // KEYDEF(0x54, KEY_KP_2)
    // KEYDEF(0x55, KEY_KP_3)
    // KEYDEF(0x56, KEY_KP_4)
    // KEYDEF(0x57, KEY_KP_5)
    // KEYDEF(0x58, KEY_KP_6)
    // KEYDEF(0x59, KEY_KP_7)
    // KEYDEF(0x5B, KEY_KP_8)
    // KEYDEF(0x5C, KEY_KP_9)
    // KEYDEF(0x45, KEY_KP_Add)
    // KEYDEF(0x41, KEY_KP_Decimal)
    // KEYDEF(0x4B, KEY_KP_Divide)
    // KEYDEF(0x4C, KEY_KP_Enter)
    // KEYDEF(0x51, KEY_KP_Equal)
    // KEYDEF(0x43, KEY_KP_Multiply)
    // KEYDEF(0x4E, KEY_KP_Subtract)
    }

    return result;
}

@interface OSXWindowDelegate : NSObject<NSWindowDelegate>
@end

@implementation OSXWindowDelegate
-(void)windowWillClose:(id)sender {
    platform.is_quit_requested = true;
}

@end

@interface OSXContentView : NSView<NSTextInputClient>
@end

@implementation OSXContentView
- (void)mouseDown:(NSEvent *)event {
    update_key_state(&platform, KEY_MOUSE_LEFT, true);
}

- (void)mouseUp:(NSEvent *)event {
    update_key_state(&platform, KEY_MOUSE_LEFT, false);
}

- (void)rightMouseDown:(NSEvent *)event {
    update_key_state(&platform, KEY_MOUSE_RIGHT, true);
}

- (void)rightMouseUp:(NSEvent *)event {
    update_key_state(&platform, KEY_MOUSE_RIGHT, false);
}

// - (void)otherMouseDown:(NSEvent *)event
// {
//     update_key_state(&platform, &input.keys[FIRST_MOUSE_BUTTON + (int) [event buttonNumber]], true);
// }

// - (void)otherMouseUp:(NSEvent *)event
// {
//     update_key_state(&platform, &input.keys[FIRST_MOUSE_BUTTON + (int) [event buttonNumber]], false);
// }

- (void)keyDown:(NSEvent *)event {
    KeyKind key = osx_scancode_to_key([event keyCode]);
    if (key) {
        update_key_state(&platform, key, true);
    }
}

- (void)keyUp:(NSEvent *)event {
    KeyKind key = osx_scancode_to_key([event keyCode]);
    if (key) {
        update_key_state(&platform, key, true);
    }
}

- (void)scrollWheel:(NSEvent *)event {
    f64 delta_y = [event scrollingDeltaY];
    if ([event hasPreciseScrollingDeltas]) {
        delta_y *= 0.1;
    }
    // @TODO(hl): Check for deadzone
    platform.mwheel = delta_y;
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
    return { NSNotFound, 0 };
}

- (NSRange)markedRange {
    return { NSNotFound, 0 };
}

- (BOOL)hasMarkedText {
    return NO;
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange
{
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

struct OSXState {
    NSWindow *win;
    OSXWindowDelegate *win_delegate;
    OSXContentView *win_view;
    CALayer *layer;

    VulkanCtx vulkan;
};

static OSXState state;

static void 
init(u32 width, u32 height) {
    NSRect screen_rect = [[NSScreen mainScreen] frame];
    NSRect window_rect = NSMakeRect((screen_rect.size.width   - width) / 2,
        (screen_rect.size.height - height) / 2,
        width,
        height);

    state.win = [[NSWindow alloc]
        initWithContentRect: window_rect
        styleMask: NSWindowStyleMaskTitled
                    | NSWindowStyleMaskClosable
                    | NSWindowStyleMaskMiniaturizable
                    | NSWindowStyleMaskResizable
        backing: NSBackingStoreBuffered
        defer: NO
    ];
    [state.win setBackgroundColor: NSColor.blackColor];
    [state.win setTitle: @"GOSUDAR"];
    [state.win makeKeyAndOrderFront: nil];
    [state.win setRestorable: NO];

    state.win_delegate = [[OSXWindowDelegate alloc] init];
    [state.win setDelegate: state.win_delegate];

    state.win_view = [[OSXContentView alloc] init];
    [state.win setContentView: state.win_view];
    [state.win_view setWantsLayer: YES];
    // @NOTE(hl): Load dll for metal layer
    NSBundle *bundle = [NSBundle bundleWithPath:@"/System/Library/Frameworks/QuartzCore.framework"];
    assert(bundle);
    state.layer = [[bundle classNamed:@"CAMetalLayer"] layer];
    [state.win_view setLayer: state.layer];
}

static void 
poll_events() {
    NSEvent *event;
    do {
        event = [NSApp nextEventMatchingMask: NSEventMaskAny
                untilDate: nil
                inMode: NSDefaultRunLoopMode
                dequeue: YES];
        switch([event type]) {
            default: {
                [NSApp sendEvent: event];
            } break;
        }
    }
    while (event != nil);

    NSSize osx_window_size = [[state.win contentView] frame].size;
    vec2 winsize = Vec2(osx_window_size.width,
        osx_window_size.height);
    platform.display_size = winsize;

    NSPoint mouse_pos = state.win.mouseLocationOutsideOfEventStream;
    vec2 mouse = Vec2((i32)mouse_pos.x,
        (i32)(platform.display_size.y - mouse_pos.y));
    platform.mdelta = mouse - platform.mpos;
    platform.mpos = mouse;
}

int 
main() {
    init_filesystem();
    init_logging("out.log");
    log_error("Hello");
    log_warn("Hello again");
    u32 width = 1280;
    u32 height = 720;
    init(width, height);
    init_vulkan(state.win_view);
    
    for (;;) {
        poll_events();
        
        if (platform.is_quit_requested) {
            break;
        }
    }
    outf("End of main\n");
    shutdown_logging();
    return 0;    
}