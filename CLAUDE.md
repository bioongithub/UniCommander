# UniCommander — Claude Code Guide

## Project Overview

Multiplatform File Commander (file manager) application built with Claude Code.
Target platforms: Windows, macOS, Linux.
Language: C/C++.

## Architecture Goals

- Dual-pane file manager in the style of classic commanders (Norton Commander, Midnight Commander)
- Native UI per platform: Win32 (Windows), Cocoa (macOS), Xlib/X11 (Linux)
- Cross-platform abstraction layer for filesystem operations

## Build System

CMake 3.20+, C++17.

```bash
cmake -B build -S .
cmake --build build
./build/unicommander        # Linux/macOS
build\Debug\unicommander.exe  # Windows (MSVC)
```

## Code Conventions

- C++17 or later
- File and folder names: `snake_case`
- Class names: `PascalCase`
- Functions/methods: `camelCase`
- Constants/macros: `UPPER_SNAKE_CASE`
- Prefer `#pragma once` over include guards
- No raw owning pointers — use smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- ASCII only in all source files — no Unicode box-drawing, arrows, or other non-ASCII characters in comments, strings, or identifiers. Use `// --- Section ---` for section headers.

## UI Class Hierarchy

```
uc::Window          (include/ui/window.h)        — pure abstract interface
  └── uc::BaseWindow (include/ui/base_window.h)  — shared layout + panel state
        ├── Win32Window   (src/platform/windows/) — Win32 API
        ├── CocoaWindow   (src/platform/macos/)   — Cocoa / NSWindow
        └── X11Window     (src/platform/linux/)   — Xlib / X11

uc::Panel           (include/ui/panel.h)          — pure abstract interface
  ├── uc::DirectoryPanel (include/ui/directory_panel.h) — file/dir listing, navigation
  └── uc::TerminalPanel  (include/ui/terminal_panel.h)  — terminal (stub)
```

`uc::BaseWindow` holds all state and logic that is shared across platforms:
- Split ratios (`m_hRatio`, `m_vRatio`) and `Drag` state
- `DIVIDER_W` and `HIT_ZONE` constants
- Layout geometry: `computeLayout(W, H)` → `Layout { topH, leftW }`
- Hit testing: `hitTest(mx, my, W, H)` → `Hit` enum (HorizDivider, VertDivider, LeftPanel, RightPanel, Bottom)
- Panel instances (`m_leftPanel`, `m_rightPanel`), `initPanels()`, `leftPanel()`, `rightPanel()`
- `focusedPanel()`, `switchFocus()`
- Ratio setters `setHRatio()` / `setVRatio()` (include clamping to [0.1, 0.9])

**Rule: do not add data members or logic to platform-specific window classes
(`Win32Window`, `CocoaWindow`, `X11Window`) if the member is not tied to a
platform API. Anything shared across platforms belongs in `uc::BaseWindow`.**

**Rule: if behavior is identical across all platforms, move it to `uc::BaseWindow`
and call it from each platform. When adding or changing any feature, keep all
three platforms up to date — no platform should lag behind the others.**

Each platform provides a `createWindow()` factory; CMake selects the right one.

## Platform Implementation Notes

### Cocoa (`src/platform/macos/cocoa_window.mm`)
- `UCContentView : NSView` holds a `CocoaWindow* _owner` back-pointer (non-owning).
- All layout and hit-testing goes through `_owner->computeLayout()` and `_owner->hitTest()`.
- Do **not** duplicate ratio state in `UCContentView` — use `_owner->hRatio()` / `setHRatio()`.
- Tab key is `keyCode == 48`; `mouseDown:` calls `[self.window makeFirstResponder:self]` first.

### X11 (`src/platform/linux/x11_window.cpp`)
- Avoid naming enum values `None` — conflicts with X11's `#define None 0L`.
- Use `Hit` alias (`using Hit = uc::BaseWindow::Hit;`) at the top of the file.

### Win32 (`src/platform/windows/win32_window.cpp`)
- Use `#define NOMINMAX` before `<windows.h>` to prevent `min`/`max` macro conflicts.
- Use `Hit` alias (`using Hit = uc::BaseWindow::Hit;`) at the top of the file.

## Panel Types

