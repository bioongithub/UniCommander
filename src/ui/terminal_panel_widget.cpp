#include "ui/terminal_panel_widget.h"
#include "ui/render_context.h"

namespace uc {

TerminalPanelWidget::TerminalPanelWidget()
{
    setSizePolicy(SizePolicy::FillParent);
}

void TerminalPanelWidget::paint(RenderContext& ctx)
{
    if (!m_visible) return;

    ctx.fillRect(m_x, m_y, m_w, m_h, {30, 30, 30});

    // Centered label -- y is roughly midpoint minus one text row
    int tx = m_x + m_w / 2 - 30;   // rough centering for "Terminal"
    int ty = m_y + m_h / 2 - ctx.charHeight() / 2;
    ctx.drawText(tx, ty, "Terminal", {220, 220, 220});
}

} // namespace uc
