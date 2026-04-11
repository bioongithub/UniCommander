#pragma once
#include "ui/window.h"
#include "ui/events.h"
#include "ui/directory_panel_widget.h"
#include "ui/fkey_bar_widget.h"
#include "ui/terminal_panel_widget.h"
#include "ui/help_widget.h"
#include "ui/confirm_widget.h"
#include <algorithm>
#include <atomic>
#include <functional>
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

    // Labels for F1-F10 slots per modifier row.
    // Row 0 = Normal, 1 = Shift, 2 = Ctrl, 3 = Alt.
    // Empty string means not yet implemented for that combination.
    static const char* const FKEY_LABELS[4][10];

    // Returns the label row index matching the highest-priority active modifier.
    // Priority: Alt(3) > Ctrl(2) > Shift(1) > Normal(0) -- Far Manager convention.
    int activeModifierRow() const
    {
        if (modifierActive(Mod::Alt))   return 3;
        if (modifierActive(Mod::Ctrl))  return 2;
        if (modifierActive(Mod::Shift)) return 1;
        return 0;
    }

    // --- Modifier key indicators ---
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

    // --- Key / Mod aliases (defined in events.h) ---
    using Key = uc::Key;
    using Mod = uc::Mod;

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

    // --- Modal widgets ---
    HelpWidget&    helpWidget()    { return m_helpWidget; }
    ConfirmWidget& confirmWidget() { return m_confirmWidget; }

    // --- Panel widget access ---
    DirectoryPanelWidget* leftWidget()  const { return m_leftWidget.get(); }
    DirectoryPanelWidget* rightWidget() const { return m_rightWidget.get(); }
    TerminalPanelWidget*  termWidget()  const { return m_termWidget.get(); }
    FKeyBarWidget*        fkeyWidget()  const { return m_fkeyWidget.get(); }

    // --- Panel data access (convenience; goes through the widget) ---
    DirectoryPanel* leftPanel()  const
    {
        return m_leftWidget ? &m_leftWidget->panel() : nullptr;
    }
    DirectoryPanel* rightPanel() const
    {
        return m_rightWidget ? &m_rightWidget->panel() : nullptr;
    }

    DirectoryPanel* focusedPanel() const
    {
        if (m_leftWidget  && m_leftWidget->panel().hasFocus())  return &m_leftWidget->panel();
        if (m_rightWidget && m_rightWidget->panel().hasFocus()) return &m_rightWidget->panel();
        return nullptr;
    }

    void switchFocus()
    {
        if (!m_leftWidget || !m_rightWidget) return;
        bool leftWasFocused = m_leftWidget->panel().hasFocus();
        m_leftWidget->panel().setFocus(!leftWasFocused);
        m_rightWidget->panel().setFocus(leftWasFocused);
    }

    // --- Widget layout (call before painting) ---
    // Positions all panel/fkey/terminal widgets within the given window size.
    void layoutPanels(int W, int H)
    {
        const int effH = H - FKEY_H;
        auto [topH, leftW] = computeLayout(W, effH);
        if (m_leftWidget)
            m_leftWidget->layout(0, 0, leftW, topH);
        if (m_rightWidget)
            m_rightWidget->layout(leftW + DIVIDER_W, 0, W - leftW - DIVIDER_W, topH);
        if (m_termWidget)
            m_termWidget->layout(0, topH + DIVIDER_W, W, effH - topH - DIVIDER_W);
        if (m_fkeyWidget)
            m_fkeyWidget->layout(0, effH, W, FKEY_H);
    }

    // --- Platform interface ---
    virtual void invalidate() = 0;   // schedule a repaint
    // Pump native events until done() returns true (used by modal message loops).
    virtual void pumpEventsUntil(std::function<bool()> done) = 0;

    // Confirmation dialogs -- implemented in base_window.cpp using ConfirmWidget.
    bool confirmQuit();
    bool confirmCopy(const std::string& srcName, const std::string& dstPath);

    void handleKeyDown(Key key);

    // Dispatch a key event on the platform's main thread.
    // Default calls handleKeyDown() directly; Win32 overrides to use SendMessage.
    virtual void scheduleKeyDown(Key key) { handleKeyDown(key); }

    bool isClosing() const { return m_closing; }

    // --- Test mode dialog pre-arming ---
    void setDialogAnswer(bool answer) { m_testDialogAnswer = answer; }

    // --- State reset (for --test mode) ---
    void resetState(const std::string& dir)
    {
        m_hRatio = 0.5f;
        m_vRatio = 0.5f;
        m_drag   = Drag::Idle;
        m_modSticky[0]   = m_modSticky[1]   = m_modSticky[2]   = false;
        m_modPhysical[0] = m_modPhysical[1] = m_modPhysical[2] = false;
        m_helpWidget.reset();
        if (m_leftWidget)  m_leftWidget->panel().resetPanel(dir, true);
        if (m_rightWidget) m_rightWidget->panel().resetPanel(dir, false);
        invalidate();
    }

    // --- State snapshot (for --test mode) ---
    std::string stateSnapshot() const override;

