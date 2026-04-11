#include "win32_window.h"
#include "win32_render_context.h"
#include "test_runner.h"
#include <windowsx.h>
#include <filesystem>

using Hit = uc::BaseWindow::Hit;

static constexpr const char* CLASS_NAME = "UniCommanderWnd";
// --- Lifecycle ---
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

    if (m_hwnd)
        initPanels(std::filesystem::current_path().string());

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
    MSG  msg = {};
    BOOL r;
    // GetMessage returns 0 for WM_QUIT, -1 on error, positive for all others.
    // The naive pattern "while (GetMessage(...))" treats -1 as truthy and then
    // calls DispatchMessage with garbage -- fix by testing != 0 explicitly.
    while (m_running && (r = GetMessage(&msg, nullptr, 0, 0)) != 0)
    {
        if (r == -1) break;    // GetMessage error; bail out cleanly
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Win32Window::close()
{
    if (m_hwnd)
    {
        m_closing = true;
        // SendMessage (not PostMessage) so WM_CLOSE is processed synchronously.
        // By the time close() returns, WM_DESTROY has run and PostQuitMessage(0)
        // has been called, giving the process a clean exit code.
        SendMessage(m_hwnd, WM_CLOSE, 0, 0);
    }
}

std::function<void()> Win32Window::testWakeup()
{
    HWND hwnd = m_hwnd;
    // WM_APP+1 is in the application-defined range (0x8000-0xBFFF).
    // PostMessage is async: the test thread never blocks waiting for the
    // main thread to drain the queue.
    return [hwnd]()
    {
        if (hwnd) PostMessage(hwnd, WM_APP + 1, 0, 0);
    };
}

// --- Rendering ---
void Win32Window::paint(HDC hdc)
{
    RECT cr;
    GetClientRect(m_hwnd, &cr);
    const int W    = cr.right;
    const int H    = cr.bottom;
    const int effH = H - FKEY_H;
    auto [topH, leftW] = computeLayout(W, effH);

    Win32RenderContext ctx(hdc);

    // Dividers
    ctx.fillRect(0, topH, W, DIVIDER_W, {80, 80, 80});       // horizontal
    ctx.fillRect(leftW, 0, DIVIDER_W, topH, {80, 80, 80});   // vertical

    // Panel widgets
    layoutPanels(W, H);
    if (leftWidget())  leftWidget()->paint(ctx);
    if (rightWidget()) rightWidget()->paint(ctx);
    if (termWidget())  termWidget()->paint(ctx);
    if (fkeyWidget())  fkeyWidget()->paint(ctx);

    // Modal widget overlays (drawn last, on top of everything)
    helpWidget().layout(0, 0, W, H);
    helpWidget().paint(ctx);
    confirmWidget().layout(0, 0, W, H);
    confirmWidget().paint(ctx);
}

// --- Window procedure ---
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
        // --- Drawing ---
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (self) self->paint(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;

        case WM_SIZE:
            if (self) self->setSize(LOWORD(lp), HIWORD(lp));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        // --- Test harness command drain ---
        // The test thread calls testWakeup() which posts WM_APP+1 here.
        // drainTestQueue() runs on the main thread, serialised with WM_PAINT.
        case WM_APP + 1:
            if (self) drainTestQueue(self);
            return 0;

        // --- Keyboard ---
        // Note: VK_F10 arrives as WM_SYSKEYDOWN on Win32 (it activates the
        // system menu bar), so it must be handled in both messages.
        case WM_SYSKEYDOWN:
        {
            if (!self) break;
            using Key = uc::BaseWindow::Key;
            if (wp == VK_F10)  { self->handleKeyDown(Key::F10); return 0; }
            if (wp == VK_MENU) { self->setModifierPhysical(Mod::Alt, true);
                                  InvalidateRect(hwnd, nullptr, FALSE); return 0; }
            break;
        }

        case WM_SYSKEYUP:
        {
            if (!self) break;
            if (wp == VK_MENU) { self->setModifierPhysical(Mod::Alt, false);
                                  InvalidateRect(hwnd, nullptr, FALSE); return 0; }
            break;
        }

        case WM_KEYDOWN:
        {
            if (!self) break;
            using Key = uc::BaseWindow::Key;
            switch (wp)
            {
                case VK_F1:      self->handleKeyDown(Key::F1);     return 0;
                case VK_F2:      self->handleKeyDown(Key::F2);     return 0;
                case VK_F3:      self->handleKeyDown(Key::F3);     return 0;
                case VK_F4:      self->handleKeyDown(Key::F4);     return 0;
                case VK_F5:      self->handleKeyDown(Key::F5);     return 0;
                case VK_F6:      self->handleKeyDown(Key::F6);     return 0;
                case VK_F7:      self->handleKeyDown(Key::F7);     return 0;
                case VK_F8:      self->handleKeyDown(Key::F8);     return 0;
                case VK_F9:      self->handleKeyDown(Key::F9);     return 0;
                case VK_UP:      self->handleKeyDown(Key::Up);     return 0;
                case VK_DOWN:    self->handleKeyDown(Key::Down);   return 0;
                case VK_LEFT:    self->handleKeyDown(Key::Left);   return 0;
                case VK_RIGHT:   self->handleKeyDown(Key::Right);  return 0;
                case VK_RETURN:  self->handleKeyDown(Key::Return); return 0;
                case VK_TAB:     self->handleKeyDown(Key::Tab);    return 0;
                case VK_ESCAPE:  self->handleKeyDown(Key::Escape); return 0;
                case VK_SHIFT:   self->setModifierPhysical(Mod::Shift, true);
                                 InvalidateRect(hwnd, nullptr, FALSE); return 0;
                case VK_CONTROL: self->setModifierPhysical(Mod::Ctrl,  true);
                                 InvalidateRect(hwnd, nullptr, FALSE); return 0;
                default: break;
            }
            break;
        }

        case WM_KEYUP:
        {
            if (!self) break;
            switch (wp)
            {
                case VK_SHIFT:   self->setModifierPhysical(Mod::Shift, false);
                                 InvalidateRect(hwnd, nullptr, FALSE); return 0;
                case VK_CONTROL: self->setModifierPhysical(Mod::Ctrl,  false);
                                 InvalidateRect(hwnd, nullptr, FALSE); return 0;
                default: break;
            }
            break;
        }

        // --- Splitter drag + F-key bar clicks ---
        case WM_LBUTTONDOWN:
        {
            if (!self) break;
            RECT cr; GetClientRect(hwnd, &cr);
            const int W  = cr.right, H = cr.bottom;
            const int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);

            // Modal dialog captures all clicks when visible.
            if (self->confirmWidget().isVisible())
            {
                self->confirmWidget().handleMouseDown(mx, my);
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }

            // F-key bar occupies the bottom FKEY_H rows.
            if (my >= H - FKEY_H)
            {
                const int effW  = W - MOD_AREA_W;
                const int cellW = std::max(1, effW / 10);
                if (mx < effW)
                {
                    // Click on an F-key cell: fire the corresponding key.
                    int slot = std::min(mx / cellW, 9);
                    Key key  = static_cast<Key>(static_cast<int>(Key::F1) + slot);
                    self->handleKeyDown(key);
                }
                else
                {
                    // Click on a modifier cell: toggle sticky state.
                    int modIdx = (mx - effW) / MOD_CELL_W;
                    if (modIdx >= 0 && modIdx < 3)
                    {
                        self->toggleModifierSticky(static_cast<Mod>(modIdx));
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                }
                return 0;
            }

            auto hit = self->hitTest(mx, my, W, H - FKEY_H);

            if (hit == Hit::HorizDivider)
            {
                self->m_drag = Drag::Horiz;
                SetCapture(hwnd);
            }
            else if (hit == Hit::VertDivider)
            {
                self->m_drag = Drag::Vert;
                SetCapture(hwnd);
            }
            else if (hit == Hit::LeftPanel || hit == Hit::RightPanel)
            {
                auto* left  = self->leftPanel();
                auto* right = self->rightPanel();
                if (left && right)
                {
                    left->setFocus(hit == Hit::LeftPanel);
                    right->setFocus(hit == Hit::RightPanel);
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            if (!self) break;
            RECT cr; GetClientRect(hwnd, &cr);
            const int W   = cr.right, H   = cr.bottom;
            const int effH = H - FKEY_H;
            const int mx  = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);

            if (self->m_drag == Drag::Horiz && effH > 0)
            {
                self->setHRatio(static_cast<float>(my) / effH);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            else if (self->m_drag == Drag::Vert && W > 0)
            {
                self->setVRatio(static_cast<float>(mx) / W);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONUP:
            if (self) self->m_drag = Drag::Idle;
            ReleaseCapture();
            return 0;

        // --- Cursor ---
        case WM_SETCURSOR:
        {
            if (!self || LOWORD(lp) != HTCLIENT) break;
            POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
            RECT cr; GetClientRect(hwnd, &cr);
            auto hit = self->hitTest(pt.x, pt.y, cr.right, cr.bottom - FKEY_H);
            SetCursor(LoadCursor(nullptr, hit == Hit::HorizDivider ? IDC_SIZENS :
                                          hit == Hit::VertDivider  ? IDC_SIZEWE : IDC_ARROW));
            return TRUE;
        }

        // --- Lifecycle ---
        case WM_CLOSE:
            if (self && (self->m_closing || self->confirmQuit()))
                DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            if (self) { self->m_hwnd = nullptr; self->m_running = false; }
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

void Win32Window::pumpEventsUntil(std::function<bool()> done)
{
    MSG  msg = {};
    BOOL r;
    while (!done() && (r = GetMessage(&msg, nullptr, 0, 0)) != 0)
    {
        if (r == -1) break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

std::unique_ptr<uc::Window> createWindow()
{
    return std::make_unique<Win32Window>();
}
