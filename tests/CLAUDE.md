# Tests Directory — Claude Code Guide

## Protocol changes require README update

**Rule: any change to the stdin/stdout protocol must be reflected in `tests/README.md`.**

This includes:
- Adding, removing, or renaming a command
- Changing the state snapshot fields or format
- Changing the handshake sequence
- Changing key names accepted by `keydown`

No protocol change is complete until `README.md` is updated.

## Test file conventions

- One test script per feature area: `test_navigation.py`, `test_focus.py`, etc.
- Each script is standalone and runnable directly:
  ```bash
  python tests/test_navigation.py build/Debug/unicommander.exe
  ```
- Use `TestDriver` from `driver.py` — do not spawn subprocesses directly.
- Always call `app.quit()` at the end of a test (use `try/finally`).
- Assert with plain `assert` and a descriptive message:
  ```python
  state = app.state()
  assert state["focus"] == "left", f"expected focus=left, got {state['focus']}"
  ```

## driver.py

`driver.py` is the sole interface between test scripts and the app.
If the protocol changes, update `driver.py` and `README.md` together.
Do not duplicate protocol logic across individual test scripts.
