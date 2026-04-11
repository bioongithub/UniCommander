#pragma once

namespace uc {

// Input event types shared across the widget system.

enum class Key {
    Up, Down, Left, Right, Return, Tab, Escape, Q,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10
};

enum class Mod { Alt = 0, Shift = 1, Ctrl = 2 };

} // namespace uc
