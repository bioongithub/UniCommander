#include "win32_window.h"
#include <windowsx.h>

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
    WNDCLASSEX wc    = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = wndProc;
    wc.hInstance     = m_hinstance;
    wc.hCursor       = nullptr;   // managed in WM_SETCURSOR
    wc.hbrBackground = nullptr;   // managed in WM_PAINT
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassEx(&wc))
        return false;

    m_hwnd = CreateWindowEx(
        0, CLASS_NAME, title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, m_hinstance,
        this
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

void Win32Window::paint(HDC hdc)
{
    RECT cr;
    GetClientRect(m_hwnd, &cr);
    const int W     = cr.right;
    const int H     = cr.bottom;
    const int topH  = static_cast<int>(H * m_hRatio);
    const int leftW = static_cast<int>(W * m_vRatio);

    // ── Panel rects ───────────────────────────────────────────────────────
    RECT leftR   = { 0,              0,              leftW,             topH };
    RECT rightR  = { leftW+DIVIDER_W, 0,             W,                 topH };
    RECT bottomR = { 0,              topH+DIVIDER_W, W,                 H    };
    RECT hDivR   = { 0,              topH,           W,                 topH+DIVIDER_W };
    RECT vDivR   = { leftW,          0,              leftW+DIVIDER_W,   topH };

    // ── Fill dividers ─────────────────────────────────────────────────────
    HBRUSH divBrush = CreateSolidBrush(RGB(80, 80, 80));
    FillRect(hdc, &hDivR, divBrush);
    FillRect(hdc, &vDivR, divBrush);
    DeleteObject(divBrush);

    // ── Fill panels ───────────────────────────────────────────────────────
    HBRUSH panelBrush = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(hdc, &leftR,   panelBrush);
    FillRect(hdc, &rightR,  panelBrush);
    FillRect(hdc, &bottomR, panelBrush);
    DeleteObject(panelBrush);

    // ── Draw labels ───────────────────────────────────────────────────────
    SetTextColor(hdc, RGB(220, 220, 220));
    SetBkMode(hdc, TRANSPARENT);
    DrawText(hdc, "left",   -1, &leftR,   DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DrawText(hdc, "right",  -1, &rightR,  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DrawText(hdc, "bottom", -1, &bottomR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
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
        // ── Drawing ───────────────────────────────────────────────────────
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (self) self->paint(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;  // paint() covers every pixel; skip default erase

        case WM_SIZE:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        // ── Splitter drag ─────────────────────────────────────────────────
        case WM_LBUTTONDOWN:
        {
            if (!self) break;
            RECT cr; GetClientRect(hwnd, &cr);
            const int W     = cr.right,  H    = cr.bottom;
            const int topH  = static_cast<int>(H * self->m_hRatio);
            const int leftW = static_cast<int>(W * self->m_vRatio);
            const int mx    = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);

            if (my >= topH - HIT_ZONE && my <= topH + DIVIDER_W + HIT_ZONE)
            {
                self->m_drag = Drag::Horiz;
                SetCapture(hwnd);
            }
            else if (my < topH &&
                     mx >= leftW - HIT_ZONE && mx <= leftW + DIVIDER_W + HIT_ZONE)
            {
                self->m_drag = Drag::Vert;
                SetCapture(hwnd);
            }
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            if (!self) break;
            RECT cr; GetClientRect(hwnd, &cr);
            const int W  = cr.right, H  = cr.bottom;
            const int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);

            if (self->m_drag == Drag::Horiz && H > 0)
            {
                self->m_hRatio = clamp(static_cast<float>(my) / H);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            else if (self->m_drag == Drag::Vert && W > 0)
            {
                self->m_vRatio = clamp(static_cast<float>(mx) / W);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONUP:
            if (self) self->m_drag = Drag::Idle;
            ReleaseCapture();
            return 0;

        // ── Cursor ────────────────────────────────────────────────────────
        case WM_SETCURSOR:
        {
            if (!self || LOWORD(lp) != HTCLIENT) break;
            POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
            RECT cr; GetClientRect(hwnd, &cr);
            const int W     = cr.right,  H    = cr.bottom;
            const int topH  = static_cast<int>(H * self->m_hRatio);
            const int leftW = static_cast<int>(W * self->m_vRatio);

            const bool nearH = (pt.y >= topH  - HIT_ZONE && pt.y <= topH  + DIVIDER_W + HIT_ZONE);
            const bool nearV = (pt.y <  topH  &&
                                pt.x >= leftW - HIT_ZONE && pt.x <= leftW + DIVIDER_W + HIT_ZONE);

            SetCursor(LoadCursor(nullptr, nearH ? IDC_SIZENS :
                                          nearV ? IDC_SIZEWE : IDC_ARROW));
            return TRUE;
        }

        // ── Lifecycle ─────────────────────────────────────────────────────
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            if (self) self->m_running = false;
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

std::unique_ptr<uc::Window> createWindow()
{
    return std::make_unique<Win32Window>();
}
