#include "x11_window.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <cstring>
#include <algorithm>
#include <filesystem>

using Hit = uc::BaseWindow::Hit;

static constexpr int ROW_H    = 18;
static constexpr int HEADER_H = 20;

// --- Helpers ---
static unsigned long allocRGB(Display* dpy, int r, int g, int b)
{
    XColor c = {};
    c.red    = static_cast<unsigned short>(r * 257);
    c.green  = static_cast<unsigned short>(g * 257);
    c.blue   = static_cast<unsigned short>(b * 257);
    c.flags  = DoRed | DoGreen | DoBlue;
    XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &c);
    return c.pixel;
}

// --- Lifecycle ---
X11Window::X11Window()  = default;

X11Window::~X11Window()
{
    if (m_display)
    {
        if (m_curArrow) XFreeCursor(m_display, m_curArrow);
        if (m_curNS)    XFreeCursor(m_display, m_curNS);
        if (m_curEW)    XFreeCursor(m_display, m_curEW);
        if (m_fontInfo) XFreeFont(m_display, reinterpret_cast<XFontStruct*>(m_fontInfo));
        if (m_gc)       XFreeGC(m_display, reinterpret_cast<GC>(m_gc));
        if (m_window)   XDestroyWindow(m_display, m_window);
        XCloseDisplay(m_display);
    }
}

bool X11Window::create(const std::string& title, int width, int height)
{
    // Must be called before any other Xlib call so the test background thread
    // can safely call XSendEvent (scheduleKeyDown) while the main thread is in
    // XNextEvent.
    XInitThreads();

    m_display = XOpenDisplay(nullptr);
    if (!m_display) return false;

    setSize(width, height);

    int screen = DefaultScreen(m_display);

    m_window = XCreateSimpleWindow(
        m_display, RootWindow(m_display, screen),
        0, 0,
        static_cast<unsigned>(width),
        static_cast<unsigned>(height),
        0,
        BlackPixel(m_display, screen),
        BlackPixel(m_display, screen)
    );

    XStoreName(m_display, m_window, title.c_str());

    Atom wm_delete = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(m_display, m_window, &wm_delete, 1);

    XSelectInput(m_display, m_window,
                 ExposureMask     | KeyPressMask       |
                 ButtonPressMask  | ButtonReleaseMask  |
                 PointerMotionMask | StructureNotifyMask);

    // --- GC ---
    m_gc = reinterpret_cast<_XGC*>(XCreateGC(m_display, m_window, 0, nullptr));

    // --- Font ---
    XFontStruct* fs = XLoadQueryFont(m_display, "-*-fixed-medium-r-*-*-14-*-*-*-*-*-*-*");
    if (!fs)
        fs = XLoadQueryFont(m_display, "fixed");
    m_fontInfo = reinterpret_cast<_XFontStruct*>(fs);
    if (fs)
        XSetFont(m_display, reinterpret_cast<GC>(m_gc), fs->fid);

    // --- Colors ---
    m_dividerPx    = allocRGB(m_display,  80,  80,  80);
    m_panelBgPx    = allocRGB(m_display,  20,  20,  20);
    m_headerBgPx   = allocRGB(m_display,  40,  40,  60);
    m_headerTextPx = allocRGB(m_display, 180, 180, 255);
    m_borderFocPx  = allocRGB(m_display,   0, 120, 215);
    m_borderUnfPx  = allocRGB(m_display,  80,  80,  80);
    m_selFocPx     = allocRGB(m_display,   0,  80, 160);
    m_selUnfPx     = allocRGB(m_display,  55,  55,  55);
    m_dirTextPx    = allocRGB(m_display, 100, 180, 255);
    m_fileTextPx   = allocRGB(m_display, 220, 220, 220);
    m_selTextPx    = allocRGB(m_display, 255, 255, 255);
    m_bottomBgPx   = allocRGB(m_display,  30,  30,  30);
    m_bottomTextPx = allocRGB(m_display, 220, 220, 220);

    // --- Cursors ---
    m_curArrow = XCreateFontCursor(m_display, XC_left_ptr);
    m_curNS    = XCreateFontCursor(m_display, XC_sb_v_double_arrow);
    m_curEW    = XCreateFontCursor(m_display, XC_sb_h_double_arrow);
    XDefineCursor(m_display, m_window, m_curArrow);

    m_atomKeyDown = XInternAtom(m_display, "UC_KEY_DOWN", False);

    initPanels(std::filesystem::current_path().string());

    return true;
}

