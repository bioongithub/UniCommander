#pragma once
#include "ui/modal_widget.h"

namespace uc {

// Keyboard-shortcut help overlay.
// Non-blocking: show() / toggle() just set visibility; the main event loop
// continues running. handleKeyDown() consumes Escape / F1 / Enter to close.
//
// Uses SizePolicy::Centered with a fixed pixel size computed from LINES[].

class HelpWidget : public ModalWidget
{
public:
    static const char* const LINES[];  // nullptr-terminated; [0] is the title
    static constexpr int LINE_H  = 18;
    static constexpr int PADDING = 12;
    static constexpr int BOX_W   = 340;

    HelpWidget();

    // Non-blocking show/hide.
    void show()   { setVisible(true);  requestRepaint(); }
    void close()  { setVisible(false); requestRepaint(); }
    void toggle() { isVisible() ? close() : show(); }
    void reset()  { close(); }

    bool handleKeyDown(Key key) override;
    void paint(RenderContext& ctx) override;

private:
    static int computeHeight();
};

} // namespace uc
