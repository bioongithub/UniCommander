#include "win32_window.h"
#include <windowsx.h>
#include <filesystem>

using Hit = uc::BaseWindow::Hit;

static constexpr const char* CLASS_NAME = "UniCommanderWnd";
static constexpr int ROW_H    = 18;
static constexpr int HEADER_H = 20;

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

void Win32Window::scheduleKeyDown(Key key)
{
    // Deliver the key on the main (UI) thread so that handleKeyDown() is
    // serialised with WM_PAINT.  This prevents data races between the test
    // thread (which calls scheduleKeyDown) and the render thread that reads
    // the same DirectoryPanel state (entries, selection, scroll offset).
    //
    // WM_APP+1 is in the application-defined range (0x8000-0xBFFF).
    if (m_hwnd)
        SendMessage(m_hwnd, WM_APP + 1, static_cast<WPARAM>(key), 0);
}

bool Win32Window::confirmQuit()
{
    if (m_testDialogAnswer.has_value())
    {
        bool ans = *m_testDialogAnswer;
        m_testDialogAnswer.reset();
        return ans;
    }
    int r = MessageBoxA(m_hwnd,
                        "Are you sure you want to quit?",
                        "UniCommander",
                        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
    return r == IDYES;
}

// --- Rendering ---
void Win32Window::renderDirectoryPanel(HDC hdc, RECT rect, uc::DirectoryPanel& panel)
{
    const int panelH = rect.bottom - rect.top;

    // Ensure selected entry is scrolled into view
    const int visibleRows = std::max(0, (panelH - HEADER_H - 2) / ROW_H);
    panel.ensureVisible(visibleRows);

    // Background
    HBRUSH bgBrush = CreateSolidBrush(RGB(20, 20, 20));
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);

    // Border (blue when focused, gray otherwise)
    HPEN borderPen = CreatePen(PS_SOLID, 1,
                               panel.hasFocus() ? RGB(0, 120, 215) : RGB(80, 80, 80));
    HPEN  oldPen   = (HPEN) SelectObject(hdc, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);

    // Header — shows current path
    RECT headerR = { rect.left + 1, rect.top + 1,
                     rect.right - 1, rect.top + 1 + HEADER_H };
    HBRUSH headerBrush = CreateSolidBrush(RGB(40, 40, 60));
    FillRect(hdc, &headerR, headerBrush);
    DeleteObject(headerBrush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(180, 180, 255));
    RECT pathTextR = { headerR.left + 4, headerR.top,
                       headerR.right - 4, headerR.bottom };
    std::string pathStr = panel.getPath();
    DrawTextA(hdc, pathStr.c_str(), -1, &pathTextR,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Entry rows
    const auto& entries     = panel.entries();
    const int   scrollOff   = panel.scrollOffset();
    const int   selectedIdx = panel.selectedIndex();
    int y = rect.top + 1 + HEADER_H;

    for (int i = scrollOff;
         i < (int)entries.size() && y + ROW_H <= rect.bottom - 1;
         ++i, y += ROW_H)
    {
        bool selected = (i == selectedIdx);
        RECT rowR = { rect.left + 1, y, rect.right - 1, y + ROW_H };

        if (selected)
        {
            HBRUSH selBrush = CreateSolidBrush(
                panel.hasFocus() ? RGB(0, 80, 160) : RGB(55, 55, 55));
            FillRect(hdc, &rowR, selBrush);
            DeleteObject(selBrush);
        }

        const auto& entry = entries[i];
        COLORREF textColor =
            selected          ? RGB(255, 255, 255) :
            entry.isDir       ? RGB(100, 180, 255) :
                                RGB(220, 220, 220);
        SetTextColor(hdc, textColor);

        std::string label =
            (entry.isDir && entry.name != "..") ? "[" + entry.name + "]"
                                                 : entry.name;
        RECT textR = { rowR.left + 4, rowR.top, rowR.right - 4, rowR.bottom };
        DrawTextA(hdc, label.c_str(), -1, &textR,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
}

void Win32Window::paint(HDC hdc)
{
    RECT cr;
    GetClientRect(m_hwnd, &cr);
    const int W        = cr.right;
    const int H        = cr.bottom;
    const int effH     = H - FKEY_H;             // height available above the F-key bar
    auto [topH, leftW] = computeLayout(W, effH);

    // --- Panel rects ---
    RECT leftR   = { 0,               0,              leftW,              topH };
    RECT rightR  = { leftW+DIVIDER_W, 0,              W,                  topH };
    RECT bottomR = { 0,               topH+DIVIDER_W, W,                  effH };
    RECT hDivR   = { 0,               topH,           W,                  topH+DIVIDER_W };
    RECT vDivR   = { leftW,           0,              leftW+DIVIDER_W,    topH };

    // --- Dividers ---
    HBRUSH divBrush = CreateSolidBrush(RGB(80, 80, 80));
    FillRect(hdc, &hDivR, divBrush);
    FillRect(hdc, &vDivR, divBrush);
    DeleteObject(divBrush);

    // --- Directory panels ---
    if (m_leftPanel)  renderDirectoryPanel(hdc, leftR,  *m_leftPanel);
    if (m_rightPanel) renderDirectoryPanel(hdc, rightR, *m_rightPanel);

    // --- Bottom panel (terminal placeholder) ---
    HBRUSH panelBrush = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(hdc, &bottomR, panelBrush);
    DeleteObject(panelBrush);
    SetTextColor(hdc, RGB(220, 220, 220));
    SetBkMode(hdc, TRANSPARENT);
    DrawText(hdc, "Terminal", -1, &bottomR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // --- F-key bar ---
    renderFKeyBar(hdc, W, H);

    // --- Help overlay (drawn last, on top of everything) ---
    if (helpWindow().isVisible()) renderHelpWindow(hdc, W, H);
}

void Win32Window::renderFKeyBar(HDC hdc, int W, int H)
{
    const int barY   = H - FKEY_H;
    const int effW   = W - MOD_AREA_W;
    const int cellW  = std::max(1, effW / 10);
    const int numW   = 16;   // sub-column for the key number

    SetBkMode(hdc, TRANSPARENT);

    // --- F-key cells ---
    for (int i = 0; i < 10; ++i)
    {
        int x = i * cellW;

        // Number sub-column (black bg, yellow text)
        RECT numR = { x, barY, x + numW, H };
        HBRUSH numBr = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &numR, numBr);
        DeleteObject(numBr);
        SetTextColor(hdc, RGB(255, 255, 85));
        std::string numStr = std::to_string(i + 1);
        DrawTextA(hdc, numStr.c_str(), -1, &numR,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Label sub-column
        RECT lblR = { x + numW, barY, x + cellW, H };
        const char* lbl = FKEY_LABELS[activeModifierRow()][i];
        if (lbl[0] != '\0')
        {
            // Implemented: cyan bg, black text
            HBRUSH lblBr = CreateSolidBrush(RGB(0, 170, 170));
            FillRect(hdc, &lblR, lblBr);
            DeleteObject(lblBr);
            SetTextColor(hdc, RGB(0, 0, 0));
            RECT padR = { lblR.left + 2, lblR.top, lblR.right, lblR.bottom };
            DrawTextA(hdc, lbl, -1, &padR,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
        else
        {
            HBRUSH lblBr = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &lblR, lblBr);
            DeleteObject(lblBr);
        }
    }

    // --- Modifier cells (Alt / Sft / Ctl) ---
    static const char* MOD_LABELS[3] = { "Alt", "Sft", "Ctl" };
    for (int i = 0; i < 3; ++i)
    {
        int x = effW + i * MOD_CELL_W;
        bool active = modifierActive(static_cast<Mod>(i));
        RECT modR = { x, barY, x + MOD_CELL_W, H };
        HBRUSH modBr = CreateSolidBrush(active ? RGB(170, 170, 0) : RGB(0, 0, 100));
        FillRect(hdc, &modR, modBr);
        DeleteObject(modBr);
        SetTextColor(hdc, active ? RGB(0, 0, 0) : RGB(200, 200, 200));
        DrawTextA(hdc, MOD_LABELS[i], -1, &modR,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void Win32Window::renderHelpWindow(HDC hdc, int W, int H)
{
    auto box = uc::HelpWindow::computeBox(W, H);

    // Background
    RECT boxR = { box.x, box.y, box.x + box.w, box.y + box.h };
    HBRUSH bgBrush = CreateSolidBrush(RGB(20, 20, 50));
    FillRect(hdc, &boxR, bgBrush);
    DeleteObject(bgBrush);

    // Border
    HPEN   borderPen  = CreatePen(PS_SOLID, 2, RGB(100, 180, 255));
    HPEN   oldPen     = (HPEN)  SelectObject(hdc, borderPen);
    HBRUSH oldBrush   = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, box.x, box.y, box.x + box.w, box.y + box.h);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);

    // Text lines
    SetBkMode(hdc, TRANSPARENT);
    int y = box.y + uc::HelpWindow::PADDING;
    for (int i = 0; uc::HelpWindow::LINES[i]; ++i)
    {
        const char* line = uc::HelpWindow::LINES[i];
        // Title row (index 0) in blue; all others in light gray
        SetTextColor(hdc, i == 0 ? RGB(100, 180, 255) : RGB(220, 220, 220));
        RECT textR = { box.x + uc::HelpWindow::PADDING, y,
                       box.x + box.w - uc::HelpWindow::PADDING,
                       y + uc::HelpWindow::LINE_H };
        DrawTextA(hdc, line, -1, &textR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        y += uc::HelpWindow::LINE_H;
    }
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

        // --- Test harness key dispatch ---
        // The test thread calls scheduleKeyDown() which uses SendMessage to
        // deliver WM_APP+1 here, ensuring handleKeyDown() runs on the main
        // thread and is serialised with WM_PAINT (no panel data race).
        case WM_APP + 1:
            if (self) self->handleKeyDown(static_cast<Key>(wp));
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
                case VK_UP:      self->handleKeyDown(Key::Up);     return 0;
                case VK_DOWN:    self->handleKeyDown(Key::Down);   return 0;
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
                    self->scheduleKeyDown(key);
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

std::unique_ptr<uc::Window> createWindow()
{
    return std::make_unique<Win32Window>();
}
