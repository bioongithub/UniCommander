#import <Cocoa/Cocoa.h>
#include "cocoa_window.h"
#include "cocoa_render_context.h"
#include <filesystem>

static const CGFloat DIVIDER_W = uc::BaseWindow::DIVIDER_W;
static const CGFloat HIT_ZONE  = uc::BaseWindow::HIT_ZONE;

using Hit = uc::BaseWindow::Hit;

// --- Content view ---
@interface UCContentView : NSView
{
    BOOL         _draggingH;
    BOOL         _draggingV;
    CocoaWindow* _owner;    // non-owning; lifetime guaranteed by CocoaWindow
}
- (instancetype)initWithFrame:(NSRect)frame owner:(CocoaWindow*)owner;
@end

@implementation UCContentView

- (instancetype)initWithFrame:(NSRect)frame owner:(CocoaWindow*)owner
{
    if ((self = [super initWithFrame:frame]))
    {
        _draggingH = NO;
        _draggingV = NO;
        _owner     = owner;
    }
    return self;
}

- (BOOL)isFlipped { return YES; }  // Y increases downward (matches Win32/X11)
- (BOOL)acceptsFirstResponder { return YES; }

// --- Drawing ---
- (void)drawRect:(NSRect)__unused dirty
{
    CGFloat W    = NSWidth(self.bounds);
    CGFloat H    = NSHeight(self.bounds);
    CGFloat effH = H - FKEY_H;
    auto [topH, leftW] = _owner->computeLayout(static_cast<int>(W), static_cast<int>(effH));

    // Dividers
    [[NSColor colorWithWhite:0.31 alpha:1.0] set];
    NSRectFill(NSMakeRect(0, topH, W, DIVIDER_W));
    NSRectFill(NSMakeRect(leftW, 0, DIVIDER_W, topH));

    // Panel widgets
    CocoaRenderContext ctx;
    _owner->layoutPanels(static_cast<int>(W), static_cast<int>(H));
    if (_owner->leftWidget())  _owner->leftWidget()->paint(ctx);
    if (_owner->rightWidget()) _owner->rightWidget()->paint(ctx);
    if (_owner->termWidget())  _owner->termWidget()->paint(ctx);
    if (_owner->fkeyWidget())  _owner->fkeyWidget()->paint(ctx);

    // Modal widget overlays (drawn last, on top of everything)
    _owner->helpWidget().layout(0, 0, static_cast<int>(W), static_cast<int>(H));
    _owner->helpWidget().paint(ctx);
    _owner->confirmWidget().layout(0, 0, static_cast<int>(W), static_cast<int>(H));
    _owner->confirmWidget().paint(ctx);
}

// --- Cursor rects ---
- (void)resetCursorRects
{
    CGFloat W    = NSWidth(self.bounds);
    CGFloat H    = NSHeight(self.bounds);
    CGFloat effH = H - FKEY_H;
    auto [topH, leftW] = _owner->computeLayout(static_cast<int>(W), static_cast<int>(effH));

    [self addCursorRect:NSMakeRect(0,               topH - HIT_ZONE,  W,                    DIVIDER_W + HIT_ZONE*2)
                 cursor:[NSCursor resizeUpDownCursor]];
    [self addCursorRect:NSMakeRect(leftW - HIT_ZONE, 0,               DIVIDER_W + HIT_ZONE*2, topH)
                 cursor:[NSCursor resizeLeftRightCursor]];
}

// --- Resize ---
- (void)setFrameSize:(NSSize)newSize
{
    [super setFrameSize:newSize];
    _owner->setSize(static_cast<int>(newSize.width), static_cast<int>(newSize.height));
}

