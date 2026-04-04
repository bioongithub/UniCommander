#import <Cocoa/Cocoa.h>
#include "cocoa_window.h"
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
    CGFloat W = NSWidth(self.bounds);
    CGFloat H = NSHeight(self.bounds);
    auto [topH, leftW] = _owner->computeLayout(static_cast<int>(W), static_cast<int>(H));

    NSRect leftR   = NSMakeRect(0,              0,               leftW,                 topH);
    NSRect rightR  = NSMakeRect(leftW+DIVIDER_W, 0,              W - leftW - DIVIDER_W, topH);
    NSRect bottomR = NSMakeRect(0,              topH+DIVIDER_W,  W,                     H - topH - DIVIDER_W);
    NSRect hDivR   = NSMakeRect(0,              topH,            W,                     DIVIDER_W);
    NSRect vDivR   = NSMakeRect(leftW,          0,               DIVIDER_W,             topH);

    [[NSColor colorWithWhite:0.31 alpha:1.0] set];
    NSRectFill(hDivR);
    NSRectFill(vDivR);

    [[NSColor colorWithWhite:0.12 alpha:1.0] set];
    NSRectFill(leftR);
    NSRectFill(rightR);
    NSRectFill(bottomR);

    NSDictionary* attrs = @{
        NSForegroundColorAttributeName: [NSColor colorWithWhite:0.86 alpha:1.0],
        NSFontAttributeName:            [NSFont systemFontOfSize:13.0]
    };

    auto drawCentered = [&](NSString* text, NSRect rect) {
        NSSize sz = [text sizeWithAttributes:attrs];
        NSPoint p = NSMakePoint(NSMidX(rect) - sz.width  / 2.0,
                                NSMidY(rect) - sz.height / 2.0);
        [text drawAtPoint:p withAttributes:attrs];
    };

    drawCentered(@"left",   leftR);
    drawCentered(@"right",  rightR);
    drawCentered(@"bottom", bottomR);
}

// --- Cursor rects ---
- (void)resetCursorRects
{
    CGFloat W = NSWidth(self.bounds);
    CGFloat H = NSHeight(self.bounds);
    auto [topH, leftW] = _owner->computeLayout(static_cast<int>(W), static_cast<int>(H));

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
        case 126: _owner->handleKeyDown(Key::Up);     break;  // arrow up
        case 125: _owner->handleKeyDown(Key::Down);   break;  // arrow down
        case 36:  _owner->handleKeyDown(Key::Return); break;  // return
        case 48:  _owner->handleKeyDown(Key::Tab);    break;  // tab
        case 53:  _owner->handleKeyDown(Key::Escape); break;  // escape
        case 12:  _owner->handleKeyDown(Key::Q);      break;  // q
        default:  [super keyDown:event]; break;
    }
}

// --- Mouse ---
- (void)mouseDown:(NSEvent*)event
{
    [self.window makeFirstResponder:self];

    NSPoint p  = [self convertPoint:event.locationInWindow fromView:nil];
    CGFloat W  = NSWidth(self.bounds);
    CGFloat H  = NSHeight(self.bounds);
    Hit     hit = _owner->hitTest(static_cast<int>(p.x), static_cast<int>(p.y),
                                  static_cast<int>(W),   static_cast<int>(H));

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
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    CGFloat W = NSWidth(self.bounds), H = NSHeight(self.bounds);

    if (_draggingH && H > 0)
    {
        _owner->setHRatio(static_cast<float>(p.y / H));
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

- (void)mouseUp:(NSEvent*)__unused event
{
    _draggingH = NO;
    _draggingV = NO;
}

@end

// --- AppDelegate ---
@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)__unused sender
{
    return YES;
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
    if (m_window)
        [m_window close];
}

void CocoaWindow::invalidate()
{
    if (m_window)
        [m_window.contentView setNeedsDisplay:YES];
}

std::unique_ptr<uc::Window> createWindow()
{
    return std::make_unique<CocoaWindow>();
}