protected:
    enum class Drag { Idle, Horiz, Vert };  // 'None' clashes with X11's #define None 0L

    int   m_width   { 800 };
    int   m_height  { 600 };
    // Written by test thread (close()), read by main thread and test thread.
    // Must be atomic to prevent data races.
    std::atomic<bool> m_closing { false };
    std::optional<bool> m_testDialogAnswer;
    float m_hRatio { 0.5f };
    float m_vRatio { 0.5f };
    Drag  m_drag   { Drag::Idle };

    bool m_modSticky[3]   {};   // toggled by clicking the modifier cell
    bool m_modPhysical[3] {};   // true while the physical key is held

    HelpWidget    m_helpWidget;
    ConfirmWidget m_confirmWidget;

    std::shared_ptr<DirectoryPanelWidget> m_leftWidget;
    std::shared_ptr<DirectoryPanelWidget> m_rightWidget;
    std::shared_ptr<TerminalPanelWidget>  m_termWidget;
    std::shared_ptr<FKeyBarWidget>        m_fkeyWidget;

    void initPanels(const std::string& startPath)
    {
        m_leftWidget  = std::make_shared<DirectoryPanelWidget>(startPath);
        m_rightWidget = std::make_shared<DirectoryPanelWidget>(startPath);
        m_termWidget  = std::make_shared<TerminalPanelWidget>();
        m_fkeyWidget  = std::make_shared<FKeyBarWidget>();
        m_leftWidget->panel().setFocus(true);
        // Wire F-key bar callbacks
        m_fkeyWidget->setLabelsFn([](int row, int col) { return FKEY_LABELS[row][col]; });
        m_fkeyWidget->setActiveRowFn([this]() { return activeModifierRow(); });
        m_fkeyWidget->setModActiveFn([this](int i) {
            return modifierActive(static_cast<Mod>(i));
        });
        m_fkeyWidget->setInvalidateCallback([this]() { invalidate(); });
        m_leftWidget->setInvalidateCallback([this]()  { invalidate(); });
        m_rightWidget->setInvalidateCallback([this]() { invalidate(); });
        m_termWidget->setInvalidateCallback([this]()  { invalidate(); });
        // Wire modal widgets
        m_helpWidget.setInvalidateCallback([this]()    { invalidate(); });
        m_confirmWidget.setInvalidateCallback([this]() { invalidate(); });
        m_confirmWidget.setPump([this](auto done) { pumpEventsUntil(done); });
    }

    void handleCopy();   // implements F5: copy selected entry to other panel
    static float clamp(float v) { return std::clamp(v, 0.1f, 0.9f); }
    static std::string serializeEntries(const DirectoryPanel* panel);
};

} // namespace uc
