#include "ui/directory_panel.h"
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace uc {

DirectoryPanel::DirectoryPanel(const std::string& path)
{
    setPath(path.empty() ? fs::current_path().string() : path);
}

void DirectoryPanel::setPath(const std::string& path, const std::string& selectName)
{
    std::error_code ec;
    fs::path p = fs::canonical(fs::path(path), ec);
    if (ec) p = fs::path(path);
    m_path          = p;
    m_selectedIndex = 0;
    m_scrollOffset  = 0;
    refresh();
    if (!selectName.empty())
    {
        for (int i = 0; i < (int)m_entries.size(); ++i)
        {
            if (m_entries[i].name == selectName)
            {
                m_selectedIndex = i;
                break;
            }
        }
    }
}

void DirectoryPanel::resetPanel(const std::string& path, bool focused)
{
    setPath(path);
    m_hasFocus = focused;
}

std::string DirectoryPanel::getPath() const
{
    return m_path.string();
}

void DirectoryPanel::refresh()
{
    m_entries.clear();

    if (m_path != m_path.root_path())
        m_entries.push_back({"..", true});

    std::vector<Entry> dirs, files;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(m_path, ec))
    {
        std::error_code ec2;
        bool dir = e.is_directory(ec2) && !ec2;
        if (dir)
            dirs.push_back({e.path().filename().string(), true});
        else
            files.push_back({e.path().filename().string(), false});
    }

    auto cmpCI = [](const Entry& a, const Entry& b) {
        std::string la = a.name, lb = b.name;
        std::transform(la.begin(), la.end(), la.begin(),
                       [](unsigned char c){ return (char)std::tolower(c); });
        std::transform(lb.begin(), lb.end(), lb.begin(),
                       [](unsigned char c){ return (char)std::tolower(c); });
        return la < lb;
    };
    std::sort(dirs.begin(),  dirs.end(),  cmpCI);
    std::sort(files.begin(), files.end(), cmpCI);

    for (auto& d : dirs)  m_entries.push_back(std::move(d));
    for (auto& f : files) m_entries.push_back(std::move(f));

    if (m_selectedIndex >= (int)m_entries.size())
        m_selectedIndex = std::max(0, (int)m_entries.size() - 1);
}

void DirectoryPanel::moveUp()
{
    if (m_selectedIndex > 0)
        --m_selectedIndex;
}

void DirectoryPanel::moveDown()
{
    if (m_selectedIndex < (int)m_entries.size() - 1)
        ++m_selectedIndex;
}

void DirectoryPanel::activate()
{
    if (m_entries.empty()) return;
    const Entry& sel = m_entries[m_selectedIndex];
    if (sel.name == "..")
        setPath(m_path.parent_path().string(), m_path.filename().string());
    else if (sel.isDir)
        setPath((m_path / sel.name).string());
    // regular files: no action yet
}

void DirectoryPanel::ensureVisible(int visibleRows)
{
    if (visibleRows <= 0) return;
    if (m_selectedIndex < m_scrollOffset)
        m_scrollOffset = m_selectedIndex;
    else if (m_selectedIndex >= m_scrollOffset + visibleRows)
        m_scrollOffset = m_selectedIndex - visibleRows + 1;
    if (m_scrollOffset < 0) m_scrollOffset = 0;
}

} // namespace uc
