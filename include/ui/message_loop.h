#pragma once
#include <functional>

namespace uc {

class Widget;

// --- MessageLoop --------------------------------------------------------
// Drives a nested platform event pump, routing events through a root Widget,
// until stop() is called.  Used by modal widgets (HelpWidget, ConfirmWidget)
// to block their caller synchronously without nesting platform-specific code.
//
// Usage:
//   m_loop.setPump([this](auto done){ pumpEventsUntil(done); });  // once, at init
//   ...
//   m_loop.run(this);   // blocks inside show() until stop() is called
//   m_loop.stop();      // called by the modal on dismiss / key press

class MessageLoop
{
public:
    // PumpFn: provided by BaseWindow (platform-specific).
    // It must pump native events until done() returns true.
    using PumpFn = std::function<void(std::function<bool()> done)>;

    void setPump(PumpFn fn) { m_pump = std::move(fn); }

    // Blocks until stop() is called; routes events through root.
    void run(Widget* root);

    // Unblocks run() — call from within the modal's event handler.
    void stop();

    bool    isRunning() const { return m_running; }
    Widget* root()      const { return m_root; }

private:
    PumpFn  m_pump;
    bool    m_running { false };
    Widget* m_root    { nullptr };
};

} // namespace uc
