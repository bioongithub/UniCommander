#pragma once
#include <X11/Xlib.h>
#include "ui/render_context.h"

class X11RenderContext : public uc::RenderContext
{
public:
    X11RenderContext(Display* dpy, Drawable win, GC gc, XFontStruct* fs);

    void fillRect(int x, int y, int w, int h, uc::Color c) override;
    void drawRect(int x, int y, int w, int h, uc::Color c) override;
    void drawText(int x, int y, const char* text, uc::Color fg) override;
    void setClip(int x, int y, int w, int h) override;
    void clearClip() override;
    int  charHeight()                            const override;
    int  textWidth(const char* text, int len=-1) const override;

private:
    Display*     m_dpy;
    Drawable     m_win;
    GC           m_gc;
    XFontStruct* m_fs;    // may be nullptr; fallback metrics used when null
    int          m_ascent;
    int          m_charH;

    unsigned long pixelFor(uc::Color c) const;
};