// --- Keyboard ---
- (void)keyDown:(NSEvent*)event
{
    using Key = uc::BaseWindow::Key;
    switch (event.keyCode)
    {
        case 122: _owner->handleKeyDown(Key::F1);     break;  // F1
        case 120: _owner->handleKeyDown(Key::F2);     break;  // F2
        case 99:  _owner->handleKeyDown(Key::F3);     break;  // F3
        case 118: _owner->handleKeyDown(Key::F4);     break;  // F4
        case 96:  _owner->handleKeyDown(Key::F5);     break;  // F5
        case 97:  _owner->handleKeyDown(Key::F6);     break;  // F6
        case 98:  _owner->handleKeyDown(Key::F7);     break;  // F7
        case 100: _owner->handleKeyDown(Key::F8);     break;  // F8
        case 101: _owner->handleKeyDown(Key::F9);     break;  // F9
        case 126: _owner->handleKeyDown(Key::Up);     break;  // arrow up
        case 125: _owner->handleKeyDown(Key::Down);   break;  // arrow down
        case 123: _owner->handleKeyDown(Key::Left);   break;  // arrow left
        case 124: _owner->handleKeyDown(Key::Right);  break;  // arrow right
        case 36:  _owner->handleKeyDown(Key::Return); break;  // return
        case 48:  _owner->handleKeyDown(Key::Tab);    break;  // tab
        case 53:  _owner->handleKeyDown(Key::Escape); break;  // escape
        case 12:  _owner->handleKeyDown(Key::Q);      break;  // q
        case 109: _owner->handleKeyDown(Key::F10);    break;  // F10
        default:  [super keyDown:event]; break;
    }
}

// --- Mouse ---
- (void)mouseDown:(NSEvent*)event
{
    [self.window makeFirstResponder:self];

    NSPoint p    = [self convertPoint:event.locationInWindow fromView:nil];
    CGFloat W    = NSWidth(self.bounds);
    CGFloat H    = NSHeight(self.bounds);
    CGFloat effH = H - FKEY_H;

    // Modal dialog captures all clicks when visible.
    if (_owner->confirmWidget().isVisible())
    {
        _owner->confirmWidget().handleMouseDown(
            static_cast<int>(p.x), static_cast<int>(p.y));
        return;
    }

    // F-key bar occupies the bottom FKEY_H rows.
    if (p.y >= effH)
    {
        CGFloat effW  = W - MOD_AREA_W;
        CGFloat cellW = effW / 10.0;
        if (p.x < effW)
        {
            int slot = static_cast<int>(p.x / cellW);
            slot = std::min(slot, 9);
            using Key = uc::BaseWindow::Key;
            Key key = static_cast<Key>(static_cast<int>(Key::F1) + slot);
            _owner->handleKeyDown(key);
        }
        else
        {
            int modIdx = static_cast<int>((p.x - effW) / MOD_CELL_W);
            if (modIdx >= 0 && modIdx < 3)
            {
                _owner->toggleModifierSticky(static_cast<uc::BaseWindow::Mod>(modIdx));
                [self setNeedsDisplay:YES];
            }
        }
        return;
    }

    Hit hit = _owner->hitTest(static_cast<int>(p.x), static_cast<int>(p.y),
                              static_cast<int>(W),   static_cast<int>(effH));

    _draggingH = (hit == Hit::HorizDivider);
    _draggingV = (hit == Hit::VertDivider);

    if (!_draggingH && !_draggingV)
    {
        auto* left  = _owner->leftPanel();
        auto* right = _owner->rightPanel();
        if (left && right && (hit == Hit::LeftPanel || hit == Hit::RightPanel))
        {
            left->setFocus(hit == Hit::LeftPanel);
            right->setFocus(hit == Hit::RightPanel);
            [self setNeedsDisplay:YES];
        }
    }
}

- (void)mouseDragged:(NSEvent*)event
{
    NSPoint p    = [self convertPoint:event.locationInWindow fromView:nil];
    CGFloat W    = NSWidth(self.bounds);
    CGFloat H    = NSHeight(self.bounds);
    CGFloat effH = H - FKEY_H;

    if (_draggingH && effH > 0)
    {
        _owner->setHRatio(static_cast<float>(p.y / effH));
        [self setNeedsDisplay:YES];
        [self.window invalidateCursorRectsForView:self];
    }
    else if (_draggingV && W > 0)
    {
        _owner->setVRatio(static_cast<float>(p.x / W));
        [self setNeedsDisplay:YES];
        [self.window invalidateCursorRectsForView:self];
    }
}

