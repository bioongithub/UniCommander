#include "ui/base_window.h"
#include <sstream>

namespace uc {

// Row 0 = Normal, 1 = Shift, 2 = Ctrl, 3 = Alt.
// Only F10 Normal (Quit) is implemented; all other combinations are blank.
const char* const BaseWindow::FKEY_LABELS[4][10] = {
    { "Help", "",  "",  "",  "",  "",  "",  "",  "", "Quit" },  // Normal
    { "",  "",  "",  "",  "",  "",  "",  "",  "",  ""    },  // Shift
    { "",  "",  "",  "",  "",  "",  "",  "",  "",  ""    },  // Ctrl
    { "",  "",  "",  "",  "",  "",  "",  "",  "",  ""    },  // Alt
};

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
       << " vRatio="        << m_vRatio
       << " modAlt="        << (m_modSticky[0] ? 1 : 0)
       << " modShift="      << (m_modSticky[1] ? 1 : 0)
       << " modCtrl="       << (m_modSticky[2] ? 1 : 0)
       << " helpVisible="   << (m_helpWindow.isVisible() ? 1 : 0);
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
        case Key::F1:
            m_helpWindow.toggle();
            invalidate();
            break;

        case Key::Escape:
            if (m_helpWindow.isVisible()) { m_helpWindow.close(); invalidate(); }
            break;

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
