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
| `fkeyclick <n>` | Simulate a click on F-key bar cell n (1-10); fires same action as `keydown f<n>` |
| `modclick <mod>` | Toggle sticky state of a modifier indicator; mod: `alt`, `shift`, `ctrl` |
| `dialog yes\|no` | Pre-arm the next confirmation dialog; `confirmQuit()` returns the answer immediately without showing a native dialog |
| `reset <dir>` | Reinitialise both panels to `<dir>`, restore default state; responds with a state snapshot |
| `setpath left\|right <path>` | Set one panel's directory to `<path>` without affecting the other; responds with a state snapshot |
| `state` | Request a state snapshot (see below) |
| `quit` | Emergency close — bypasses confirmation dialog; use F10 for normal exit |
| `click <x> <y>` | Mouse button down + up at pixel coords *(not yet implemented)* |
| `mousedown <x> <y>` | Mouse button down *(not yet implemented)* |
| `mousemove <x> <y>` | Mouse move (drag) *(not yet implemented)* |
| `mouseup <x> <y>` | Mouse button up *(not yet implemented)* |
| `size <w> <h>` | Resize the window to w×h pixels *(not yet implemented)* |

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
| `modAlt` | `0` \| `1` | Sticky Alt modifier state |
| `modShift` | `0` \| `1` | Sticky Shift modifier state |
| `modCtrl` | `0` \| `1` | Sticky Ctrl modifier state |
| `helpVisible` | `0` \| `1` | Whether the F1 help overlay is currently shown |

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
app.dialog("yes")                                         # pre-arm next confirmation dialog
state = app.state()                                       # sends "state", returns dict[str, str]
app.quit()                                                # emergency: sends "quit", waits for process exit
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
