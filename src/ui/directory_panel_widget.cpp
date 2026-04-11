#include "ui/directory_panel_widget.h"
#include "ui/render_context.h"
#include <algorithm>

namespace uc {

DirectoryPanelWidget::DirectoryPanelWidget(const std::string& path)
    : m_panel(path)
{
    setSizePolicy(SizePolicy::FillParent);
}

void DirectoryPanelWidget::paint(RenderContext& ctx)
{
    if (!m_visible) return;

    const int x = m_x, y = m_y, w = m_w, h = m_h;

    // Scroll to keep selection visible
    const int visibleRows = std::max(0, (h - HEADER_H - 2) / ROW_H);
    m_panel.ensureVisible(visibleRows);

    // Background
    ctx.fillRect(x, y, w, h, {20, 20, 20});

    // Border (blue when focused, gray otherwise)
    Color borderColor = m_panel.hasFocus() ? Color{0, 120, 215} : Color{80, 80, 80};
    ctx.drawRect(x, y, w, h, borderColor);

    // Header -- current path
    ctx.fillRect(x + 1, y + 1, w - 2, HEADER_H, {40, 40, 60});
    ctx.drawText(x + 5, y + 3, m_panel.getPath().c_str(), {180, 180, 255});

    // Entry rows
    const auto& entries     = m_panel.entries();
    const int   scrollOff   = m_panel.scrollOffset();
    const int   selectedIdx = m_panel.selectedIndex();
    int rowY = y + 1 + HEADER_H;

    for (int i = scrollOff;
         i < static_cast<int>(entries.size()) && rowY + ROW_H <= y + h - 1;
         ++i, rowY += ROW_H)
    {
        bool selected = (i == selectedIdx);

        if (selected)
        {
            Color selBg = m_panel.hasFocus() ? Color{0, 80, 160} : Color{55, 55, 55};
            ctx.fillRect(x + 1, rowY, w - 2, ROW_H, selBg);
        }

        const auto& entry = entries[i];
        Color textColor =
            selected       ? Color{255, 255, 255} :
            entry.isDir    ? Color{100, 180, 255} :
                             Color{220, 220, 220};

        std::string label =
            (entry.isDir && entry.name != "..") ? "[" + entry.name + "]"
                                                 : entry.name;
        ctx.drawText(x + 5, rowY + 2, label.c_str(), textColor);
    }
}

} // namespace uc
