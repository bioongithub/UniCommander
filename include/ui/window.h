#pragma once
#include <memory>
#include <string>

// Abstract base class for a top-level application window.
class Window
{
public:
    virtual ~Window() = default;

    virtual bool create(const std::string& title, int width, int height) = 0;
    virtual void show() = 0;
    virtual void run()  = 0;   // enters the platform event loop (blocking)
    virtual void close() = 0;
};

// Factory — implemented in each platform source file.
std::unique_ptr<Window> createWindow();