### DirectoryPanel (`include/ui/directory_panel.h`, `src/ui/directory_panel.cpp`)
- Holds current `std::filesystem::path`, a sorted entry list, selection index, scroll offset
- Entries: `..` (if not root) → directories (sorted, case-insensitive) → files (sorted)
- Navigation: `moveUp()`, `moveDown()`, `activate()` (enter dir / go to parent)
- `ensureVisible(int visibleRows)` — adjusts scroll offset; call after navigation with the panel's visible row count
- Rendering is **not** in the panel — platform windows render it using `entries()`, `selectedIndex()`, `scrollOffset()`, `getPath()`

### TerminalPanel (`include/ui/terminal_panel.h`)
- Stub only; implementation deferred

## Layout

Three panels split by two draggable dividers (default 50/50):

```
┌──────────┬──────────┐
│          │          │
│   left   │  right   │  ← DirectoryPanel × 2, split by vertical divider
│          │          │
├──────────┴──────────┤  ← horizontal divider
│                     │
│       bottom        │  ← TerminalPanel (placeholder)
│                     │
└─────────────────────┘
```

Dividers can be dragged; split ratio is preserved on window resize.
Clicking a panel gives it keyboard focus. Tab switches focus between left/right panels.
Arrow keys navigate entries; Enter activates (enters directory or goes up via `..`).

### Layout helpers (in `uc::BaseWindow`)

```cpp
auto [topH, leftW] = computeLayout(W, H);   // pixel positions of dividers
Hit hit = hitTest(mx, my, W, H);             // what's under the cursor/click
```

Use these in all platform event handlers — do not recompute geometry inline.

## Directory Structure

```
UniCommander/
├── src/
│   ├── main.cpp
│   ├── ui/
│   │   └── directory_panel.cpp
│   └── platform/
│       ├── windows/   win32_window.h / .cpp
│       ├── macos/     cocoa_window.h / .mm
│       └── linux/     x11_window.h  / .cpp
├── include/
│   └── ui/
│       ├── window.h           — uc::Window interface + createWindow()
│       ├── base_window.h      — uc::BaseWindow (shared layout + panel state)
│       ├── panel.h            — uc::Panel interface (type, focus)
│       ├── directory_panel.h  — uc::DirectoryPanel
│       └── terminal_panel.h   — uc::TerminalPanel (stub)
├── tests/
├── third_party/
├── docs/
└── CMakeLists.txt
```

## Platform Notes

- **Windows**: MSVC; Win32 API (`GDI`, `WM_PAINT`, `WM_SETCURSOR`); WIN32 subsystem with `/ENTRY:mainCRTStartup`; use `#define NOMINMAX` before `<windows.h>` to prevent `min`/`max` macro conflicts with `std::min`/`std::max`
- **macOS**: Clang; `NSWindow` + custom `UCContentView : NSView` (isFlipped); links `Cocoa.framework`
- **Linux**: GCC or Clang; Xlib (`XFillRectangle`, `XDrawString`); links `X11`
- Avoid naming enum values `None` — conflicts with X11's `#define None 0L`

## Current Features

### Implemented (all platforms unless noted)

| Feature | Win32 | Cocoa | X11 |
|---|:---:|:---:|:---:|
| 3-panel layout (left / right / bottom) | ✓ | ✓ | ✓ |
| Draggable horizontal + vertical dividers | ✓ | ✓ | ✓ |
| Resize-aware layout (ratios preserved) | ✓ | ✓ | ✓ |
| Cursor shape changes on dividers | ✓ | ✓ | ✓ |
| Panel focus via mouse click | ✓ | ✓ | ✓ |
| Tab to switch panel focus | ✓ | ✓ | ✓ |
| DirectoryPanel rendering (path header, entry rows, scroll) | ✓ | — | — |
| Arrow key navigation (Up / Down) | ✓ | — | — |
| Enter to activate (enter dir / go to `..`) | ✓ | — | — |
| Q / Escape to quit | — | — | ✓ |

### Not yet implemented

- DirectoryPanel rendering on Cocoa and X11 (those platforms show placeholder labels)
- Arrow key navigation and Enter on Cocoa and X11
- In-app help page / dialog
- Function key bar (F3 view, F5 copy, F6 move, F7 mkdir, F8 delete, F10 quit)
- File operations: copy, move, delete, rename, create directory
- File viewer (F3)
- Search / filter within a panel
- TerminalPanel (stub only)
- Unit tests (none yet)

