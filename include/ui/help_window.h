#pragma once

namespace uc {

// Keyboard-shortcut help overlay.
// Holds visibility state and content; rendering is platform-specific.
// Follows the same pattern as DirectoryPanel: the class owns data and logic,
// platform windows own rendering.
class HelpWindow
{
public:
    // Geometry of the overlay box centered in a W x H client area.
    struct Box { int x, y, w, h; };

    // Help text lines; nullptr-terminated.
    // First entry is the title (platforms may render it in a distinct color).
    static const char* const LINES[];

    static constexpr int LINE_H  = 18;   // pixels per text row
    static constexpr int PADDING = 12;   // inner padding on all sides
    static constexpr int BOX_W   = 340;  // fixed overlay width

    // Returns the bounding box centered in a client area of size W x H.
    static Box computeBox(int W, int H);

    // --- State ---
    bool isVisible() const { return m_visible; }
    void toggle()          { m_visible = !m_visible; }
    void close()           { m_visible = false; }
    void reset()           { m_visible = false; }

private:
    bool m_visible { false };
};

} // namespace uc
