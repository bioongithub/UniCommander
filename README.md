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

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Arrow Up / Down | Navigate entries in the active panel |
| Enter | Enter directory / go to parent via `..` |
| Tab | Switch focus between left and right panel |
| F10 | Quit (confirmation dialog) |

## F-key bar

A Far Manager-style function key bar is displayed at the bottom of the window.
Each of the 10 cells shows the key number and, for implemented keys, its action.
Clicking a cell fires the same action as pressing the corresponding F-key.

## Modifier key indicators

Three cells (Alt / Sft / Ctl) are shown to the right of the F-key bar.
- Pressing a modifier key highlights its cell while held.
- Clicking a cell toggles its **sticky** state (stays active until clicked again).

Sticky mode is useful in remote desktop sessions where modifier keys may not
pass through reliably.

## Testing

The test suite drives the app as a subprocess via a stdin/stdout text protocol.
See [tests/README.md](tests/README.md) for the protocol reference and how to run tests.

## License

MIT
