#pragma once
#include "ui/base_window.h"

// Forward-declare X11 internals without pulling in <X11/Xlib.h>,
// which defines a conflicting global 'Window' typedef.
struct _XDisplay;
struct _XGC;
struct _XFontStruct;

class X11Window final : public uc::BaseWindow
{
public:
    X11Window();
    ~X11Window() override;

    bool create(const std::string& title, int width, int height) override;
    void show()       override;
    void run()        override;
    void close()      override;
    void invalidate() override;

private:
    void paint();
    void renderDirectoryPanel(int rx, int ry, int rw, int rh,
                              uc::DirectoryPanel& panel);

    _XDisplay*    m_display  { nullptr };
    unsigned long m_window   { 0 };       // X11 Window  = unsigned long
    _XGC*         m_gc       { nullptr }; // X11 GC      = _XGC*
    _XFontStruct* m_fontInfo { nullptr };
    bool          m_running  { false };

    // Pixel values for drawing (allocated at create time)
    unsigned long m_dividerPx    { 0 };
    unsigned long m_panelBgPx    { 0 };
    unsigned long m_headerBgPx   { 0 };
    unsigned long m_headerTextPx { 0 };
    unsigned long m_borderFocPx  { 0 };
    unsigned long m_borderUnfPx  { 0 };
    unsigned long m_selFocPx     { 0 };
    unsigned long m_selUnfPx     { 0 };
    unsigned long m_dirTextPx    { 0 };
    unsigned long m_fileTextPx   { 0 };
    unsigned long m_selTextPx    { 0 };
    unsigned long m_bottomBgPx   { 0 };
    unsigned long m_bottomTextPx { 0 };

    // Cursors (X11 Cursor = unsigned long)
    unsigned long m_curArrow { 0 };
    unsigned long m_curNS    { 0 };
    unsigned long m_curEW    { 0 };
};
