#include "x11_window.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

X11Window::X11Window()  = default;

X11Window::~X11Window()
{
    if (m_display)
    {
        if (m_window) XDestroyWindow(m_display, m_window);
        XCloseDisplay(m_display);
    }
}

bool X11Window::create(const std::string& title, int width, int height)
{
    m_display = XOpenDisplay(nullptr);
    if (!m_display)
        return false;

    int screen = DefaultScreen(m_display);

    m_window = XCreateSimpleWindow(
        m_display,
        RootWindow(m_display, screen),
        0, 0,
        static_cast<unsigned>(width),
        static_cast<unsigned>(height),
        1,
        BlackPixel(m_display, screen),
        WhitePixel(m_display, screen)
    );

    XStoreName(m_display, m_window, title.c_str());

    Atom wm_delete = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(m_display, m_window, &wm_delete, 1);

    XSelectInput(m_display, m_window,
                 ExposureMask | KeyPressMask | StructureNotifyMask);
    return true;
}

void X11Window::show()
{
    if (m_display && m_window)
        XMapWindow(m_display, m_window);
}

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
            case ClientMessage:
                if (static_cast<Atom>(event.xclient.data.l[0]) == wm_delete)
                    m_running = false;
                break;

            case KeyPress:
            {
                KeySym key = XLookupKeysym(&event.xkey, 0);
                if (key == XK_q || key == XK_Escape)
                    m_running = false;
                break;
            }

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
