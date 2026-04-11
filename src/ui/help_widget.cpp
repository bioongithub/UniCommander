#include "ui/help_widget.h"

namespace uc {

const char* const HelpWidget::LINES[] = {
    "UniCommander - Key Reference",
    "",
    "Tab       Switch panel focus",
    "Up/Down   Move selection",
    "Enter     Open dir / parent (..)",
    "",
    "F1        Help (this screen)",
    "F5        Copy to other panel",
    "F10       Quit (with confirmation)",
    "",
    "Escape or F1 to close",
    nullptr
};

int HelpWidget::computeHeight()
{
    int n = 0;
    for (const char* const* p = LINES; *p; ++p) ++n;
    return PADDING * 2 + n * LINE_H;
}

HelpWidget::HelpWidget()
{
    setSizePolicy(SizePolicy::Centered);
    setFixedSize(BOX_W, computeHeight());
    setVisible(false);
}

bool HelpWidget::handleKeyDown(Key key)
{
    if (!isVisible()) return false;
    if (key == Key::Escape || key == Key::F1 || key == Key::Return)
    {
        close();
        return true;
    }
    return true;  // swallow all keys while help is open
}

void HelpWidget::paint(RenderContext& ctx)
{
    if (!isVisible()) return;

    constexpr Color BG    { 20,  20,  50};
    constexpr Color BORDER{100, 180, 255};
    constexpr Color TITLE {100, 180, 255};
    constexpr Color BODY  {220, 220, 220};

    ctx.fillRect(m_x, m_y, m_w, m_h, BG);
    ctx.drawRect(m_x, m_y, m_w, m_h, BORDER);

    int y = m_y + PADDING;
    for (int i = 0; LINES[i]; ++i)
    {
        if (LINES[i][0] != '\0')
            ctx.drawText(m_x + PADDING, y, LINES[i], i == 0 ? TITLE : BODY);
        y += LINE_H;
    }
}

} // namespace uc
