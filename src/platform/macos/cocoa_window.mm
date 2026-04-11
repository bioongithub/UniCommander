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
    CGFloat W    = NSWidth(self.bounds);
    CGFloat H    = NSHeight(self.bounds);
    CGFloat effH = H - FKEY_H;            // height available above the F-key bar
    auto [topH, leftW] = _owner->computeLayout(static_cast<int>(W), static_cast<int>(effH));

    NSRect hDivR   = NSMakeRect(0,              topH,           W,        DIVIDER_W);
    NSRect vDivR   = NSMakeRect(leftW,          0,              DIVIDER_W, topH);
    NSRect bottomR = NSMakeRect(0,              topH+DIVIDER_W, W,        effH - topH - DIVIDER_W);

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

    // F-key bar
    [self renderFKeyBar:NSMakeRect(0, effH, W, FKEY_H)];

    // Help overlay (drawn last, on top of everything)
    if (_owner->helpWindow().isVisible())
        [self renderHelpWindow:self.bounds];
}

- (void)renderFKeyBar:(NSRect)bar
{
    NSFont* font   = [NSFont userFixedPitchFontOfSize:11.0];
    CGFloat fontH  = font.ascender - font.descender;
    CGFloat textY  = bar.origin.y + (FKEY_H - fontH) / 2.0 - font.descender;
    CGFloat effW   = bar.size.width - MOD_AREA_W;
    CGFloat cellW  = effW / 10.0;
    CGFloat numW   = 16.0;

    // --- F-key cells ---
    for (int i = 0; i < 10; ++i)
    {
        CGFloat x = bar.origin.x + i * cellW;

        // Number sub-column
        NSRect numR = NSMakeRect(x, bar.origin.y, numW, FKEY_H);
        [[NSColor blackColor] set];
        NSRectFill(numR);
        NSString* numStr = [NSString stringWithFormat:@"%d", i + 1];
        NSSize numSz = [numStr sizeWithAttributes:@{NSFontAttributeName: font}];
        NSDictionary* numAttrs = @{
            NSForegroundColorAttributeName: [NSColor colorWithRed:1.0 green:1.0 blue:0.33 alpha:1.0],
            NSFontAttributeName:            font
        };
        [numStr drawAtPoint:NSMakePoint(x + (numW - numSz.width) / 2.0, textY)
             withAttributes:numAttrs];

        // Label sub-column
        CGFloat lblW = cellW - numW;
        NSRect  lblR = NSMakeRect(x + numW, bar.origin.y, lblW, FKEY_H);
        const char* lbl = uc::BaseWindow::FKEY_LABELS[_owner->activeModifierRow()][i];
        if (lbl[0] != '\0')
        {
            [[NSColor colorWithRed:0.0 green:0.67 blue:0.67 alpha:1.0] set];
            NSRectFill(lblR);
            NSString* lblStr = [NSString stringWithUTF8String:lbl];
            NSDictionary* lblAttrs = @{
                NSForegroundColorAttributeName: [NSColor blackColor],
                NSFontAttributeName:            font
            };
            [lblStr drawAtPoint:NSMakePoint(x + numW + 2.0, textY) withAttributes:lblAttrs];
        }
        else
        {
            [[NSColor blackColor] set];
            NSRectFill(lblR);
        }
    }

    // --- Modifier cells ---
    static const char* MOD_LABELS[3] = { "Alt", "Sft", "Ctl" };
    for (int i = 0; i < 3; ++i)
    {
        CGFloat x      = bar.origin.x + effW + i * MOD_CELL_W;
        bool    active = _owner->modifierActive(static_cast<uc::BaseWindow::Mod>(i));
        NSRect  modR   = NSMakeRect(x, bar.origin.y, MOD_CELL_W, FKEY_H);
        if (active)
            [[NSColor colorWithRed:0.67 green:0.67 blue:0.0 alpha:1.0] set];
        else
            [[NSColor colorWithRed:0.0  green:0.0  blue:0.39 alpha:1.0] set];
        NSRectFill(modR);

        NSString* modStr = [NSString stringWithUTF8String:MOD_LABELS[i]];
        NSColor*  modFg  = active ? [NSColor blackColor] : [NSColor colorWithWhite:0.78 alpha:1.0];
        NSDictionary* modAttrs = @{NSForegroundColorAttributeName: modFg, NSFontAttributeName: font};
        NSSize modSz = [modStr sizeWithAttributes:modAttrs];
        [modStr drawAtPoint:NSMakePoint(x + (MOD_CELL_W - modSz.width) / 2.0, textY)
             withAttributes:modAttrs];
    }
}

- (void)renderHelpWindow:(NSRect)bounds
{
    auto box = uc::HelpWindow::computeBox(static_cast<int>(NSWidth(bounds)),
                                          static_cast<int>(NSHeight(bounds)));
    NSRect boxR = NSMakeRect(box.x, box.y, box.w, box.h);

    // Background
    [[NSColor colorWithRed:0.08 green:0.08 blue:0.20 alpha:1.0] set];
    NSRectFill(boxR);

    // Border
    [[NSColor colorWithRed:0.39 green:0.71 blue:1.0 alpha:1.0] set];
    NSFrameRectWithWidth(boxR, 2.0);

    // Text lines
    NSFont* font = [NSFont userFixedPitchFontOfSize:12.0];
    CGFloat fontH = font.ascender - font.descender;
    CGFloat y = box.y + uc::HelpWindow::PADDING;

    for (int i = 0; uc::HelpWindow::LINES[i]; ++i)
    {
        NSColor* col = (i == 0)
            ? [NSColor colorWithRed:0.39 green:0.71 blue:1.0 alpha:1.0]
            : [NSColor colorWithWhite:0.86 alpha:1.0];
        NSDictionary* attrs = @{
            NSForegroundColorAttributeName: col,
            NSFontAttributeName:            font
        };
        NSString* line = [NSString stringWithUTF8String:uc::HelpWindow::LINES[i]];
        // Adjust baseline: drawAtPoint uses the font baseline, not the top
        [line drawAtPoint:NSMakePoint(box.x + uc::HelpWindow::PADDING,
                                      y + (uc::HelpWindow::LINE_H - fontH) / 2.0
                                        - font.descender)
           withAttributes:attrs];
        y += uc::HelpWindow::LINE_H;
    }
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

bool CocoaWindow::confirmQuit()
{
    if (m_testDialogAnswer.has_value())
    {
        bool ans = *m_testDialogAnswer;
        m_testDialogAnswer.reset();
        return ans;
    }
    NSAlert* alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Quit UniCommander?"];
    [alert setInformativeText:@"Are you sure you want to quit?"];
    [alert addButtonWithTitle:@"Yes"];
    [alert addButtonWithTitle:@"No"];
    [alert setAlertStyle:NSAlertStyleWarning];
    return [alert runModal] == NSAlertFirstButtonReturn;
}

bool CocoaWindow::confirmCopy(const std::string& srcName, const std::string& dstPath)
{
    if (m_testDialogAnswer.has_value())
    {
        bool ans = *m_testDialogAnswer;
        m_testDialogAnswer.reset();
        return ans;
    }
    NSString* info = [NSString stringWithFormat:@"Copy \"%s\" to %s?",
                      srcName.c_str(), dstPath.c_str()];
    NSAlert* alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Copy"];
    [alert setInformativeText:info];
    [alert addButtonWithTitle:@"Copy"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert setAlertStyle:NSAlertStyleInformational];
    return [alert runModal] == NSAlertFirstButtonReturn;
}

std::unique_ptr<uc::Window> createWindow()
{
    return std::make_unique<CocoaWindow>();
}