void X11Window::show()
{
    if (m_display && m_window)
        XMapWindow(m_display, m_window);
}

// --- Painting ---
void X11Window::renderDirectoryPanel(int rx, int ry, int rw, int rh,
                                     uc::DirectoryPanel& panel)
{
    GC gc = reinterpret_cast<GC>(m_gc);
    auto* fs = reinterpret_cast<XFontStruct*>(m_fontInfo);
    const int fontAscent  = fs ? fs->ascent  : 11;
    const int fontDescent = fs ? fs->descent :  2;

    const int visibleRows = std::max(0, (rh - HEADER_H - 2) / ROW_H);
    panel.ensureVisible(visibleRows);

    // Background
    XSetForeground(m_display, gc, m_panelBgPx);
    XFillRectangle(m_display, m_window, gc,
                   rx, ry, static_cast<unsigned>(rw), static_cast<unsigned>(rh));

    // Border
    XSetForeground(m_display, gc, panel.hasFocus() ? m_borderFocPx : m_borderUnfPx);
    XDrawRectangle(m_display, m_window, gc,
                   rx, ry, static_cast<unsigned>(rw - 1), static_cast<unsigned>(rh - 1));

    // Header background
    XSetForeground(m_display, gc, m_headerBgPx);
    XFillRectangle(m_display, m_window, gc,
                   rx + 1, ry + 1, static_cast<unsigned>(rw - 2), HEADER_H);

    // Header path text
    XSetForeground(m_display, gc, m_headerTextPx);
    std::string path = panel.getPath();
    int headerTextY  = ry + 1 + (HEADER_H + fontAscent) / 2 - fontDescent;
    XDrawString(m_display, m_window, gc,
                rx + 4, headerTextY, path.c_str(), static_cast<int>(path.size()));

    // Entry rows
    const auto& entries    = panel.entries();
    const int   scrollOff  = panel.scrollOffset();
    const int   selectedIdx = panel.selectedIndex();
    int y = ry + 1 + HEADER_H;

    for (int i = scrollOff;
         i < static_cast<int>(entries.size()) && y + ROW_H <= ry + rh - 1;
         ++i, y += ROW_H)
    {
        bool selected = (i == selectedIdx);

        if (selected)
        {
            XSetForeground(m_display, gc,
                           panel.hasFocus() ? m_selFocPx : m_selUnfPx);
            XFillRectangle(m_display, m_window, gc,
                           rx + 1, y, static_cast<unsigned>(rw - 2), ROW_H);
        }

        const auto& entry = entries[i];
        if (selected)
            XSetForeground(m_display, gc, m_selTextPx);
        else if (entry.isDir)
            XSetForeground(m_display, gc, m_dirTextPx);
        else
            XSetForeground(m_display, gc, m_fileTextPx);

        std::string label = (entry.isDir && entry.name != "..")
            ? "[" + entry.name + "]" : entry.name;
        int entryTextY = y + (ROW_H + fontAscent) / 2 - fontDescent;
        XDrawString(m_display, m_window, gc,
                    rx + 4, entryTextY,
                    label.c_str(), static_cast<int>(label.size()));
    }
}

