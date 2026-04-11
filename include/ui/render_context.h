#pragma once
#include <cstdint>
#include <cstring>

namespace uc {

// Platform-neutral color (no alpha; this app does not need transparency).
struct Color
{
    uint8_t r, g, b;
    constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
};

// --- RenderContext -------------------------------------------------------
// Abstract drawing surface passed to Widget::paint().
// Each platform provides a concrete subclass backed by HDC / GC / NSView.
//
// Coordinate system: (0,0) is top-left, y increases downward -- consistent
// with Win32 and X11. The Cocoa implementation flips internally (isFlipped).
//
// drawText convention: (x, y) is the TOP-LEFT of the text row. The
// implementation vertically centers the glyph within charHeight() pixels.

class RenderContext
{
public:
    virtual ~RenderContext() = default;

    // --- Primitives ---
    virtual void fillRect(int x, int y, int w, int h, Color c)        = 0;
    virtual void drawRect(int x, int y, int w, int h, Color c)        = 0; // outline
    virtual void drawText(int x, int y, const char* text, Color fg)   = 0;

    // --- Clip region ---
    virtual void setClip(int x, int y, int w, int h) = 0;
    virtual void clearClip()                         = 0;

    // --- Font metrics ---
    virtual int charHeight()                            const = 0; // ascent + descent
    virtual int textWidth(const char* text, int len=-1) const = 0; // pixel width
};

} // namespace uc
