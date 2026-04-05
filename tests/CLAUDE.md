# Tests Directory — Claude Code Guide

## Protocol changes require README update

**Rule: any change to the stdin/stdout protocol must be reflected in `tests/README.md`.**

This includes:
- Adding, removing, or renaming a command
- Changing the state snapshot fields or format
- Changing the handshake sequence
- Changing key names accepted by `keydown`

No protocol change is complete until `README.md` is updated.

## Test isolation

All test suites share one app process owned by `_run_all`. `TestDriver` owns the
process (creates it, quits it). `TestCase.run(app)` takes the driver as an
argument -- it never creates or destroys it. Before each test method the driver
sends `reset <dir>` to restore the initial panel state (both panels pointing to
the tests directory, left focus, selection at 0, default ratios). Tests must not
assume any state left by a previous test.

## Test file conventions

- One test script per feature area: `test_navigation.py`, `test_focus.py`, etc.
- Each script subclasses `TestCase` and is standalone and runnable directly:
  ```bash
  python tests/test_navigation.py build/Debug/unicommander.exe
  ```
- `TestCase` owns the driver and calls `app.quit()` automatically in `run()`.
- Assert with plain `assert` and a descriptive message:
  ```python
  s = self.app.state()
  assert s["focus"] == "left", f"expected left, got {s['focus']}"
  ```
- Exit with the return value of `run()` so CI can detect failures:
  ```python
  if __name__ == "__main__":
      ok = MyTests(sys.argv[1]).run()
      sys.exit(0 if ok else 1)
  ```

## driver.py

`driver.py` is the sole interface between test scripts and the app.
It provides two classes: `TestDriver` (protocol layer) and `TestCase` (test runner base).
If the protocol changes, update `driver.py` and `README.md` together.
Do not duplicate protocol logic across individual test scripts.