void X11Window::paint()
{
    GC gc = reinterpret_cast<GC>(m_gc);
    auto* fs = reinterpret_cast<XFontStruct*>(m_fontInfo);

    const int W = m_width;
    const int H = m_height;
    auto [topH, leftW] = computeLayout(W, H);

    // Divider background (fills the gaps between panels)
    XSetForeground(m_display, gc, m_dividerPx);
    XFillRectangle(m_display, m_window, gc,
                   0, 0, static_cast<unsigned>(W), static_cast<unsigned>(H));

    // Directory panels
    if (m_leftPanel)
        renderDirectoryPanel(0, 0, leftW, topH, *m_leftPanel);
    if (m_rightPanel)
        renderDirectoryPanel(leftW + DIVIDER_W, 0,
                             W - leftW - DIVIDER_W, topH, *m_rightPanel);

    // Bottom panel
    XSetForeground(m_display, gc, m_bottomBgPx);
    XFillRectangle(m_display, m_window, gc,
                   0, topH + DIVIDER_W,
                   static_cast<unsigned>(W),
                   static_cast<unsigned>(H - topH - DIVIDER_W));

    const char* bottomLabel = "Terminal";
    int len = static_cast<int>(strlen(bottomLabel));
    int tw  = fs ? XTextWidth(fs, bottomLabel, len) : len * 7;
    int th  = fs ? (fs->ascent + fs->descent) : 13;
    int tx  = (W - tw) / 2;
    int ty  = topH + DIVIDER_W + (H - topH - DIVIDER_W + th) / 2
              - (fs ? fs->descent : 2);
    XSetForeground(m_display, gc, m_bottomTextPx);
    XDrawString(m_display, m_window, gc, tx, ty, bottomLabel, len);

    XFlush(m_display);
}

// --- Event loop ---
void X11Window::run()
{
    if (!m_display) return;

    Atom wm_delete = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    m_running = true;

    XEvent event = {};
    while (m_running)
    {
        XNextEvent(m_display, &event);
        switch (event.type)
        {
            case Expose:
                if (event.xexpose.count == 0)
                    paint();
                break;

            case ConfigureNotify:
                setSize(event.xconfigure.width, event.xconfigure.height);
                paint();
                break;

            case ButtonPress:
            {
                if (event.xbutton.button != Button1) break;
                const int mx = event.xbutton.x, my = event.xbutton.y;
                auto hit = hitTest(mx, my, m_width, m_height);

                if (hit == Hit::HorizDivider)
                    m_drag = Drag::Horiz;
                else if (hit == Hit::VertDivider)
                    m_drag = Drag::Vert;
                else if (hit == Hit::LeftPanel || hit == Hit::RightPanel)
                {
                    auto* left  = leftPanel();
                    auto* right = rightPanel();
                    if (left && right)
                    {
                        left->setFocus(hit == Hit::LeftPanel);
                        right->setFocus(hit == Hit::RightPanel);
                        paint();
                    }
                }
                break;
            }

            case MotionNotify:
            {
                const int mx = event.xmotion.x, my = event.xmotion.y;

                if (m_drag == Drag::Horiz && m_height > 0)
                {
                    setHRatio(static_cast<float>(my) / m_height);
                    paint();
                }
                else if (m_drag == Drag::Vert && m_width > 0)
                {
                    setVRatio(static_cast<float>(mx) / m_width);
                    paint();
                }
                else
                {
                    auto hit = hitTest(mx, my, m_width, m_height);
                    unsigned long cur = hit == Hit::HorizDivider ? m_curNS :
                                        hit == Hit::VertDivider  ? m_curEW : m_curArrow;
                    XDefineCursor(m_display, m_window, cur);
                    XFlush(m_display);
                }
                break;
            }

            case ButtonRelease:
                m_drag = Drag::Idle;
                break;

            case KeyPress:
            {
                using Key = uc::BaseWindow::Key;
                KeySym ks = XLookupKeysym(&event.xkey, 0);
                switch (ks)
                {
                    case XK_Up:       handleKeyDown(Key::Up);     break;
                    case XK_Down:     handleKeyDown(Key::Down);   break;
                    case XK_Return:   handleKeyDown(Key::Return); break;
                    case XK_Tab:      handleKeyDown(Key::Tab);    break;
                    case XK_Escape:   handleKeyDown(Key::Escape); break;
                    case XK_q:        handleKeyDown(Key::Q);      m_running = false; break;
                    case XK_F10:      handleKeyDown(Key::F10);    break;
                    default: break;
                }
                break;
            }

            case ClientMessage:
                if (static_cast<Atom>(event.xclient.message_type) == m_atomKeyDown)
                {
                    // Dispatched from scheduleKeyDown() on the test thread.
                    // Runs here on the main thread, serialised with rendering.
                    handleKeyDown(static_cast<Key>(event.xclient.data.l[0]));
                }
                else if (static_cast<Atom>(event.xclient.data.l[0]) == wm_delete)
                {
                    if (m_closing || confirmQuit())
                        m_running = false;
                    else
                        paint();    // restore window after cancelled dialog
                }
                break;

            default:
                break;
        }
    }
}

