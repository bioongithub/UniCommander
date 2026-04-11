#pragma once
#include "ui/widget.h"

namespace uc {

// Placeholder widget for the bottom terminal panel.
// Renders a dark background with a centered "Terminal" label.
// Layout: SizePolicy::Relative -- BaseWindow assigns its fractional bounds.

class TerminalPanelWidget : public Widget
{
public:
    TerminalPanelWidget();
    void paint(RenderContext& ctx) override;
};

} // namespace uc