- (void)flagsChanged:(NSEvent*)event
{
    NSEventModifierFlags flags = event.modifierFlags;
    using Mod = uc::BaseWindow::Mod;
    _owner->setModifierPhysical(Mod::Alt,   (flags & NSEventModifierFlagOption)  != 0);
    _owner->setModifierPhysical(Mod::Shift, (flags & NSEventModifierFlagShift)   != 0);
    _owner->setModifierPhysical(Mod::Ctrl,  (flags & NSEventModifierFlagControl) != 0);
    [self setNeedsDisplay:YES];
}

- (void)mouseUp:(NSEvent*)__unused event
{
    _draggingH = NO;
    _draggingV = NO;
}

@end

// --- Window delegate (handles close button and Cmd+Q) ---
@interface UCWindowDelegate : NSObject <NSWindowDelegate>
{
    CocoaWindow* _owner;
}
- (instancetype)initWithOwner:(CocoaWindow*)owner;
@end

@implementation UCWindowDelegate
- (instancetype)initWithOwner:(CocoaWindow*)owner
{
    if ((self = [super init]))
        _owner = owner;
    return self;
}
- (BOOL)windowShouldClose:(id)__unused sender
{
    return (_owner->isClosing() || _owner->confirmQuit()) ? YES : NO;
}
@end

// --- AppDelegate ---
@interface AppDelegate : NSObject <NSApplicationDelegate>
{
    CocoaWindow* _owner;
}
- (void)setOwner:(CocoaWindow*)owner;
@end

@implementation AppDelegate
- (void)setOwner:(CocoaWindow*)owner { _owner = owner; }
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)__unused sender
{
    return YES;
}
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)__unused sender
{
    if (_owner && !_owner->confirmQuit())
        return NSTerminateCancel;
    return NSTerminateNow;
}
@end

// --- CocoaWindow ---
CocoaWindow::CocoaWindow()  = default;
CocoaWindow::~CocoaWindow() = default;

bool CocoaWindow::create(const std::string& title, int width, int height)
{
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    m_delegate = [[AppDelegate alloc] init];
    [m_delegate setOwner:this];
    [NSApp setDelegate:m_delegate];

    NSRect frame = NSMakeRect(0, 0, width, height);
    NSUInteger style = NSWindowStyleMaskTitled
                     | NSWindowStyleMaskClosable
                     | NSWindowStyleMaskResizable
                     | NSWindowStyleMaskMiniaturizable;

    m_window = [[NSWindow alloc] initWithContentRect:frame
                                           styleMask:style
                                             backing:NSBackingStoreBuffered
                                               defer:NO];
    if (!m_window) return false;

    initPanels(std::filesystem::current_path().string());

    UCContentView* view = [[UCContentView alloc] initWithFrame:frame owner:this];
    [m_window setContentView:view];
    [m_window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    [m_window makeFirstResponder:view];
    [m_window center];

    m_winDelegate = [[UCWindowDelegate alloc] initWithOwner:this];
    [m_window setDelegate:m_winDelegate];

    return true;
}

void CocoaWindow::show()
{
    if (m_window)
        [m_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

void CocoaWindow::run()
{
    [NSApp run];
}

void CocoaWindow::close()
{
    m_closing = true;
    if (m_window)
        [m_window close];
}

void CocoaWindow::invalidate()
{
    if (m_window)
        [m_window.contentView setNeedsDisplay:YES];
}

void CocoaWindow::pumpEventsUntil(std::function<bool()> done)
{
    while (!done())
    {
        NSEvent* ev = [NSApp nextEventMatchingMask:NSEventMaskAny
                                         untilDate:[NSDate distantFuture]
                                            inMode:NSDefaultRunLoopMode
                                           dequeue:YES];
        if (ev)
            [NSApp sendEvent:ev];
    }
}

std::unique_ptr<uc::Window> createWindow()
{
    return std::make_unique<CocoaWindow>();
}
