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

`uc::BaseWindow` holds all state that is shared across platforms:
- Split ratios (`m_hRatio`, `m_vRatio`) and `Drag` state
- `DIVIDER_W` and `HIT_ZONE` constants
- Panel instances (`m_leftPanel`, `m_rightPanel`) and `initPanels()`
- `focusedPanel()` and `switchFocus()` logic

**Rule: do not add data members or logic to platform-specific window classes
(`Win32Window`, `CocoaWindow`, `X11Window`) if the member is not tied to a
platform API. Anything shared across platforms belongs in `uc::BaseWindow`.**

Each platform provides a `createWindow()` factory; CMake selects the right one.

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