void X11Window::invalidate()
{
    paint();
}

void X11Window::close()
{
    m_closing = true;
    if (!m_display || !m_window) { m_running = false; return; }
    // Post WM_DELETE_WINDOW to ourselves to unblock XNextEvent
    Atom wm_protocols = XInternAtom(m_display, "WM_PROTOCOLS",    False);
    Atom wm_delete    = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    XClientMessageEvent ev = {};
    ev.type         = ClientMessage;
    ev.window       = m_window;
    ev.message_type = wm_protocols;
    ev.format       = 32;
    ev.data.l[0]    = static_cast<long>(wm_delete);
    XSendEvent(m_display, m_window, False, NoEventMask,
               reinterpret_cast<XEvent*>(&ev));
    XFlush(m_display);
}

void X11Window::scheduleKeyDown(Key key)
{
    // Post a UC_KEY_DOWN client message so handleKeyDown() runs on the main
    // thread (serialised with XNextEvent / paint), not on the test thread.
    if (!m_display || !m_window) return;
    XClientMessageEvent ev = {};
    ev.type         = ClientMessage;
    ev.window       = m_window;
    ev.message_type = m_atomKeyDown;
    ev.format       = 32;
    ev.data.l[0]    = static_cast<long>(key);
    XSendEvent(m_display, m_window, False, NoEventMask,
               reinterpret_cast<XEvent*>(&ev));
    XFlush(m_display);
}

