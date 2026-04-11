#pragma once
#include "ui/widget.h"
#include "ui/directory_panel.h"
#include <string>

namespace uc {

// Widget wrapper around DirectoryPanel.
// Owns the panel data/logic; paint() renders it via the abstract RenderContext,
// replacing the per-platform renderDirectoryPanel() functions.
//
// Layout: SizePolicy::Relative -- BaseWindow sets the fractional bounds from
// its hRatio / vRatio whenever a divider is dragged or the window resizes.

class DirectoryPanelWidget : public Widget
{
public:
    static constexpr int ROW_H    = 18;
    static constexpr int HEADER_H = 20;

    explicit DirectoryPanelWidget(const std::string& path);

    // --- Panel data access (mirrors the DirectoryPanel interface) ---
    DirectoryPanel&       panel()       { return m_panel; }
    const DirectoryPanel& panel() const { return m_panel; }

    void paint(RenderContext& ctx) override;

private:
    DirectoryPanel m_panel;
};

} // namespace uc
