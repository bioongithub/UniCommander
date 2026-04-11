#pragma once
#include "ui/widget.h"
#include <functional>

namespace uc {

// Widget that renders the F-key bar at the bottom of the window.
//
// The labels and modifier state are supplied via callbacks set by the owner
// (BaseWindow) so that the widget has no dependency on the platform classes.
//
// Layout: SizePolicy::Fixed (full width, FKEY_H tall) -- the owner positions
// it via setFixedBounds() or by setting m_x/m_y/m_w/m_h directly.

class FKeyBarWidget : public Widget
{
public:
    static constexpr int FKEY_H      = 20;
    static constexpr int NUM_W       = 16;   // width of the key-number sub-column
    static constexpr int MOD_CELL_W  = 30;   // width of each modifier indicator
    static constexpr int MOD_AREA_W  = MOD_CELL_W * 3;

    FKeyBarWidget();

    // --- Callbacks wired by BaseWindow ---

    // Returns the label for F-key slot [row][col] (row 0-3, col 0-9).
    void setLabelsFn(std::function<const char*(int row, int col)> fn);

    // Returns the active modifier row index (0=Normal, 1=Shift, 2=Ctrl, 3=Alt).
    void setActiveRowFn(std::function<int()> fn);

    // Returns true if the modifier with the given index (0=Alt,1=Sft,2=Ctl) is active.
    void setModActiveFn(std::function<bool(int)> fn);

    void paint(RenderContext& ctx) override;

private:
    std::function<const char*(int, int)> m_labelsFn;
    std::function<int()>                 m_activeRowFn;
    std::function<bool(int)>             m_modActiveFn;

    static const char* const MOD_LABELS[3];
};

} // namespace uc
