#include "ui/base_window.h"
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

namespace uc {

// Row 0 = Normal, 1 = Shift, 2 = Ctrl, 3 = Alt.
const char* const BaseWindow::FKEY_LABELS[4][10] = {
    { "Help", "",  "",  "", "Copy", "",  "",  "",  "", "Quit" },  // Normal
    { "",  "",  "",  "",  "",  "",  "",  "",  "",  ""    },  // Shift
    { "",  "",  "",  "",  "",  "",  "",  "",  "",  ""    },  // Ctrl
    { "",  "",  "",  "",  "",  "",  "",  "",  "",  ""    },  // Alt
};

std::string BaseWindow::stateSnapshot() const
{
    auto* lp = leftPanel();
    auto* rp = rightPanel();
    const bool leftFocused = lp && lp->hasFocus();
    std::ostringstream ss;
    ss << "focus="          << (leftFocused ? "left" : "right")
       << " leftSelected="  << (lp ? lp->selectedIndex()  : 0)
       << " rightSelected=" << (rp ? rp->selectedIndex() : 0)
       << " leftPath="      << (lp ? lp->getPath()        : "")
       << " rightPath="     << (rp ? rp->getPath()       : "")
       << " leftEntries="   << serializeEntries(lp)
       << " rightEntries="  << serializeEntries(rp)
       << " hRatio="        << m_hRatio
       << " vRatio="        << m_vRatio
       << " modAlt="        << (m_modSticky[0] ? 1 : 0)
       << " modShift="      << (m_modSticky[1] ? 1 : 0)
       << " modCtrl="       << (m_modSticky[2] ? 1 : 0)
       << " helpVisible="   << (m_helpWidget.isVisible() ? 1 : 0);
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

bool BaseWindow::confirmQuit()
{
    if (m_testDialogAnswer.has_value())
    {
        bool ans = *m_testDialogAnswer;
        m_testDialogAnswer.reset();
        return ans;
    }
    m_confirmWidget.show("Quit UniCommander?", "");
    return m_confirmWidget.result();
}

bool BaseWindow::confirmCopy(const std::string& srcName, const std::string& dstPath)
{
    if (m_testDialogAnswer.has_value())
    {
        bool ans = *m_testDialogAnswer;
        m_testDialogAnswer.reset();
        return ans;
    }
    m_confirmWidget.show("Copy \"" + srcName + "\"", "to " + dstPath + " ?");
    return m_confirmWidget.result();
}

void BaseWindow::handleKeyDown(Key key)
{
    // Modal widgets capture input first (front-to-back priority).
    if (m_confirmWidget.isVisible())
    {
        if (m_confirmWidget.handleKeyDown(key)) { invalidate(); return; }
    }
    if (m_helpWidget.isVisible())
    {
        if (m_helpWidget.handleKeyDown(key)) { invalidate(); return; }
    }

    switch (key)
    {
        case Key::F1:
            m_helpWidget.toggle();
            break;

        case Key::F5:
            handleCopy();
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

// --- Copy ---

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

    auto* dst = (src == leftPanel()) ? rightPanel() : leftPanel();
    if (!dst) return;

    const auto& entries = src->entries();
    if (entries.empty()) return;

    const auto& sel = entries[src->selectedIndex()];
    if (sel.name == "..") return;

    fs::path srcPath = fs::path(src->getPath()) / sel.name;
    fs::path dstPath = fs::path(dst->getPath()) / sel.name;

    if (srcPath == dstPath) return;

    if (!confirmCopy(sel.name, dst->getPath())) return;

    if (!copyEntry(srcPath, dstPath)) return;

    src->refresh();
    dst->refresh();
    invalidate();
}

} // namespace uc
