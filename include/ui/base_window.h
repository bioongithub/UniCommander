#pragma once
#include "ui/window.h"
#include "ui/directory_panel.h"
#include <algorithm>
#include <atomic>
#include <memory>
#include <optional>
#include <string>

namespace uc {

// Intermediate base that holds layout state shared by all platform windows.
// Platform classes inherit from this instead of Window directly.
class BaseWindow : public Window
{
public:
    static constexpr int DIVIDER_W   = 5;
    static constexpr int HIT_ZONE    = 4;
    static constexpr int FKEY_H      = 20;  // height of the F-key bar
    static constexpr int MOD_CELL_W  = 30;  // width of each modifier indicator cell
    static constexpr int MOD_AREA_W  = MOD_CELL_W * 3;

    // Labels for F1-F10 slots; empty string = not yet implemented.
    static const char* const FKEY_LABELS[10];

    // --- Modifier key indicators ---
    enum class Mod { Alt = 0, Shift = 1, Ctrl = 2 };

    void setModifierPhysical(Mod m, bool down)
    {
        m_modPhysical[static_cast<int>(m)] = down;
    }
    void toggleModifierSticky(Mod m)
    {
        int i = static_cast<int>(m);
        m_modSticky[i] = !m_modSticky[i];
    }
    bool modifierActive(Mod m) const
    {
        int i = static_cast<int>(m);
        return m_modPhysical[i] || m_modSticky[i];
    }

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
    virtual void invalidate()   = 0;  // platform: schedule a repaint
    virtual bool confirmQuit()  = 0;  // platform: show confirmation dialog, true = quit
    void handleKeyDown(Key key);

    // Dispatch a key event so that handleKeyDown() runs on the platform's main
    // thread.  This eliminates data races between the test thread and the render
    // thread that accesses the same panel state.
    //
    // Default implementation calls handleKeyDown() directly (safe for X11 and
    // Cocoa where there is no separate render thread).  Win32 overrides this to
    // use SendMessage so the call is serialised with WM_PAINT.
    virtual void scheduleKeyDown(Key key) { handleKeyDown(key); }

    bool isClosing() const { return m_closing; }

    // --- Test mode dialog pre-arming ---
    // Call setDialogAnswer() before the action that triggers confirmQuit().
    // confirmQuit() will consume the answer and return immediately (no dialog shown).
    void setDialogAnswer(bool answer) { m_testDialogAnswer = answer; }

    // --- State reset (for --test mode) ---
    void resetState(const std::string& dir)
    {
        m_hRatio = 0.5f;
        m_vRatio = 0.5f;
        m_drag   = Drag::Idle;
        m_modSticky[0]   = m_modSticky[1]   = m_modSticky[2]   = false;
        m_modPhysical[0] = m_modPhysical[1] = m_modPhysical[2] = false;
        if (m_leftPanel)  m_leftPanel->resetPanel(dir, true);
        if (m_rightPanel) m_rightPanel->resetPanel(dir, false);
        invalidate();
    }

    // --- State snapshot (for --test mode) ---
    std::string stateSnapshot() const override;

protected:
    enum class Drag { Idle, Horiz, Vert };  // 'None' clashes with X11's #define None 0L

    int   m_width   { 800 };
    int   m_height  { 600 };
    // Written by test thread (close()), read by main thread (WM_CLOSE handler)
    // and test thread (isClosing()). Must be atomic to prevent C++ data race
    // and ensure the main thread sees the updated value before processing WM_CLOSE.
    std::atomic<bool> m_closing { false };
    std::optional<bool> m_testDialogAnswer; // pre-armed answer for test mode
    float m_hRatio { 0.5f };
    float m_vRatio { 0.5f };
    Drag  m_drag   { Drag::Idle };

    bool m_modSticky[3]   {};   // toggled by clicking the modifier cell
    bool m_modPhysical[3] {};   // true while the physical key is held

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
