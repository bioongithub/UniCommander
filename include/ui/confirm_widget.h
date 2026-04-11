#pragma once
#include "ui/modal_widget.h"
#include <string>

namespace uc {

// Yes/No confirmation overlay. show() blocks until the user answers.
// After show() returns, result() gives the answer.
//
// Keys: Y / Enter (when Yes focused) -> true
//       N / Escape                   -> false
//       Tab / Left / Right           -> toggle button focus

class ConfirmWidget : public ModalWidget
{
public:
    static constexpr int BOX_W = 380;
    static constexpr int BOX_H = 130;

    ConfirmWidget();

    // Blocking: returns only when user answers Yes or No.
    void show(const std::string& line1, const std::string& line2 = "");

    bool result() const { return m_result; }

    bool handleKeyDown(Key key)        override;
    bool handleMouseDown(int x, int y) override;
    void paint(RenderContext& ctx)     override;

private:
    std::string m_line1;
    std::string m_line2;
    int  m_focusedButton { 1 };  // 0 = Yes, 1 = No (default No -- safer)
    bool m_result        { false };

    // Button geometry derived from widget bounds (set by layout()).
    static constexpr int BTN_W = 80;
    static constexpr int BTN_H = 28;
    int btnY() const { return m_y + m_h - BTN_H - 12; }
    int yesX() const { return m_x + m_w / 2 - BTN_W - 10; }
    int noX()  const { return m_x + m_w / 2 + 10; }
};

} // namespace uc
