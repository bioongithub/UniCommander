#pragma once
#include "panel.h"

namespace uc {

// Placeholder — implementation deferred.
class TerminalPanel : public Panel
{
public:
    PanelType type()     const override { return PanelType::Terminal; }
    bool      hasFocus() const override { return m_hasFocus; }
    void      setFocus(bool focused) override { m_hasFocus = focused; }

private:
    bool m_hasFocus{false};
};

} // namespace uc