## Development Plan

Phases are ordered by dependency. Complete each phase before starting the next.

### Phase 1 — Platform parity
Bring Cocoa and X11 up to the same feature level as Win32.

- [ ] DirectoryPanel rendering on X11 (path header, entry list, selection highlight, scroll)
- [ ] Arrow key navigation + Enter on X11
- [ ] DirectoryPanel rendering on Cocoa
- [ ] Arrow key navigation + Enter on Cocoa
- [ ] Unified quit shortcut (Q / Escape / Cmd+Q) across all platforms

### Phase 2 — Infrastructure
Foundation required before user-visible features can be built reliably.

- [ ] UI test harness (see detailed plan below)
- [ ] In-app help dialog / overlay (keyboard shortcut reference, accessible via F1 or ?)
- [ ] Catch2 unit tests for `DirectoryPanel` (sorting, navigation, scroll, edge cases) — after harness is in place

### UI Test Harness — Detailed Plan

#### Architecture: stdin/stdout protocol

The app runs in headless `--test` mode (no window, no display required).
An external Python script drives it as a subprocess via stdin/stdout pipes.
This keeps all test code outside the app and works identically on all 3 platforms.

```
Python script                    unicommander --test
─────────────────                ───────────────────
send: "keydown down\n"    →      handleKeyDown(Key::Down)
send: "keydown down\n"    →      handleKeyDown(Key::Down)
send: "state\n"           →      print state snapshot
receive: "selected=2 ..." ←
assert state["selected"] == 2
```

Python test example:
```python
app = TestDriver("unicommander")   # wraps subprocess
app.keydown("down")
app.keydown("down")
state = app.state()
assert state["selected"] == 2
assert state["focus"] == "left"

for _ in range(3):
    app.keydown("down")
state = app.state()
assert state["selected"] == 5
```

#### State snapshot format (one line, key=value pairs)
```
focus=left leftSelected=2 rightSelected=0 leftPath=/home/user rightPath=/home/user hRatio=0.5 vRatio=0.5
```

#### Command set (stdin, one command per line)
```
keydown <key>          # keys: up down return tab escape q f1..f10
click <x> <y>          # mouse button down + up at coords
mousedown <x> <y>
mousemove <x> <y>
mouseup <x> <y>
size <w> <h>           # resize the test window
state                  # request state snapshot → written to stdout
quit                   # exit the test runner
```

#### App-side steps

**Step 1 — `Key` enum in `BaseWindow`**
Add to `base_window.h`:
```cpp
enum class Key { Up, Down, Return, Tab, Escape, Q, F1, F2, F3, F4, F5,
                 F6, F7, F8, F9, F10 };
```

**Step 2 — Window size in `BaseWindow`**
Add `m_width`, `m_height` to `BaseWindow` with `setSize(int w, int h)`.
Platforms call `setSize()` on create and on every resize event.
Handle* methods use these stored dimensions instead of querying the OS.

**Step 3 — Virtual `invalidate()` in `BaseWindow`**
```cpp
virtual void invalidate() = 0;
```
- Win32: `InvalidateRect(m_hwnd, nullptr, FALSE)`
- Cocoa: `[contentView setNeedsDisplay:YES]`
- X11: `paint()`
- TestWindow: no-op

**Step 4 — Event handling methods in `BaseWindow`**
Move all event logic out of platform files:
```cpp
void handleKeyDown  (Key key);
void handleMouseDown(int x, int y);
void handleMouseMove(int x, int y);
void handleMouseUp  (int x, int y);
```
Platform-specific concerns that stay in platform code:
- Win32: `SetCapture` / `ReleaseCapture` around drag
- Cocoa: `makeFirstResponder` before `handleMouseDown`
- X11: cursor shape update after `handleMouseMove`
- All rendering stays in platform code

**Step 5 — Refactor Win32**
- `WM_SIZE`        → `setSize(LOWORD(lp), HIWORD(lp))`
- `WM_KEYDOWN`     → translate `wp` → `Key`, call `handleKeyDown(key)`
- `WM_LBUTTONDOWN` → `handleMouseDown(mx, my)`, then `SetCapture` if dragging
- `WM_MOUSEMOVE`   → `handleMouseMove(mx, my)`
- `WM_LBUTTONUP`   → `handleMouseUp(mx, my)`, then `ReleaseCapture`

