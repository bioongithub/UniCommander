#pragma once
#include "ui/window.h"

// Forward-declare opaque Objective-C handle to avoid polluting C++ headers.
#ifdef __OBJC__
@class NSWindow;
@class AppDelegate;
#else
using NSWindow    = struct objc_object;
using AppDelegate = struct objc_object;
#endif

class CocoaWindow final : public Window
{
public:
    CocoaWindow();
    ~CocoaWindow() override;

    bool create(const std::string& title, int width, int height) override;
    void show()  override;
    void run()   override;
    void close() override;

private:
    NSWindow*    m_window   { nullptr };
    AppDelegate* m_delegate { nullptr };
};
