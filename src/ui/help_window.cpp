#include "ui/help_window.h"

namespace uc {

const char* const HelpWindow::LINES[] = {
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

HelpWindow::Box HelpWindow::computeBox(int W, int H)
{
    int lineCount = 0;
    for (const char* const* p = LINES; *p; ++p) ++lineCount;
    int boxH = PADDING * 2 + lineCount * LINE_H;
    return { (W - BOX_W) / 2, (H - boxH) / 2, BOX_W, boxH };
}

} // namespace uc
