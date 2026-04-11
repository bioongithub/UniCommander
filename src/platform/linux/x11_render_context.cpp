#include "x11_render_context.h"
#include <cstring>

X11RenderContext::X11RenderContext(Display* dpy, Drawable win, GC gc, XFontStruct* fs)
    : m_dpy(dpy), m_win(win), m_gc(gc), m_fs(fs)
{
    m_ascent = fs ? fs->ascent  : 11;
    m_charH  = fs ? (fs->ascent + fs->descent) : 13;
}

unsigned long X11RenderContext::pixelFor(uc::Color c) const
{
    XColor xc = {};
    xc.red   = static_cast<unsigned short>(c.r * 257);
    xc.green = static_cast<unsigned short>(c.g * 257);
    xc.blue  = static_cast<unsigned short>(c.b * 257);
    xc.flags = DoRed | DoGreen | DoBlue;
    XAllocColor(m_dpy, DefaultColormap(m_dpy, DefaultScreen(m_dpy)), &xc);
    return xc.pixel;
}

void X11RenderContext::fillRect(int x, int y, int w, int h, uc::Color c)
{
    XSetForeground(m_dpy, m_gc, pixelFor(c));
    XFillRectangle(m_dpy, m_win, m_gc,
                   x, y,
                   static_cast<unsigned>(w), static_cast<unsigned>(h));
}

void X11RenderContext::drawRect(int x, int y, int w, int h, uc::Color c)
{
    XSetForeground(m_dpy, m_gc, pixelFor(c));
    XDrawRectangle(m_dpy, m_win, m_gc,
                   x, y,
                   static_cast<unsigned>(w - 1), static_cast<unsigned>(h - 1));
}

void X11RenderContext::drawText(int x, int y, const char* text, uc::Color fg)
{
    // y is the top of the row; XDrawString takes the baseline.
    int baseline = y + m_ascent;
    XSetForeground(m_dpy, m_gc, pixelFor(fg));
    XDrawString(m_dpy, m_win, m_gc, x, baseline,
                text, static_cast<int>(strlen(text)));
}

void X11RenderContext::setClip(int x, int y, int w, int h)
{
    XRectangle r = { static_cast<short>(x), static_cast<short>(y),
                     static_cast<unsigned short>(w), static_cast<unsigned short>(h) };
    XSetClipRectangles(m_dpy, m_gc, 0, 0, &r, 1, Unsorted);
}

void X11RenderContext::clearClip()
{
    XSetClipMask(m_dpy, m_gc, None);
}

int X11RenderContext::charHeight() const
{
    return m_charH;
}

int X11RenderContext::textWidth(const char* text, int len) const
{
    if (len < 0) len = static_cast<int>(strlen(text));
    if (!m_fs)   return len * 7;   // fallback estimate
    return XTextWidth(m_fs, text, len);
}
