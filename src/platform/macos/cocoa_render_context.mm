#import <Cocoa/Cocoa.h>
#include "cocoa_render_context.h"
#include <cstring>

// Helper: retrieve the stored NSFont* from the opaque void* member.
static NSFont* asFont(void* p) { return (__bridge NSFont*)p; }

// Helper: build an NSColor from a platform-neutral Color.
static NSColor* asColor(uc::Color c)
{
    return [NSColor colorWithRed:c.r / 255.0
                           green:c.g / 255.0
                            blue:c.b / 255.0
                           alpha:1.0];
}

CocoaRenderContext::CocoaRenderContext()
{
    NSFont* font = [NSFont userFixedPitchFontOfSize:12.0];
    m_font    = (__bridge_retained void*)font;
    // In a flipped view (isFlipped == YES) ascender > 0, descender < 0.
    m_ascent  = static_cast<int>( font.ascender);
    m_descent = static_cast<int>(-font.descender);  // store as positive
    m_charH   = m_ascent + m_descent;
}

CocoaRenderContext::~CocoaRenderContext()
{
    // Release the retained NSFont.
    CFRelease(m_font);
}

void CocoaRenderContext::fillRect(int x, int y, int w, int h, uc::Color c)
{
    [asColor(c) set];
    NSRectFill(NSMakeRect(x, y, w, h));
}

void CocoaRenderContext::drawRect(int x, int y, int w, int h, uc::Color c)
{
    [asColor(c) set];
    NSFrameRect(NSMakeRect(x, y, w, h));
}

void CocoaRenderContext::drawText(int x, int y, const char* text, uc::Color fg)
{
    // y is the top of the row. Cocoa's drawAtPoint uses the glyph baseline.
    // In a flipped view: baseline = top + ascent.
    NSFont* font  = asFont(m_font);
    NSString* str = [NSString stringWithUTF8String:text];
    NSDictionary* attrs = @{
        NSForegroundColorAttributeName: asColor(fg),
        NSFontAttributeName:            font
    };
    [str drawAtPoint:NSMakePoint(x, y + m_ascent - m_charH) withAttributes:attrs];
}

void CocoaRenderContext::setClip(int x, int y, int w, int h)
{
    NSRectClip(NSMakeRect(x, y, w, h));
}

void CocoaRenderContext::clearClip()
{
    // Restore infinite clip by clipping to an enormous rect.
    NSRectClip(NSMakeRect(-16000, -16000, 32000, 32000));
}

int CocoaRenderContext::textWidth(const char* text, int len) const
{
    NSFont*   font  = asFont(m_font);
    NSString* str   = (len < 0)
        ? [NSString stringWithUTF8String:text]
        : [NSString stringWithUTF8String:std::string(text, static_cast<size_t>(len)).c_str()];
    NSDictionary* attrs = @{ NSFontAttributeName: font };
    return static_cast<int>([str sizeWithAttributes:attrs].width);
}
