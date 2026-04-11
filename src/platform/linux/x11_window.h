#pragma once
#include "ui/base_window.h"
#include <functional>

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
    void show()             override;
    void run()              override;
    void close()            override;
    void invalidate()       override;
    void pumpEventsUntil(std::function<bool()> done)            override;
    std::function<void()> testWakeup()                          override;

private:
    void paint();
    void processEvent(XEvent& event);   // shared by run() and pumpEventsUntil()

    _XDisplay*    m_display  { nullptr };
    unsigned long m_window   { 0 };       // X11 Window  = unsigned long
    _XGC*         m_gc       { nullptr }; // X11 GC      = _XGC*
    _XFontStruct* m_fontInfo { nullptr };
    bool          m_running  { false };

    // Cursors (X11 Cursor = unsigned long)
    unsigned long m_curArrow    { 0 };
    unsigned long m_curNS       { 0 };
    unsigned long m_curEW       { 0 };

    // Atoms
    unsigned long m_atomDrainQueue { 0 };  // UC_DRAIN_QUEUE (test-thread wakeup)
    unsigned long m_atomWmDelete   { 0 };  // WM_DELETE_WINDOW
    unsigned long m_atomWmProtocols{ 0 };  // WM_PROTOCOLS
};
