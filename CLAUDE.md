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
  └── uc::BaseWindow (include/ui/base_window.h)  — shared layout state
        ├── Win32Window   (src/platform/windows/) — Win32 API
        ├── CocoaWindow   (src/platform/macos/)   — Cocoa / NSWindow
        └── X11Window     (src/platform/linux/)   — Xlib / X11

uc::Panel           (include/ui/panel.h)          — pure abstract interface
```

`uc::BaseWindow` holds: split ratios (`m_hRatio`, `m_vRatio`), `Drag` state,
`DIVIDER_W` and `HIT_ZONE` constants.

Each platform provides a `createWindow()` factory; CMake selects the right one.

## Layout

Three panels split by two draggable dividers (default 50/50):

```
┌──────────┬──────────┐
│          │          │
│   left   │  right   │  ← top half, split by vertical divider
│          │          │
├──────────┴──────────┤  ← horizontal divider
│                     │
│       bottom        │
│                     │
└─────────────────────┘
```

Dividers can be dragged; split ratio is preserved on window resize.

## Directory Structure

```
UniCommander/
├── src/
│   ├── main.cpp
│   └── platform/
│       ├── windows/   win32_window.h / .cpp
│       ├── macos/     cocoa_window.h / .mm
│       └── linux/     x11_window.h  / .cpp
├── include/
│   └── ui/
│       ├── window.h       — uc::Window interface + createWindow()
│       ├── base_window.h  — uc::BaseWindow (shared layout state)
│       └── panel.h        — uc::Panel interface
├── tests/
├── third_party/
├── docs/
└── CMakeLists.txt
```

## Platform Notes

- **Windows**: MSVC; Win32 API (`GDI`, `WM_PAINT`, `WM_SETCURSOR`); WIN32 subsystem with `/ENTRY:mainCRTStartup`
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
