#include "ui/base_window.h"
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

namespace uc {

// Row 0 = Normal, 1 = Shift, 2 = Ctrl, 3 = Alt.
// Only F10 Normal (Quit) is implemented; all other combinations are blank.
const char* const BaseWindow::FKEY_LABELS[4][10] = {
    { "Help", "",  "",  "", "Copy", "",  "",  "",  "", "Quit" },  // Normal
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

        case Key::F5:
            handleCopy();
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

// Recursive copy that works consistently across all platforms and MSVC versions.
// Returns false on the first error (silently — caller may add error surfacing).
static bool copyEntry(const fs::path& src, const fs::path& dst)
{
    try
    {
        std::error_code ec;
        if (fs::is_regular_file(src))
        {
            fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
            return !ec;
        }
        if (fs::is_directory(src))
        {
            fs::create_directory(dst, ec);
            if (ec) return false;
            for (const auto& e : fs::directory_iterator(src))
            {
                if (!copyEntry(e.path(), dst / e.path().filename()))
                    return false;
            }
            return true;
        }
    }
    catch (...) {}
    return false;
}

void BaseWindow::handleCopy()
{
    auto* src = focusedPanel();
    if (!src) return;

    auto* dst = (src == m_leftPanel.get()) ? m_rightPanel.get() : m_leftPanel.get();
    if (!dst) return;

    const auto& entries = src->entries();
    if (entries.empty()) return;

    const auto& sel = entries[src->selectedIndex()];
    if (sel.name == "..") return;  // cannot copy the parent-directory entry

    fs::path srcPath = fs::path(src->getPath()) / sel.name;
    fs::path dstPath = fs::path(dst->getPath()) / sel.name;

    if (srcPath == dstPath) return;  // source and destination are identical

    if (!confirmCopy(sel.name, dst->getPath())) return;

    if (!copyEntry(srcPath, dstPath)) return;  // TODO: surface via virtual showError()

    src->refresh();
    dst->refresh();
    invalidate();
}

} // namespace uc
