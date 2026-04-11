#include "x11_window.h"
#include "x11_render_context.h"
#include "test_runner.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <cstring>
#include <algorithm>
#include <filesystem>

using Hit = uc::BaseWindow::Hit;

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
                 ExposureMask      | KeyPressMask        | KeyReleaseMask |
                 ButtonPressMask   | ButtonReleaseMask   |
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

    // --- Cursors ---
    m_curArrow = XCreateFontCursor(m_display, XC_left_ptr);
    m_curNS    = XCreateFontCursor(m_display, XC_sb_v_double_arrow);
    m_curEW    = XCreateFontCursor(m_display, XC_sb_h_double_arrow);
    XDefineCursor(m_display, m_window, m_curArrow);

    m_atomDrainQueue  = XInternAtom(m_display, "UC_DRAIN_QUEUE",   False);
    m_atomWmDelete    = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    m_atomWmProtocols = XInternAtom(m_display, "WM_PROTOCOLS",     False);

    initPanels(std::filesystem::current_path().string());

    return true;
}

void X11Window::show()
{
    if (m_display && m_window)
        XMapWindow(m_display, m_window);
}

// --- Painting ---
void X11Window::paint()
{
    auto* fs = reinterpret_cast<XFontStruct*>(m_fontInfo);
    X11RenderContext ctx(m_display, m_window,
                         reinterpret_cast<GC>(m_gc), fs);

    const int W    = m_width;
    const int H    = m_height;
    const int effH = H - FKEY_H;
    auto [topH, leftW] = computeLayout(W, effH);

    // Fill divider color for the top area; panels overdraw their own regions
    ctx.fillRect(0, 0, W, effH, {80, 80, 80});

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

    XFlush(m_display);
}


// --- Event processing ---
void X11Window::processEvent(XEvent& event)
{
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

            // Modal dialog captures all clicks when visible.
            if (confirmWidget().isVisible())
            {
                confirmWidget().handleMouseDown(mx, my);
                break;
            }

            // F-key bar occupies the bottom FKEY_H rows.
            if (my >= m_height - FKEY_H)
            {
                const int effW  = m_width - MOD_AREA_W;
                const int cellW = std::max(1, effW / 10);
                if (mx < effW)
                {
                    int slot = std::min(mx / cellW, 9);
                    Key key  = static_cast<Key>(static_cast<int>(Key::F1) + slot);
                    handleKeyDown(key);
                }
                else
                {
                    int modIdx = (mx - effW) / MOD_CELL_W;
                    if (modIdx >= 0 && modIdx < 3)
                    {
                        toggleModifierSticky(static_cast<Mod>(modIdx));
                        paint();
                    }
                }
                break;
            }

            auto hit = hitTest(mx, my, m_width, m_height - FKEY_H);
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
            const int mx    = event.xmotion.x, my = event.xmotion.y;
            const int effH  = m_height - FKEY_H;

            if (m_drag == Drag::Horiz && effH > 0)
            {
                setHRatio(static_cast<float>(my) / effH);
                paint();
            }
            else if (m_drag == Drag::Vert && m_width > 0)
            {
                setVRatio(static_cast<float>(mx) / m_width);
                paint();
            }
            else
            {
                auto hit = hitTest(mx, my, m_width, effH);
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
                case XK_F1:        handleKeyDown(Key::F1);     break;
                case XK_F2:        handleKeyDown(Key::F2);     break;
                case XK_F3:        handleKeyDown(Key::F3);     break;
                case XK_F4:        handleKeyDown(Key::F4);     break;
                case XK_F5:        handleKeyDown(Key::F5);     break;
                case XK_F6:        handleKeyDown(Key::F6);     break;
                case XK_F7:        handleKeyDown(Key::F7);     break;
                case XK_F8:        handleKeyDown(Key::F8);     break;
                case XK_F9:        handleKeyDown(Key::F9);     break;
                case XK_Up:        handleKeyDown(Key::Up);     break;
                case XK_Down:      handleKeyDown(Key::Down);   break;
                case XK_Left:      handleKeyDown(Key::Left);   break;
                case XK_Right:     handleKeyDown(Key::Right);  break;
                case XK_Return:    handleKeyDown(Key::Return); break;
                case XK_Tab:       handleKeyDown(Key::Tab);    break;
                case XK_Escape:    handleKeyDown(Key::Escape); break;
                case XK_q:         handleKeyDown(Key::Q);      m_running = false; break;
                case XK_F10:       handleKeyDown(Key::F10);    break;
                case XK_Shift_L:
                case XK_Shift_R:   setModifierPhysical(Mod::Shift, true);  paint(); break;
                case XK_Control_L:
                case XK_Control_R: setModifierPhysical(Mod::Ctrl,  true);  paint(); break;
                case XK_Alt_L:
                case XK_Alt_R:     setModifierPhysical(Mod::Alt,   true);  paint(); break;
                default: break;
            }
            break;
        }

        case KeyRelease:
        {
            KeySym ks = XLookupKeysym(&event.xkey, 0);
            switch (ks)
            {
                case XK_Shift_L:
                case XK_Shift_R:   setModifierPhysical(Mod::Shift, false); paint(); break;
                case XK_Control_L:
                case XK_Control_R: setModifierPhysical(Mod::Ctrl,  false); paint(); break;
                case XK_Alt_L:
                case XK_Alt_R:     setModifierPhysical(Mod::Alt,   false); paint(); break;
                default: break;
            }
            break;
        }

        case ClientMessage:
            if (static_cast<Atom>(event.xclient.message_type) == m_atomDrainQueue)
            {
                // Posted from testWakeup() on the test thread.
                // Runs here on the main thread, serialised with rendering.
                drainTestQueue(this);
            }
            else if (static_cast<Atom>(event.xclient.data.l[0]) == m_atomWmDelete)
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

// --- Event loop ---
void X11Window::run()
{
    if (!m_display) return;
    m_running = true;
    XEvent event = {};
    while (m_running)
    {
        XNextEvent(m_display, &event);
        processEvent(event);
    }
}

void X11Window::pumpEventsUntil(std::function<bool()> done)
{
    if (!m_display) return;
    XEvent event = {};
    while (!done())
    {
        XNextEvent(m_display, &event);
        processEvent(event);
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
    Atom wm_protocols = static_cast<Atom>(m_atomWmProtocols);
    Atom wm_delete    = static_cast<Atom>(m_atomWmDelete);
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

std::function<void()> X11Window::testWakeup()
{
    // Returns a callable safe to invoke from the test thread.
    // Posting UC_DRAIN_QUEUE unblocks XNextEvent so drainTestQueue() runs
    // on the main thread, serialised with rendering.
    // XInitThreads() was already called in create(), so XSendEvent from
    // another thread is safe.
    auto* display = m_display;
    auto  window  = m_window;
    auto  atom    = m_atomDrainQueue;
    return [display, window, atom]()
    {
        if (!display || !window) return;
        XClientMessageEvent ev = {};
        ev.type         = ClientMessage;
        ev.window       = window;
        ev.message_type = atom;
        ev.format       = 32;
        XSendEvent(display, window, False, NoEventMask,
                   reinterpret_cast<XEvent*>(&ev));
        XFlush(display);
    };
}

std::unique_ptr<uc::Window> createWindow()
{
    return std::make_unique<X11Window>();
}
