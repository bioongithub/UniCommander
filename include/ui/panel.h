#pragma once

namespace uc {

enum class PanelType { Directory, Terminal };

// Minimal abstract interface shared by all panel types.
// Rendering is platform-specific and handled by the window.
class Panel
{
public:
    virtual ~Panel() = default;

    virtual PanelType type()               const = 0;
    virtual bool      hasFocus()           const = 0;
    virtual void      setFocus(bool focused)     = 0;
};

} // namespace uc
