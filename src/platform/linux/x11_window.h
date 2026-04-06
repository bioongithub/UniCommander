#pragma once
#include "ui/base_window.h"
#include <condition_variable>
#include <mutex>

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
    bool confirmQuit()           override;
    void scheduleKeyDown(Key key) override;

private:
    void paint();
    void renderDirectoryPanel(int rx, int ry, int rw, int rh,
                              uc::DirectoryPanel& panel);
    void renderFKeyBar();

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

    // F-key bar colors
    unsigned long m_fkeyNumBgPx  { 0 };
    unsigned long m_fkeyNumFgPx  { 0 };
    unsigned long m_fkeyLblBgPx  { 0 };
    unsigned long m_fkeyLblFgPx  { 0 };
    unsigned long m_modInaBgPx   { 0 };
    unsigned long m_modActBgPx   { 0 };
    unsigned long m_modActFgPx   { 0 };
    unsigned long m_modInaFgPx   { 0 };

    // Cursors (X11 Cursor = unsigned long)
    unsigned long m_curArrow    { 0 };
    unsigned long m_curNS       { 0 };
    unsigned long m_curEW       { 0 };

    // Atom for test-thread key dispatch (client message UC_KEY_DOWN)
    unsigned long m_atomKeyDown { 0 };

    // Synchronise scheduleKeyDown(): test thread blocks until the main thread
    // finishes handling the key (mirrors Win32 SendMessage synchronous semantics).
    std::mutex              m_keyMutex;
    std::condition_variable m_keyCv;
    bool                    m_keyProcessed { false };
};
