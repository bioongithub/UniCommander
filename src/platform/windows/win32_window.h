#pragma once
#include "ui/window.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class Win32Window final : public uc::Window
{
public:
    Win32Window();
    ~Win32Window() override;

    bool create(const std::string& title, int width, int height) override;
    void show()  override;
    void run()   override;
    void close() override;

private:
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND      m_hwnd      { nullptr };
    HINSTANCE m_hinstance { nullptr };
    bool      m_running   { false };
};
