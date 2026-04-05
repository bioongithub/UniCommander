#pragma once
#include "ui/window.h"
#include "ui/directory_panel.h"
#include <algorithm>
#include <memory>
#include <string>

namespace uc {

// Intermediate base that holds layout state shared by all platform windows.
// Platform classes inherit from this instead of Window directly.
class BaseWindow : public Window
{
public:
    static constexpr int DIVIDER_W = 5;
    static constexpr int HIT_ZONE  = 4;

    // --- Key enum ---
    enum class Key { Up, Down, Return, Tab, Escape, Q,
                     F1, F2, F3, F4, F5, F6, F7, F8, F9, F10 };

    // --- Window size ---
    int  width()  const { return m_width; }
    int  height() const { return m_height; }
    void setSize(int w, int h) { m_width = w; m_height = h; }

    // --- Layout ---
    struct Layout { int topH; int leftW; };

    Layout computeLayout(int W, int H) const
    {
        return { static_cast<int>(H * m_hRatio), static_cast<int>(W * m_vRatio) };
    }

    // --- Hit testing ---
    enum class Hit { HorizDivider, VertDivider, LeftPanel, RightPanel, Bottom };

    Hit hitTest(int mx, int my, int W, int H) const
    {
        auto [topH, leftW] = computeLayout(W, H);
        if (my >= topH - HIT_ZONE && my <= topH + DIVIDER_W + HIT_ZONE)
            return Hit::HorizDivider;
        if (my < topH && mx >= leftW - HIT_ZONE && mx <= leftW + DIVIDER_W + HIT_ZONE)
            return Hit::VertDivider;
        if (my < topH)
            return mx < leftW ? Hit::LeftPanel : Hit::RightPanel;
        return Hit::Bottom;
    }

    // --- Ratios ---
    float hRatio() const { return m_hRatio; }
    float vRatio() const { return m_vRatio; }
    void  setHRatio(float v) { m_hRatio = clamp(v); }
    void  setVRatio(float v) { m_vRatio = clamp(v); }

    // --- Panel access ---
    DirectoryPanel* leftPanel()  const { return m_leftPanel.get(); }
    DirectoryPanel* rightPanel() const { return m_rightPanel.get(); }

    DirectoryPanel* focusedPanel() const
    {
        if (m_leftPanel  && m_leftPanel->hasFocus())  return m_leftPanel.get();
        if (m_rightPanel && m_rightPanel->hasFocus()) return m_rightPanel.get();
        return nullptr;
    }

    void switchFocus()
    {
        if (!m_leftPanel || !m_rightPanel) return;
        bool leftWasFocused = m_leftPanel->hasFocus();
        m_leftPanel->setFocus(!leftWasFocused);
        m_rightPanel->setFocus(leftWasFocused);
    }

    // --- Event handling ---
    virtual void invalidate() = 0;   // platform: schedule a repaint
    void handleKeyDown(Key key);

    // --- State reset (for --test mode) ---
    void resetState(const std::string& dir)
    {
        m_hRatio = 0.5f;
        m_vRatio = 0.5f;
        m_drag   = Drag::Idle;
        if (m_leftPanel)  m_leftPanel->resetPanel(dir, true);
        if (m_rightPanel) m_rightPanel->resetPanel(dir, false);
        invalidate();
    }

    // --- State snapshot (for --test mode) ---
    std::string stateSnapshot() const override;

protected:
    enum class Drag { Idle, Horiz, Vert };  // 'None' clashes with X11's #define None 0L

    int   m_width  { 800 };
    int   m_height { 600 };
    float m_hRatio { 0.5f };
    float m_vRatio { 0.5f };
    Drag  m_drag   { Drag::Idle };

    std::unique_ptr<DirectoryPanel> m_leftPanel;
    std::unique_ptr<DirectoryPanel> m_rightPanel;

    void initPanels(const std::string& startPath)
    {
        m_leftPanel  = std::make_unique<DirectoryPanel>(startPath);
        m_rightPanel = std::make_unique<DirectoryPanel>(startPath);
        m_leftPanel->setFocus(true);
    }

    static float clamp(float v) { return std::clamp(v, 0.1f, 0.9f); }
    static std::string serializeEntries(const DirectoryPanel* panel);
};

} // namespace uc
