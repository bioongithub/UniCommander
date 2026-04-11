#pragma once
#define NOMINMAX
#include <windows.h>
#include "ui/render_context.h"

class Win32RenderContext : public uc::RenderContext
{
public:
    explicit Win32RenderContext(HDC hdc);

    void fillRect(int x, int y, int w, int h, uc::Color c) override;
    void drawRect(int x, int y, int w, int h, uc::Color c) override;
    void drawText(int x, int y, const char* text, uc::Color fg) override;
    void setClip(int x, int y, int w, int h) override;
    void clearClip() override;
    int  charHeight()                            const override;
    int  textWidth(const char* text, int len=-1) const override;

private:
    HDC        m_hdc;
    TEXTMETRIC m_tm;
};
