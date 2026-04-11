#pragma once
#include "ui/events.h"
#include "ui/render_context.h"
#include <functional>
#include <memory>
#include <vector>

namespace uc {

// --- Rect ---------------------------------------------------------------
struct Rect { int x, y, w, h; };

// --- Widget -------------------------------------------------------------
// Base class for all UI elements: panels, overlays, dialogs, the f-key bar,
// and (eventually) the root window itself.
//
// Geometry is computed top-down: the parent calls layout() on each child,
// passing its own bounds. The child decides where it sits within those bounds
// according to its SizePolicy, then propagates to its own children.
//
// Input is dispatched by the platform window to the appropriate widget.
// Return true from handle*() to signal the event was consumed.
//
// Rendering is NOT here -- platform windows traverse the tree and draw each
// widget type. A RenderContext abstraction will be added in a later step.

class Widget : public std::enable_shared_from_this<Widget>
{
public:
    // --- Size policy ---
    // Fixed      : fixed pixel size (m_fixedW/H), anchored at parent's top-left.
    // FillParent : stretch to fill all of the parent bounds.
    // Centered   : fixed pixel size, centered within parent bounds.
    // Relative   : position and size as fractions of parent bounds (0.0-1.0).
    //              Use setRelativeBounds(). Scales with parent on every resize.
    //              Suitable for panels whose share of the window can be dragged.
    enum class SizePolicy { Fixed, FillParent, Centered, Relative };

    virtual ~Widget() = default;

    // --- Hierarchy ---
    void addChild(std::shared_ptr<Widget> child);
    void removeChild(Widget* child);

    Widget* parent() const { return m_parent.lock().get(); }
    const std::vector<std::shared_ptr<Widget>>& children() const { return m_children; }

    // --- Layout ---
    // Called by the parent with its own bounds. Computes m_x/m_y/m_w/m_h
    // according to the size policy, then propagates to all children.
    virtual void layout(int parentX, int parentY, int parentW, int parentH);

    Rect bounds() const { return { m_x, m_y, m_w, m_h }; }

    void setSizePolicy(SizePolicy policy) { m_sizePolicy = policy; }
    void setFixedSize(int w, int h)       { m_fixedW = w; m_fixedH = h; }
    // rx,ry,rw,rh are all in [0,1] relative to parent bounds.
    void setRelativeBounds(float rx, float ry, float rw, float rh)
    {
        m_relX = rx; m_relY = ry; m_relW = rw; m_relH = rh;
    }
    SizePolicy sizePolicy() const { return m_sizePolicy; }

    // --- Visibility ---
    bool isVisible() const  { return m_visible; }
    void setVisible(bool v) { m_visible = v; }

    // --- Hit testing ---
    // Returns the deepest visible child (or this) whose bounds contain (x,y),
    // or nullptr if (x,y) is outside this widget or it is hidden.
    // Children are tested in reverse order so the topmost (last-added) wins.
    virtual Widget* hitTest(int x, int y);

    // --- Rendering ---
    // Called by the platform paint handler, top-down through the tree.
    // Default implementation paints nothing but recurses into children.
    virtual void paint(RenderContext& ctx);

    // --- Repaint request ---
    // The platform window injects this so widgets can request a repaint
    // without knowing which platform they run on.
    void setInvalidateCallback(std::function<void()> fn) { m_invalidateCb = std::move(fn); }

    // --- Input ---
    virtual bool handleKeyDown(Key key)        { (void)key;        return false; }
    virtual bool handleMouseDown(int x, int y) { (void)x; (void)y; return false; }

protected:
    void requestRepaint() { if (m_invalidateCb) m_invalidateCb(); }

    int m_x{}, m_y{}, m_w{}, m_h{};

    // Fixed / Centered
    int m_fixedW{}, m_fixedH{};

    // Relative
    float m_relX{}, m_relY{}, m_relW{ 1.0f }, m_relH{ 1.0f };

    SizePolicy m_sizePolicy { SizePolicy::FillParent };
    bool m_visible { true };

    std::weak_ptr<Widget>                m_parent;
    std::vector<std::shared_ptr<Widget>> m_children;

private:
    std::function<void()> m_invalidateCb;
};

} // namespace uc