**Step 6 — Refactor Cocoa**
- `setFrameSize:` / `viewDidEndLiveResize` → `setSize(w, h)`
- `keyDown:`      → translate `keyCode` → `Key`, call `handleKeyDown(key)`
- `mouseDown:`    → `makeFirstResponder`, then `handleMouseDown(x, y)`
- `mouseDragged:` → `handleMouseMove(x, y)`
- `mouseUp:`      → `handleMouseUp(x, y)`

**Step 7 — Refactor X11**
- `ConfigureNotify` → `setSize(w, h)`
- `KeyPress`        → translate keysym → `Key`, call `handleKeyDown(key)`
- `ButtonPress`     → `handleMouseDown(x, y)`
- `MotionNotify`    → `handleMouseMove(x, y)`, then update cursor (platform-only)
- `ButtonRelease`   → `handleMouseUp(x, y)`

**Step 8 — `TestWindow` class**
Add `tests/test_window.h`:
- Inherits `BaseWindow`
- `create()` calls `initPanels(tempPath)` and `setSize(800, 600)`, no OS calls
- `show()`, `run()`, `close()` — all no-ops
- `invalidate()` — no-op
- `stateSnapshot()` — returns the state string for stdout

**Step 9 — `--test` mode in `main.cpp`**
```cpp
if (argc > 1 && std::string(argv[1]) == "--test") {
    TestWindow win;
    win.create("", 800, 600);
    runTestLoop(win);   // reads stdin, dispatches commands, writes state
    return 0;
}
```
`runTestLoop` is ~50 lines in `src/test_runner.cpp` (not compiled in normal builds).

**Step 10 — Python `TestDriver` helper**
Add `tests/driver.py`:
- Wraps `subprocess.Popen`
- Methods: `keydown(key)`, `click(x, y)`, `size(w, h)`, `state()` → dict, `quit()`
- `state()` sends `"state\n"` and parses the response line into a dict

**Step 11 — Initial Python test scripts**
Add `tests/test_navigation.py`, `tests/test_focus.py`, etc.:
- Navigation: Up/Down moves selection, clamps at bounds
- Enter: activates directory, path changes; `..` goes to parent
- Tab: switches focus between panels
- Click: panel focus follows mouse
- Hit testing: click on known divider coords, check drag state via hRatio change

### Phase 3 — Core file manager features
Classic commander functionality.

- [ ] Function key bar rendered at the bottom of the window (F1–F10 labels)
- [ ] F5 Copy — copy selected file/dir to the opposite panel's directory
- [ ] F6 Move / Rename — move or rename selected file/dir
- [ ] F7 Make directory — prompt for name, create in current panel's directory
- [ ] F8 Delete — confirm-prompt then delete selected file/dir
- [ ] F10 Quit — close the application

### Phase 4 — Usability improvements

- [ ] F3 File viewer — read-only text viewer for selected file
- [ ] Search / filter — type to filter entries in the active panel
- [ ] Multi-selection (Ins key or Space to mark entries)
- [ ] Status bar — show entry count, selected count, free disk space

### Phase 5 — Terminal panel

- [ ] Implement `TerminalPanel` — embedded terminal in the bottom panel
- [ ] Hook shell working directory to the focused directory panel's path

## Feature Development Rule

**When adding any new function or feature, always:**
1. Update `README.md` to document the new functionality.
2. Add or update the in-app help page to reflect the new feature.
3. Add tests in `tests/` covering the new behavior.

No feature is complete until all three are done.

## Test Protocol Rule

**Any change to the stdin/stdout test protocol must be reflected in `tests/README.md`.**

This includes new commands, removed commands, changed state snapshot fields,
changed key names, or changes to the handshake. See `tests/CLAUDE.md` for
the full rule set governing the test directory.

## Testing

Unit tests go in `tests/`. Use a C++ testing framework (e.g., Google Test or Catch2).

```bash
cmake --build build --target tests
ctest --test-dir build
```

## Git Workflow

- Branch from `master` for features: `feature/<name>`
- Keep commits focused and atomic
- Do not commit build artifacts (covered by `.gitignore`)

## Links

- Repository: https://github.com/bioongithub/UniCommander
- License: MIT
