#include "win32_render_context.h"
#include <cstring>

Win32RenderContext::Win32RenderContext(HDC hdc) : m_hdc(hdc)
{
    GetTextMetrics(m_hdc, &m_tm);
    SetBkMode(m_hdc, TRANSPARENT);
}

void Win32RenderContext::fillRect(int x, int y, int w, int h, uc::Color c)
{
    RECT   r  = { x, y, x + w, y + h };
    HBRUSH br = CreateSolidBrush(RGB(c.r, c.g, c.b));
    FillRect(m_hdc, &r, br);
    DeleteObject(br);
}

void Win32RenderContext::drawRect(int x, int y, int w, int h, uc::Color c)
{
    HPEN   pen    = CreatePen(PS_SOLID, 1, RGB(c.r, c.g, c.b));
    HPEN   oldPen = static_cast<HPEN>(SelectObject(m_hdc, pen));
    HBRUSH oldBr  = static_cast<HBRUSH>(SelectObject(m_hdc, GetStockObject(NULL_BRUSH)));
    Rectangle(m_hdc, x, y, x + w, y + h);
    SelectObject(m_hdc, oldPen);
    SelectObject(m_hdc, oldBr);
    DeleteObject(pen);
}

void Win32RenderContext::drawText(int x, int y, const char* text, uc::Color fg)
{
    SetTextColor(m_hdc, RGB(fg.r, fg.g, fg.b));
    TextOutA(m_hdc, x, y, text, static_cast<int>(strlen(text)));
}

void Win32RenderContext::setClip(int x, int y, int w, int h)
{
    HRGN rgn = CreateRectRgn(x, y, x + w, y + h);
    SelectClipRgn(m_hdc, rgn);
    DeleteObject(rgn);
}

void Win32RenderContext::clearClip()
{
    SelectClipRgn(m_hdc, nullptr);
}

int Win32RenderContext::charHeight() const
{
    return m_tm.tmHeight;
}

int Win32RenderContext::textWidth(const char* text, int len) const
{
    if (len < 0) len = static_cast<int>(strlen(text));
    SIZE sz = {};
    GetTextExtentPoint32A(m_hdc, text, len, &sz);
    return sz.cx;
}
