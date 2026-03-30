#pragma once
#include "ui/window.h"

// Forward-declare Display without pulling in <X11/Xlib.h> (which defines
// a conflicting 'Window' typedef in the global namespace).
struct _XDisplay;

class X11Window final : public uc::Window
{
public:
    X11Window();
    ~X11Window() override;

    bool create(const std::string& title, int width, int height) override;
    void show()  override;
    void run()   override;
    void close() override;

private:
    _XDisplay*    m_display { nullptr };
    unsigned long m_window  { 0 };      // X11 Window is typedef'd to unsigned long
    bool          m_running { false };
};
