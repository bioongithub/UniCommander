#include "ui/message_loop.h"

namespace uc {

void MessageLoop::run(Widget* root)
{
    m_root    = root;
    m_running = true;
    if (m_pump)
        m_pump([this] { return !m_running; });
    m_running = false;
    m_root    = nullptr;
}

void MessageLoop::stop()
{
    m_running = false;
}

} // namespace uc
