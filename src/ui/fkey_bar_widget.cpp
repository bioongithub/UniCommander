#include "ui/fkey_bar_widget.h"
#include "ui/render_context.h"
#include <algorithm>
#include <string>

namespace uc {

const char* const FKeyBarWidget::MOD_LABELS[3] = { "Alt", "Sft", "Ctl" };

FKeyBarWidget::FKeyBarWidget()
{
    setSizePolicy(SizePolicy::FillParent);
}

void FKeyBarWidget::setLabelsFn(std::function<const char*(int, int)> fn)
{
    m_labelsFn = std::move(fn);
}

void FKeyBarWidget::setActiveRowFn(std::function<int()> fn)
{
    m_activeRowFn = std::move(fn);
}

void FKeyBarWidget::setModActiveFn(std::function<bool(int)> fn)
{
    m_modActiveFn = std::move(fn);
}

void FKeyBarWidget::paint(RenderContext& ctx)
{
    if (!m_visible) return;

    const int barX  = m_x;
    const int barY  = m_y;
    const int barW  = m_w;
    const int barH  = m_h;

    const int effW  = barW - MOD_AREA_W;
    const int cellW = std::max(1, effW / 10);
    const int row   = m_activeRowFn ? m_activeRowFn() : 0;

    // --- F-key cells ---
    for (int i = 0; i < 10; ++i)
    {
        int cx = barX + i * cellW;

        // Number sub-column (black bg, yellow text)
        ctx.fillRect(cx, barY, NUM_W, barH, {0, 0, 0});
        std::string numStr = std::to_string(i + 1);
        ctx.drawText(cx + 2, barY + 2, numStr.c_str(), {255, 255, 85});

        // Label sub-column
        const int lblX = cx + NUM_W;
        const int lblW = cellW - NUM_W;
        const char* lbl = (m_labelsFn && lblW > 0) ? m_labelsFn(row, i) : "";
        if (lbl && lbl[0] != '\0')
        {
            // Implemented key: cyan background, black text
            ctx.fillRect(lblX, barY, lblW, barH, {0, 170, 170});
            ctx.drawText(lblX + 2, barY + 2, lbl, {0, 0, 0});
        }
        else
        {
            ctx.fillRect(lblX, barY, lblW, barH, {0, 0, 0});
        }
    }

    // --- Modifier cells (Alt / Sft / Ctl) ---
    for (int i = 0; i < 3; ++i)
    {
        int mx     = barX + effW + i * MOD_CELL_W;
        bool active = m_modActiveFn ? m_modActiveFn(i) : false;
        Color bg   = active ? Color{170, 170, 0} : Color{0, 0, 100};
        Color fg   = active ? Color{0, 0, 0}     : Color{200, 200, 200};
        ctx.fillRect(mx, barY, MOD_CELL_W, barH, bg);
        ctx.drawText(mx + 4, barY + 2, MOD_LABELS[i], fg);
    }
}

} // namespace uc
