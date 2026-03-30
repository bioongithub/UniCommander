#include "x11_window.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <cstring>
#include <algorithm>

// ── Helpers ───────────────────────────────────────────────────────────────

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

// ── Lifecycle ─────────────────────────────────────────────────────────────

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
    m_display = XOpenDisplay(nullptr);
    if (!m_display) return false;

    m_width  = width;
    m_height = height;

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

    // ── GC ────────────────────────────────────────────────────────────────
    m_gc = reinterpret_cast<_XGC*>(XCreateGC(m_display, m_window, 0, nullptr));

    // ── Font ──────────────────────────────────────────────────────────────
    XFontStruct* fs = XLoadQueryFont(m_display, "-*-fixed-medium-r-*-*-14-*-*-*-*-*-*-*");
    if (!fs)
        fs = XLoadQueryFont(m_display, "fixed");
    m_fontInfo = reinterpret_cast<_XFontStruct*>(fs);
    if (fs)
        XSetFont(m_display, reinterpret_cast<GC>(m_gc), fs->fid);

    // ── Colors ────────────────────────────────────────────────────────────
    m_panelPx   = allocRGB(m_display,  30,  30,  30);
    m_dividerPx = allocRGB(m_display,  80,  80,  80);
    m_textPx    = allocRGB(m_display, 220, 220, 220);

    // ── Cursors ───────────────────────────────────────────────────────────
    m_curArrow = XCreateFontCursor(m_display, XC_left_ptr);
    m_curNS    = XCreateFontCursor(m_display, XC_sb_v_double_arrow);
    m_curEW    = XCreateFontCursor(m_display, XC_sb_h_double_arrow);
    XDefineCursor(m_display, m_window, m_curArrow);

    return true;
}

void X11Window::show()
{
    if (m_display && m_window)
        XMapWindow(m_display, m_window);
}

// ── Painting ──────────────────────────────────────────────────────────────

void X11Window::paint()
{
    GC gc = reinterpret_cast<GC>(m_gc);

    const int W     = m_width;
    const int H     = m_height;
    const int topH  = static_cast<int>(H * m_hRatio);
    const int leftW = static_cast<int>(W * m_vRatio);

    // Fill dividers (whole background first, then panels on top)
    XSetForeground(m_display, gc, m_dividerPx);
    XFillRectangle(m_display, m_window, gc, 0, 0,
                   static_cast<unsigned>(W), static_cast<unsigned>(H));

    // Fill panels
    XSetForeground(m_display, gc, m_panelPx);
    XFillRectangle(m_display, m_window, gc,
                   0, 0,
                   static_cast<unsigned>(leftW),
                   static_cast<unsigned>(topH));                    // left
    XFillRectangle(m_display, m_window, gc,
                   leftW + DIVIDER_W, 0,
                   static_cast<unsigned>(W - leftW - DIVIDER_W),
                   static_cast<unsigned>(topH));                    // right
    XFillRectangle(m_display, m_window, gc,
                   0, topH + DIVIDER_W,
                   static_cast<unsigned>(W),
                   static_cast<unsigned>(H - topH - DIVIDER_W));    // bottom

    // Draw labels (centered)
    XSetForeground(m_display, gc, m_textPx);
    auto* fs = reinterpret_cast<XFontStruct*>(m_fontInfo);

    auto drawCentered = [&](const char* text, int rx, int ry, int rw, int rh)
    {
        int len = static_cast<int>(strlen(text));
        int tw  = fs ? XTextWidth(fs, text, len) : len * 7;
        int th  = fs ? (fs->ascent + fs->descent) : 13;
        int tx  = rx + (rw - tw) / 2;
        int ty  = ry + (rh + th) / 2 - (fs ? fs->descent : 2);
        XDrawString(m_display, m_window, gc, tx, ty, text, len);
    };

    drawCentered("left",   0,              0,             leftW,                topH);
    drawCentered("right",  leftW+DIVIDER_W, 0,            W - leftW - DIVIDER_W, topH);
    drawCentered("bottom", 0,              topH+DIVIDER_W, W,                   H - topH - DIVIDER_W);

    XFlush(m_display);
}

// ── Event loop ────────────────────────────────────────────────────────────

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
                m_width  = event.xconfigure.width;
                m_height = event.xconfigure.height;
                paint();
                break;

            case ButtonPress:
            {
                if (event.xbutton.button != Button1) break;
                const int mx    = event.xbutton.x, my = event.xbutton.y;
                const int topH  = static_cast<int>(m_height * m_hRatio);
                const int leftW = static_cast<int>(m_width  * m_vRatio);

                if (my >= topH - HIT_ZONE && my <= topH + DIVIDER_W + HIT_ZONE)
                    m_drag = Drag::Horiz;
                else if (my < topH &&
                         mx >= leftW - HIT_ZONE && mx <= leftW + DIVIDER_W + HIT_ZONE)
                    m_drag = Drag::Vert;
                break;
            }

            case MotionNotify:
            {
                const int mx = event.xmotion.x, my = event.xmotion.y;
                const int topH  = static_cast<int>(m_height * m_hRatio);
                const int leftW = static_cast<int>(m_width  * m_vRatio);

                if (m_drag == Drag::Horiz && m_height > 0)
                {
                    m_hRatio = clamp(static_cast<float>(my) / m_height);
                    paint();
                }
                else if (m_drag == Drag::Vert && m_width > 0)
                {
                    m_vRatio = clamp(static_cast<float>(mx) / m_width);
                    paint();
                }
                else
                {
                    // Update cursor based on position
                    bool nearH = (my >= topH  - HIT_ZONE && my <= topH  + DIVIDER_W + HIT_ZONE);
                    bool nearV = (my <  topH  &&
                                  mx >= leftW - HIT_ZONE && mx <= leftW + DIVIDER_W + HIT_ZONE);
                    unsigned long cur = nearH ? m_curNS : nearV ? m_curEW : m_curArrow;
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
                KeySym key = XLookupKeysym(&event.xkey, 0);
                if (key == XK_q || key == XK_Escape)
                    m_running = false;
                break;
            }

            case ClientMessage:
                if (static_cast<Atom>(event.xclient.data.l[0]) == wm_delete)
                    m_running = false;
                break;

            default:
                break;
        }
    }
}

void X11Window::close()
{
    m_running = false;
}

std::unique_ptr<uc::Window> createWindow()
{
    return std::make_unique<X11Window>();
}
