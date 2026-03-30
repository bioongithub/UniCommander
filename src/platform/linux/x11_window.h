#pragma once
#include "ui/window.h"

// Forward-declare X11 types to keep the header dependency-free.
struct _XDisplay;
using Display = _XDisplay;
using XID     = unsigned long;
using Window  = XID;

class X11Window final : public ::Window
{
public:
    X11Window();
    ~X11Window() override;

    bool create(const std::string& title, int width, int height) override;
    void show()  override;
    void run()   override;
    void close() override;

private:
    Display* m_display { nullptr };
    ::Window m_window  { 0 };
    bool     m_running { false };
};
