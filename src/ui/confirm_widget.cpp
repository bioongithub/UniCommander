#include "ui/confirm_widget.h"

namespace uc {

ConfirmWidget::ConfirmWidget()
{
    setSizePolicy(SizePolicy::Centered);
    setFixedSize(BOX_W, BOX_H);
    setVisible(false);
}

void ConfirmWidget::show(const std::string& line1, const std::string& line2)
{
    m_line1         = line1;
    m_line2         = line2;
    m_focusedButton = 1;     // default: No
    m_result        = false;
    requestRepaint();
    beginModal();            // blocks until endModal()
}

bool ConfirmWidget::handleKeyDown(Key key)
{
    if (!isVisible()) return false;

    switch (key)
    {
        case Key::Return:
            m_result = (m_focusedButton == 0);
            endModal();
            return true;

        case Key::Escape:
            m_result = false;
            endModal();
            return true;

        case Key::Tab:
        case Key::Left:
        case Key::Right:
            m_focusedButton = 1 - m_focusedButton;
            requestRepaint();
            return true;

        default:
            // Y / N handled via raw character — not in Key enum yet, so
            // consume everything to prevent background action while modal shows.
            return true;
    }
}

bool ConfirmWidget::handleMouseDown(int x, int y)
{
    if (!isVisible()) return false;

    // Yes button
    if (x >= yesX() && x < yesX() + BTN_W &&
        y >= btnY() && y < btnY() + BTN_H)
    {
        m_focusedButton = 0;
        m_result = true;
        endModal();
        return true;
    }

    // No button
    if (x >= noX() && x < noX() + BTN_W &&
        y >= btnY() && y < btnY() + BTN_H)
    {
        m_focusedButton = 1;
        m_result = false;
        endModal();
        return true;
    }

    // Click anywhere else inside the dialog: consume but keep open
    return true;
}

void ConfirmWidget::paint(RenderContext& ctx)
{
    if (!isVisible()) return;

    constexpr Color BG       { 40,  40,  55};
    constexpr Color BORDER   {  0, 120, 215};
    constexpr Color TEXT     {255, 255, 255};
    constexpr Color BTN_IDLE { 55,  55,  75};
    constexpr Color BTN_HOT  {  0,  80, 160};

    ctx.fillRect(m_x, m_y, m_w, m_h, BG);
    ctx.drawRect(m_x, m_y, m_w, m_h, BORDER);

    // --- Message lines ---
    const int lineH = ctx.charHeight() + 4;
    const bool twoLines = !m_line2.empty();
    const int textAreaH = twoLines ? lineH * 2 : lineH;
    const int btnAreaH  = BTN_H + 20;
    int textTop = m_y + (m_h - btnAreaH - textAreaH) / 2;

    auto drawCentered = [&](const std::string& s, int y)
    {
        int tw = ctx.textWidth(s.c_str(), static_cast<int>(s.size()));
        ctx.drawText(m_x + (m_w - tw) / 2, y, s.c_str(), TEXT);
    };

    drawCentered(m_line1, textTop);
    if (twoLines) drawCentered(m_line2, textTop + lineH);

    // --- Buttons ---
    auto drawBtn = [&](int bx, bool focused, const char* label)
    {
        ctx.fillRect(bx, btnY(), BTN_W, BTN_H, focused ? BTN_HOT : BTN_IDLE);
        ctx.drawRect(bx, btnY(), BTN_W, BTN_H, BORDER);
        int lw = ctx.textWidth(label);
        ctx.drawText(bx + (BTN_W - lw) / 2, btnY() + (BTN_H - ctx.charHeight()) / 2,
                     label, TEXT);
    };

    drawBtn(yesX(), m_focusedButton == 0, "Yes");
    drawBtn(noX(),  m_focusedButton == 1, "No");
}

} // namespace uc
