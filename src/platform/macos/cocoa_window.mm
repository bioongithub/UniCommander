#import <Cocoa/Cocoa.h>
#include "cocoa_window.h"

static const CGFloat DIVIDER_W = uc::BaseWindow::DIVIDER_W;
static const CGFloat HIT_ZONE  = uc::BaseWindow::HIT_ZONE;

// ── Content view ──────────────────────────────────────────────────────────

@interface UCContentView : NSView
{
    CGFloat _hRatio;
    CGFloat _vRatio;
    BOOL    _draggingH;
    BOOL    _draggingV;
}
@end

@implementation UCContentView

- (instancetype)initWithFrame:(NSRect)frame
{
    if ((self = [super initWithFrame:frame]))
    {
        _hRatio    = 0.5;
        _vRatio    = 0.5;
        _draggingH = NO;
        _draggingV = NO;
    }
    return self;
}

- (BOOL)isFlipped { return YES; }  // Y increases downward (matches Win32/X11)
- (BOOL)acceptsFirstResponder { return YES; }

// ── Drawing ───────────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)__unused dirty
{
    CGFloat W     = NSWidth(self.bounds);
    CGFloat H     = NSHeight(self.bounds);
    CGFloat topH  = H * _hRatio;
    CGFloat leftW = W * _vRatio;

    NSRect leftR   = NSMakeRect(0,              0,      leftW,                 topH);
    NSRect rightR  = NSMakeRect(leftW+DIVIDER_W, 0,     W - leftW - DIVIDER_W, topH);
    NSRect bottomR = NSMakeRect(0,              topH+DIVIDER_W, W,             H - topH - DIVIDER_W);
    NSRect hDivR   = NSMakeRect(0,              topH,   W,                     DIVIDER_W);
    NSRect vDivR   = NSMakeRect(leftW,          0,      DIVIDER_W,             topH);

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

// ── Cursor rects ──────────────────────────────────────────────────────────

- (void)resetCursorRects
{
    CGFloat W     = NSWidth(self.bounds);
    CGFloat H     = NSHeight(self.bounds);
    CGFloat topH  = H * _hRatio;
    CGFloat leftW = W * _vRatio;

    [self addCursorRect:NSMakeRect(0,              topH  - HIT_ZONE, W,         DIVIDER_W + HIT_ZONE*2)
                 cursor:[NSCursor resizeUpDownCursor]];
    [self addCursorRect:NSMakeRect(leftW - HIT_ZONE, 0,              DIVIDER_W + HIT_ZONE*2, topH)
                 cursor:[NSCursor resizeLeftRightCursor]];
}

// ── Mouse ─────────────────────────────────────────────────────────────────

- (void)mouseDown:(NSEvent*)event
{
    NSPoint p     = [self convertPoint:event.locationInWindow fromView:nil];
    CGFloat topH  = NSHeight(self.bounds) * _hRatio;
    CGFloat leftW = NSWidth(self.bounds)  * _vRatio;

    _draggingH = (p.y >= topH - HIT_ZONE && p.y <= topH + DIVIDER_W + HIT_ZONE);
    _draggingV = (!_draggingH && p.y < topH &&
                  p.x >= leftW - HIT_ZONE && p.x <= leftW + DIVIDER_W + HIT_ZONE);
}

- (void)mouseDragged:(NSEvent*)event
{
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    CGFloat W = NSWidth(self.bounds), H = NSHeight(self.bounds);

    if (_draggingH && H > 0)
    {
        _hRatio = static_cast<CGFloat>(std::clamp(static_cast<float>(p.y / H), 0.1f, 0.9f));
        [self setNeedsDisplay:YES];
        [self.window invalidateCursorRectsForView:self];
    }
    else if (_draggingV && W > 0)
    {
        _vRatio = static_cast<CGFloat>(std::clamp(static_cast<float>(p.x / W), 0.1f, 0.9f));
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

// ── AppDelegate ───────────────────────────────────────────────────────────

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)__unused sender
{
    return YES;
}
@end

// ── CocoaWindow ───────────────────────────────────────────────────────────

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

    UCContentView* view = [[UCContentView alloc] initWithFrame:frame];
    [m_window setContentView:view];
    [m_window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    [m_window center];
    return m_window != nullptr;
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

std::unique_ptr<uc::Window> createWindow()
{
    return std::make_unique<CocoaWindow>();
}
