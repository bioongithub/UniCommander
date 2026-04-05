#pragma once
#include "panel.h"
#include <filesystem>
#include <string>
#include <vector>

namespace uc {

class DirectoryPanel : public Panel
{
public:
    struct Entry {
        std::string name;
        bool        isDir{false};
    };

    explicit DirectoryPanel(const std::string& path = "");

    PanelType type()     const override { return PanelType::Directory; }
    bool      hasFocus() const override { return m_hasFocus; }
    void      setFocus(bool focused) override { m_hasFocus = focused; }

    void        setPath(const std::string& path);
    void        resetPanel(const std::string& path, bool focused);
    std::string getPath() const;
    void        refresh();

    void moveUp();
    void moveDown();
    void activate();                    // enter dir or go up via ".."
    void ensureVisible(int visibleRows);

    const std::vector<Entry>& entries()       const { return m_entries; }
    int                       selectedIndex() const { return m_selectedIndex; }
    int                       scrollOffset()  const { return m_scrollOffset; }

private:
    std::filesystem::path m_path;
    std::vector<Entry>    m_entries;
    int                   m_selectedIndex{0};
    int                   m_scrollOffset{0};
    bool                  m_hasFocus{false};
};

} // namespace uc
