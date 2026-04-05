# UniCommander — Claude Code Guide

## Project Overview

Multiplatform File Commander (file manager) application built with Claude Code.
Target platforms: Windows, macOS, Linux.
Language: C/C++.

## Architecture Goals

- Dual-pane file manager modeled after Far Manager, with extensions
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
| DirectoryPanel rendering (path header, entry rows, scroll) | ✓ | ✓ | ✓ |
| Arrow key navigation (Up / Down) | ✓ | ✓ | ✓ |
| Enter to activate (enter dir / go to `..`) | ✓ | ✓ | ✓ |
| Q to quit | — | — | ✓ |
| stdin/stdout test harness (`--test` mode) | ✓ | ✓ | ✓ |

### Not yet implemented

- Unified quit shortcut on Win32 and Cocoa (X11 quits on Q; Win32 and Cocoa do not)
- In-app help page / dialog
- Function key bar (F3 view, F5 copy, F6 move, F7 mkdir, F8 delete, F10 quit)
- File operations: copy, move, delete, rename, create directory
- File viewer (F3)
- Search / filter within a panel
- TerminalPanel (stub only)
- Catch2 unit tests for DirectoryPanel

## Development Plan

Phases are ordered by dependency. Complete each phase before starting the next.

### Phase 1 — Platform parity
Bring Cocoa and X11 up to the same feature level as Win32.

- [x] DirectoryPanel rendering on X11 (path header, entry list, selection highlight, scroll)
- [x] Arrow key navigation + Enter on X11
- [x] DirectoryPanel rendering on Cocoa
- [x] Arrow key navigation + Enter on Cocoa
- [ ] Unified quit shortcut (F10 / Cmd+Q) across all platforms — Escape is NOT a quit key

### Phase 2 — Infrastructure
Foundation required before user-visible features can be built reliably.

- [x] UI test harness (see implementation notes below)
- [ ] In-app help dialog / overlay (keyboard shortcut reference, accessible via F1 or ?)
- [ ] Catch2 unit tests for `DirectoryPanel` (sorting, navigation, scroll, edge cases)

### UI Test Harness — Implementation Notes

**Status: implemented.** See `tests/` directory and `src/test_runner.cpp`.

#### Architecture

The app runs in `--test` mode (same platform window, no display required on CI).
A background thread (`startTestThread` in `src/test_runner.cpp`) reads stdin and
dispatches commands. The Python driver in `tests/driver.py` wraps the subprocess.

```
Python script                    unicommander --test <dir>
─────────────────                ─────────────────────────
                           ←     "ready\n"
send: "keydown down\n"    →      handleKeyDown(Key::Down)
send: "state\n"           →      prints state snapshot
receive: "focus=left ..." ←
assert state["leftSelected"] == 1
```

#### State snapshot format (one line, key=value pairs)
```
focus=left leftSelected=2 rightSelected=0 leftPath=/home/user rightPath=/home/user leftEntries=../ ,foo/,[bar],baz rightEntries=... hRatio=0.5 vRatio=0.5
```
Entry list format: comma-separated names; directories have a trailing `/`.

#### Command set (stdin, one command per line)
```
keydown <key>          # keys: up down return tab escape q f1..f10
state                  # request state snapshot -> written to stdout
reset <dir>            # reset both panels to <dir>, restore default ratios/focus; returns snapshot
quit                   # close app and exit
```

Not yet implemented: `click`, `mousedown`, `mousemove`, `mouseup`, `size`.

#### Files
- `src/test_runner.h` / `src/test_runner.cpp` — `startTestThread(weak_ptr<Window>)`
- `tests/driver.py` — `TestDriver` (protocol) + `TestCase` (test runner base)
- `tests/utils.py` — `entries_for(path)` filesystem helper
- `tests/test_initial_state.py`, `test_focus.py`, `test_navigation.py`, `test_activation.py`, `test_right_panel.py`

### Phase 3 — Far Manager baseline

Implement the standard Far Manager command set. F-key bar is always visible at
the bottom. Esc = cancel (never quit).

#### F-key bar + quit
- [ ] F-key bar rendered at bottom (F1–F10 labels, always visible)
- [ ] F10 — Quit (and Cmd+Q on macOS, Alt+F4 on Windows)

#### Navigation
- [ ] Ctrl+\\ — Go to root of current panel's drive
- [ ] Ctrl+R — Refresh / reload current panel
- [ ] Ctrl+U — Swap left and right panels
- [ ] Alt+F1 / Alt+F2 — Change drive / root for left / right panel
- [ ] Space — Select entry and show directory size (for dirs)
- [ ] Ins — Mark / unmark file (multi-select toggle)
- [ ] `+` / `-` — Select / deselect by wildcard pattern
- [ ] `*` — Invert selection

#### File operations
- [ ] F5 — Copy selected file(s)/dir(s) to opposite panel's directory
- [ ] F6 — Move / Rename selected file(s)/dir(s)
- [ ] F7 — Make directory (prompt for name)
- [ ] F8 — Delete selected file(s)/dir(s) with confirmation prompt

#### Viewers and editors
- [ ] F3 — View file (read-only text viewer, scrollable)
- [ ] F4 — Edit file (launch external editor, configurable)

#### Search
- [ ] Alt+F7 — Find file (name pattern + optional content search)

#### Menus and help
- [ ] F9 — Activate menu bar (Files / Commands / Options)
- [ ] F1 — In-app help overlay (keyboard shortcut reference)
- [ ] F2 — User menu (configurable shortcut list, stored in config file)

#### Status bar
- [ ] Status bar below panels: entry count, selected count, free disk space

### Phase 4 — Extensions

UniCommander-specific additions beyond the Far Manager baseline. Specific
features TBD as the project evolves.

- [ ] Terminal panel — embedded shell in the bottom pane, cwd follows active panel
- [ ] Quick filter — type to filter entries in active panel (incremental search)
- [ ] Configurable color themes
- [ ] Column view option (name / size / date / attributes)

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
