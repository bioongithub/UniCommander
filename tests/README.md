# UniCommander Test Suite

## Overview

Tests drive the application as a subprocess via a stdin/stdout text protocol.
The app must be launched with `--test`; a real window is shown and all input is
dispatched as native OS events by a background thread.

## Running tests

```bash
python tests/test_navigation.py build/Debug/unicommander.exe   # Windows
python tests/test_navigation.py build/unicommander              # Linux/macOS
```

`driver.py` provides the `TestDriver` helper used by all test scripts.

---

## Protocol

### Handshake

After the app starts, the background thread prints exactly:

```
ready
```

The driver waits for this line before sending any commands.

### Commands (stdin, one per line)

| Command | Description |
|---|---|
| `keydown <key>` | Simulate a key press |
| `reset <dir>` | Reinitialise both panels to `<dir>`, restore default state; responds with a state snapshot |
| `click <x> <y>` | Mouse button down + up at pixel coords |
| `mousedown <x> <y>` | Mouse button down |
| `mousemove <x> <y>` | Mouse move (drag) |
| `mouseup <x> <y>` | Mouse button up |
| `size <w> <h>` | Resize the window to w×h pixels |
| `state` | Request a state snapshot (see below) |
| `quit` | Close the app and exit |

#### Key names for `keydown`

`up`, `down`, `return`, `tab`, `escape`, `q`,
`f1`, `f2`, `f3`, `f4`, `f5`, `f6`, `f7`, `f8`, `f9`, `f10`

### State snapshot (stdout, one line)

Sent in response to the `state` command:

```
focus=left leftSelected=2 rightSelected=0 leftPath=/home/user rightPath=/home/user leftEntries=..,src/,include/,README.md rightEntries=..,src/,include/,README.md hRatio=0.5 vRatio=0.5
```

Fields are space-separated `key=value` pairs. All fields are always present.

| Field | Type | Description |
|---|---|---|
| `focus` | `left` \| `right` | Which panel has keyboard focus |
| `leftSelected` | int | Zero-based selection index in the left panel |
| `rightSelected` | int | Zero-based selection index in the right panel |
| `leftPath` | string | Absolute path shown in the left panel |
| `rightPath` | string | Absolute path shown in the right panel |
| `leftEntries` | string | Comma-separated entry names; directories have a trailing `/` |
| `rightEntries` | string | Comma-separated entry names; directories have a trailing `/` |
| `hRatio` | float | Horizontal split ratio (0.0–1.0) |
| `vRatio` | float | Vertical split ratio (0.0–1.0) |

### Error responses

Unknown commands produce:

```
error unknown: <original line>
```

---

## Python API (`driver.py`)

### `TestDriver` — low-level protocol wrapper

```python
from driver import TestDriver

app = TestDriver("path/to/unicommander", "/initial/dir")  # spawns process, waits for "ready"
app.send("keydown down")                                  # raw command
app.reset()                                               # sends "reset <initial_dir>"
state = app.state()                                       # sends "state", returns dict[str, str]
app.quit()                                                # sends "quit", waits for process exit
```

### `TestCase` — base class for test suites

Subclass `TestCase`, add `test_*` methods, call `run()`. Tests run in
alphabetical order; `run()` calls `app.quit()` at the end and returns
`True` if all tests passed.

```python
from driver import TestCase
import sys

class NavigationTests(TestCase):
    def test_initial_focus(self):
        s = self.app.state()
        assert s["focus"] == "left", f"expected left, got {s['focus']}"

    def test_move_down(self):
        self.app.send("keydown down")
        s = self.app.state()
        assert s["leftSelected"] == "1", f"expected 1, got {s['leftSelected']}"

if __name__ == "__main__":
    ok = NavigationTests(sys.argv[1]).run()
    sys.exit(0 if ok else 1)
```
