#pragma once
#include "ui/base_window.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

class Win32Window final : public uc::BaseWindow
{
public:
    Win32Window();
    ~Win32Window() override;

    bool create(const std::string& title, int width, int height) override;
    void show()              override;
    void run()               override;
    void close()             override;
    void invalidate()        override { InvalidateRect(m_hwnd, nullptr, FALSE); }
    bool confirmQuit()       override;
    void scheduleKeyDown(Key key) override;

private:
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    void paint(HDC hdc);
    void renderDirectoryPanel(HDC hdc, RECT rect, uc::DirectoryPanel& panel);

    HWND      m_hwnd      { nullptr };
    HINSTANCE m_hinstance { nullptr };
    bool      m_running   { false };
};
