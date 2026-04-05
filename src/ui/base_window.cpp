#include "ui/base_window.h"
#include <sstream>

namespace uc {

std::string BaseWindow::stateSnapshot() const
{
    const bool leftFocused = m_leftPanel && m_leftPanel->hasFocus();
    std::ostringstream ss;
    ss << "focus="          << (leftFocused ? "left" : "right")
       << " leftSelected="  << (m_leftPanel  ? m_leftPanel->selectedIndex()  : 0)
       << " rightSelected=" << (m_rightPanel ? m_rightPanel->selectedIndex() : 0)
       << " leftPath="      << (m_leftPanel  ? m_leftPanel->getPath()        : "")
       << " rightPath="     << (m_rightPanel ? m_rightPanel->getPath()       : "")
       << " leftEntries="   << serializeEntries(m_leftPanel.get())
       << " rightEntries="  << serializeEntries(m_rightPanel.get())
       << " hRatio="        << m_hRatio
       << " vRatio="        << m_vRatio;
    return ss.str();
}

std::string BaseWindow::serializeEntries(const DirectoryPanel* panel)
{
    if (!panel || panel->entries().empty()) return "";
    std::string result;
    for (const auto& e : panel->entries())
    {
        if (!result.empty()) result += ',';
        result += e.name;
        if (e.isDir) result += '/';
    }
    return result;
}

void BaseWindow::handleKeyDown(Key key)
{
    switch (key)
    {
        case Key::Tab:
            switchFocus();
            invalidate();
            break;

        case Key::Up:
            if (auto* p = focusedPanel()) { p->moveUp(); invalidate(); }
            break;

        case Key::Down:
            if (auto* p = focusedPanel()) { p->moveDown(); invalidate(); }
            break;

        case Key::Return:
            if (auto* p = focusedPanel()) { p->activate(); invalidate(); }
            break;

        case Key::F10:
            if (confirmQuit()) close();
            break;

        default:
            break;
    }
}

} // namespace uc
