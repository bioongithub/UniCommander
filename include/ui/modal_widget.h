#pragma once
#include "ui/widget.h"
#include "ui/message_loop.h"

namespace uc {

// Base for all modal (blocking or non-blocking overlay) widgets.
// Subclasses that need to block their caller (e.g. ConfirmWidget) call
// beginModal() inside their show() method; non-blocking overlays (e.g.
// HelpWidget) just call setVisible(true) directly.
//
// The platform window injects the pump function once via setPump() so that
// beginModal() can run a nested event loop on the correct platform.

class ModalWidget : public Widget
{
public:
    void setPump(MessageLoop::PumpFn fn) { m_loop.setPump(std::move(fn)); }

protected:
    // Sets visible + blocks in the nested event loop until endModal() is called.
    void beginModal();

    // Unblocks beginModal(), sets invisible. Call from a key/mouse handler.
    void endModal();

    MessageLoop m_loop;
};

} // namespace uc