bool X11Window::confirmQuit()
{
    if (m_testDialogAnswer.has_value())
    {
        bool ans = *m_testDialogAnswer;
        m_testDialogAnswer.reset();
        return ans;
    }

    GC    gc = reinterpret_cast<GC>(m_gc);
    auto* fs = reinterpret_cast<XFontStruct*>(m_fontInfo);

    const int dlgW = 300, dlgH = 110;
    const int btnW = 80,  btnH = 28;

    // Dialog and button positions — recalculated on resize
    int dlgX = (m_width  - dlgW) / 2;
    int dlgY = (m_height - dlgH) / 2;
    int btnY = dlgY + dlgH - btnH - 12;
    int yesX = dlgX + dlgW / 2 - btnW - 10;
    int noX  = dlgX + dlgW / 2 + 10;

    // Local colors
    unsigned long dlgBgPx  = allocRGB(m_display, 40,  40,  55);
    unsigned long btnBgPx  = allocRGB(m_display, 55,  55,  75);

    // focused: 0 = Yes, 1 = No  (default No — safer)
    int focused = 1;

    auto drawDialog = [&]()
    {
        // Dialog border + background
        XSetForeground(m_display, gc, m_borderFocPx);
        XDrawRectangle(m_display, m_window, gc,
                       dlgX - 1, dlgY - 1,
                       static_cast<unsigned>(dlgW + 1),
                       static_cast<unsigned>(dlgH + 1));
        XSetForeground(m_display, gc, dlgBgPx);
        XFillRectangle(m_display, m_window, gc,
                       dlgX, dlgY,
                       static_cast<unsigned>(dlgW),
                       static_cast<unsigned>(dlgH));

        // Title text
        const char* msg  = "Quit UniCommander?";
        int         mlen = static_cast<int>(strlen(msg));
        int         mw   = fs ? XTextWidth(fs, msg, mlen) : mlen * 7;
        int         my   = dlgY + (dlgH - btnH - 20) / 2
                           + (fs ? fs->ascent : 11);
        XSetForeground(m_display, gc, m_selTextPx);
        XDrawString(m_display, m_window, gc,
                    dlgX + (dlgW - mw) / 2, my, msg, mlen);

        // Draw one button: helper lambda
        auto drawBtn = [&](int bx, int by, const char* label, bool hot)
        {
            XSetForeground(m_display, gc, hot ? m_selFocPx : btnBgPx);
            XFillRectangle(m_display, m_window, gc,
                           bx, by,
                           static_cast<unsigned>(btnW),
                           static_cast<unsigned>(btnH));
            XSetForeground(m_display, gc, m_borderFocPx);
            XDrawRectangle(m_display, m_window, gc,
                           bx, by,
                           static_cast<unsigned>(btnW),
                           static_cast<unsigned>(btnH));
            int llen = static_cast<int>(strlen(label));
            int lw   = fs ? XTextWidth(fs, label, llen) : llen * 7;
            int ly   = by + (btnH + (fs ? fs->ascent : 11)) / 2
                       - (fs ? fs->descent : 2);
            XSetForeground(m_display, gc, m_selTextPx);
            XDrawString(m_display, m_window, gc,
                        bx + (btnW - lw) / 2, ly, label, llen);
        };

        drawBtn(yesX, btnY, "Yes", focused == 0);
        drawBtn(noX,  btnY, "No",  focused == 1);
        XFlush(m_display);
    };

    drawDialog();

    // Mini event loop — runs until user confirms or cancels
    XEvent ev = {};
    while (true)
    {
        XNextEvent(m_display, &ev);

        if (ev.type == Expose && ev.xexpose.count == 0)
        {
            paint();
            drawDialog();
        }
        else if (ev.type == ConfigureNotify)
        {
            setSize(ev.xconfigure.width, ev.xconfigure.height);
            dlgX = (m_width  - dlgW) / 2;
            dlgY = (m_height - dlgH) / 2;
            btnY = dlgY + dlgH - btnH - 12;
            yesX = dlgX + dlgW / 2 - btnW - 10;
            noX  = dlgX + dlgW / 2 + 10;
            paint();
            drawDialog();
        }
        else if (ev.type == KeyPress)
        {
            KeySym ks = XLookupKeysym(&ev.xkey, 0);
            if (ks == XK_Return)
            {
                if (focused != 0) paint();   // cancel: restore window
                return focused == 0;
            }
            if (ks == XK_Escape || ks == XK_n || ks == XK_N)
            {
                paint();
                return false;
            }
            if (ks == XK_y || ks == XK_Y)
                return true;
            if (ks == XK_Tab || ks == XK_Left || ks == XK_Right)
            {
                focused = 1 - focused;
                drawDialog();
            }
        }
        else if (ev.type == ButtonPress && ev.xbutton.button == Button1)
        {
            int mx = ev.xbutton.x, my = ev.xbutton.y;
            bool onYes = (mx >= yesX && mx < yesX + btnW &&
                          my >= btnY && my < btnY + btnH);
            bool onNo  = (mx >= noX  && mx < noX  + btnW &&
                          my >= btnY && my < btnY + btnH);
            if (onYes) return true;
            if (onNo)  { paint(); return false; }
        }
    }
}

std::unique_ptr<uc::Window> createWindow()
{
    return std::make_unique<X11Window>();
}
