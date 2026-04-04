#import <Cocoa/Cocoa.h>
#include "cocoa_window.h"
#include <filesystem>

static const CGFloat DIVIDER_W = uc::BaseWindow::DIVIDER_W;
static const CGFloat HIT_ZONE  = uc::BaseWindow::HIT_ZONE;
static const int     ROW_H     = 18;
static const int     HEADER_H  = 20;

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
- (void)renderDirectoryPanel:(NSRect)r panel:(uc::DirectoryPanel&)panel
{
    NSFont* font      = [NSFont userFixedPitchFontOfSize:12.0];
    CGFloat fontH     = font.ascender - font.descender;
    CGFloat baselineOff = (ROW_H - fontH) / 2.0 + font.descender;

    int visibleRows = std::max(0, (int)(NSHeight(r) - HEADER_H - 2) / ROW_H);
    panel.ensureVisible(visibleRows);

    // Background
    [[NSColor colorWithRed:0.08 green:0.08 blue:0.08 alpha:1.0] set];
    NSRectFill(r);

    // Border
    NSColor* borderColor = panel.hasFocus()
        ? [NSColor colorWithRed:0.0  green:0.47 blue:0.84 alpha:1.0]
        : [NSColor colorWithWhite:0.31 alpha:1.0];
    [borderColor set];
    NSFrameRect(r);

    // Header background
    NSRect headerR = NSMakeRect(r.origin.x + 1, r.origin.y + 1,
                                r.size.width - 2, HEADER_H);
    [[NSColor colorWithRed:0.16 green:0.16 blue:0.24 alpha:1.0] set];
    NSRectFill(headerR);

    // Header path text
    NSString* path = [NSString stringWithUTF8String:panel.getPath().c_str()];
    NSDictionary* headerAttrs = @{
        NSForegroundColorAttributeName: [NSColor colorWithRed:0.71 green:0.71 blue:1.0 alpha:1.0],
        NSFontAttributeName: font
    };
    CGFloat headerTextY = r.origin.y + 1 + (HEADER_H - fontH) / 2.0 - font.descender;
    [path drawAtPoint:NSMakePoint(r.origin.x + 4, headerTextY) withAttributes:headerAttrs];

    // Entry rows
    const auto& entries    = panel.entries();
    int         scrollOff  = panel.scrollOffset();
    int         selectedIdx = panel.selectedIndex();
    CGFloat y = r.origin.y + 1 + HEADER_H;

    for (int i = scrollOff;
         i < (int)entries.size() && y + ROW_H <= r.origin.y + r.size.height - 1;
         ++i, y += ROW_H)
    {
        bool selected = (i == selectedIdx);
        NSRect rowR = NSMakeRect(r.origin.x + 1, y, r.size.width - 2, ROW_H);

        if (selected)
        {
            NSColor* selColor = panel.hasFocus()
                ? [NSColor colorWithRed:0.0 green:0.31 blue:0.63 alpha:1.0]
                : [NSColor colorWithWhite:0.22 alpha:1.0];
            [selColor set];
            NSRectFill(rowR);
        }

        const auto& entry = entries[i];
        NSColor* textColor;
        if (selected)
            textColor = [NSColor whiteColor];
        else if (entry.isDir)
            textColor = [NSColor colorWithRed:0.39 green:0.71 blue:1.0 alpha:1.0];
        else
            textColor = [NSColor colorWithWhite:0.86 alpha:1.0];

        std::string labelStr = (entry.isDir && entry.name != "..")
            ? "[" + entry.name + "]" : entry.name;
        NSString* label = [NSString stringWithUTF8String:labelStr.c_str()];
        NSDictionary* rowAttrs = @{
            NSForegroundColorAttributeName: textColor,
            NSFontAttributeName: font
        };
        [label drawAtPoint:NSMakePoint(r.origin.x + 4, y + baselineOff)
            withAttributes:rowAttrs];
    }
}

- (void)drawRect:(NSRect)__unused dirty
{
    CGFloat W = NSWidth(self.bounds);
    CGFloat H = NSHeight(self.bounds);
    auto [topH, leftW] = _owner->computeLayout(static_cast<int>(W), static_cast<int>(H));

    NSRect hDivR   = NSMakeRect(0,              topH,           W,        DIVIDER_W);
    NSRect vDivR   = NSMakeRect(leftW,          0,              DIVIDER_W, topH);
    NSRect bottomR = NSMakeRect(0,              topH+DIVIDER_W, W,        H - topH - DIVIDER_W);

    // Dividers
    [[NSColor colorWithWhite:0.31 alpha:1.0] set];
    NSRectFill(hDivR);
    NSRectFill(vDivR);

    // Directory panels
    NSRect leftR  = NSMakeRect(0,              0, leftW,                 topH);
    NSRect rightR = NSMakeRect(leftW+DIVIDER_W, 0, W - leftW - DIVIDER_W, topH);
    if (_owner->leftPanel())
        [self renderDirectoryPanel:leftR  panel:*_owner->leftPanel()];
    if (_owner->rightPanel())
        [self renderDirectoryPanel:rightR panel:*_owner->rightPanel()];

    // Bottom panel
    [[NSColor colorWithWhite:0.12 alpha:1.0] set];
    NSRectFill(bottomR);

    NSDictionary* attrs = @{
        NSForegroundColorAttributeName: [NSColor colorWithWhite:0.86 alpha:1.0],
        NSFontAttributeName:            [NSFont systemFontOfSize:13.0]
    };
    NSString* label = @"Terminal";
    NSSize sz = [label sizeWithAttributes:attrs];
    [label drawAtPoint:NSMakePoint(NSMidX(bottomR) - sz.width  / 2.0,
                                   NSMidY(bottomR) - sz.height / 2.0)
        withAttributes:attrs];
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
