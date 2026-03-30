#include "win32_window.h"
#include <stdexcept>

static constexpr const char* CLASS_NAME = "UniCommanderWnd";

Win32Window::Win32Window()
    : m_hinstance(GetModuleHandle(nullptr))
{}

Win32Window::~Win32Window()
{
    if (m_hwnd)
        DestroyWindow(m_hwnd);
}

bool Win32Window::create(const std::string& title, int width, int height)
{
    WNDCLASSEX wc      = {};
    wc.cbSize          = sizeof(wc);
    wc.style           = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc     = wndProc;
    wc.hInstance       = m_hinstance;
    wc.hCursor         = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground   = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName   = CLASS_NAME;

    if (!RegisterClassEx(&wc))
        return false;

    m_hwnd = CreateWindowEx(
        0, CLASS_NAME, title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, m_hinstance,
        this   // passed to WM_NCCREATE as CREATESTRUCT::lpCreateParams
    );

    return m_hwnd != nullptr;
}

void Win32Window::show()
{
    if (m_hwnd)
    {
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
    }
}

void Win32Window::run()
{
    m_running = true;
    MSG msg   = {};
    while (m_running && GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Win32Window::close()
{
    m_running = false;
    if (m_hwnd)
        PostMessage(m_hwnd, WM_CLOSE, 0, 0);
}

LRESULT CALLBACK Win32Window::wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    Win32Window* self = nullptr;

    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lp);
        self     = static_cast<Win32Window*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    }
    else
    {
        self = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    switch (msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            if (self) self->m_running = false;
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wp, lp);
    }
}

// Factory
std::unique_ptr<Window> createWindow()
{
    return std::make_unique<Win32Window>();
}
