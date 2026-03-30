# UniCommander

A multiplatform File Commander application built with Claude Code.
Target platforms: Windows, macOS, Linux.

## Requirements

- CMake 3.20+
- C++17-capable compiler (MSVC, GCC, Clang)

## Build

```bash
cmake -B build -S .
cmake --build build
```

## Run

```bash
# Linux / macOS
./build/unicommander

# Windows (MSVC)
build\Debug\unicommander.exe
```

## License

MIT
