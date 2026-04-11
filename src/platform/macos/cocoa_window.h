#pragma once
#include "ui/base_window.h"

#ifdef __OBJC__
@class NSWindow;
@class AppDelegate;
@class UCWindowDelegate;
#else
using NSWindow          = struct objc_object;
using AppDelegate       = struct objc_object;
using UCWindowDelegate  = struct objc_object;
#endif

class CocoaWindow final : public uc::BaseWindow
{
public:
    CocoaWindow();
    ~CocoaWindow() override;

    bool create(const std::string& title, int width, int height) override;
    void show()       override;
    void run()        override;
    void close()      override;
    void invalidate() override;
    void pumpEventsUntil(std::function<bool()> done) override;
    std::function<void()> testWakeup()               override;

private:
    NSWindow*         m_window      { nullptr };
    AppDelegate*      m_delegate    { nullptr };
    UCWindowDelegate* m_winDelegate { nullptr };
};
