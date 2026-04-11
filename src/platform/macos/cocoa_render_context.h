#pragma once
#include "ui/render_context.h"

// NSFont* is stored as void* so this header compiles from plain C++ translation
// units. The .mm implementation casts it back via __bridge.
#ifdef __OBJC__
@class NSFont;
#endif

class CocoaRenderContext : public uc::RenderContext
{
public:
    CocoaRenderContext();   // loads fixed-pitch font, computes metrics
    ~CocoaRenderContext();

    void fillRect(int x, int y, int w, int h, uc::Color c) override;
    void drawRect(int x, int y, int w, int h, uc::Color c) override;
    void drawText(int x, int y, const char* text, uc::Color fg) override;
    void setClip(int x, int y, int w, int h) override;
    void clearClip() override;
    int  charHeight()                            const override { return m_charH;  }
    int  textWidth(const char* text, int len=-1) const override;

private:
    void* m_font;   // NSFont* retained via CFRetain
    int   m_charH;
    int   m_ascent;
    int   m_descent;
};
