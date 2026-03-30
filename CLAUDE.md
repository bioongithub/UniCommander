# UniCommander — Claude Code Guide

## Project Overview

Multiplatform File Commander (file manager) application built with Claude Code.
Target platforms: Windows, macOS, Linux.
Language: C/C++.

## Architecture Goals

- Dual-pane file manager in the style of classic commanders (Norton Commander, Midnight Commander)
- Cross-platform abstraction layer for filesystem operations
- Native UI per platform or a cross-platform UI toolkit (e.g., Qt, wxWidgets, or ncurses for TUI)

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

## Directory Structure (planned)

```
UniCommander/
├── src/            # Source files
│   ├── core/       # Platform-independent logic (filesystem ops, config)
│   ├── ui/         # UI layer
│   └── platform/   # Platform-specific implementations
├── include/        # Public headers
├── tests/          # Unit and integration tests
├── third_party/    # External dependencies
├── docs/           # Documentation
└── CMakeLists.txt
```

## Testing

Unit tests go in `tests/`. Use a C++ testing framework (e.g., Google Test or Catch2).

```bash
cmake --build build --target tests
ctest --test-dir build
```

## Platform Notes

- **Windows**: MSVC or MinGW/Clang; use Win32 API or abstraction layer
- **macOS**: Clang; use POSIX APIs
- **Linux**: GCC or Clang; use POSIX APIs

## Git Workflow

- Branch from `master` for features: `feature/<name>`
- Keep commits focused and atomic
- Do not commit build artifacts (covered by `.gitignore`)

## Links

- Repository: https://github.com/bioongithub/UniCommander
- License: MIT
