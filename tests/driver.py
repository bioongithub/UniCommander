"""
Main test runner. Explicitly lists and runs all test suites.

Usage:
    python tests/driver.py build/Debug/unicommander.exe
"""
import os
import subprocess
import sys

_TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _TESTS_DIR)

from utils import entries_for


# --- Protocol driver ---

class TestDriver:
    """Low-level stdin/stdout protocol driver. Wraps the app subprocess."""

    def __init__(self, executable, initial_dir):
        self._initial_dir = initial_dir
        self.proc = subprocess.Popen(
            [os.path.abspath(executable), "--test", initial_dir],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        line = self.proc.stdout.readline().strip()
        if line != "ready":
            self.proc.kill()
            raise RuntimeError(f"Expected 'ready', got: {line!r}")

    def send(self, command):
        self.proc.stdin.write(command + "\n")
        self.proc.stdin.flush()

    def reset(self):
        self.send(f"reset {self._initial_dir}")
        line = self.proc.stdout.readline().strip()
        state = dict(pair.split("=", 1) for pair in line.split())
        assert state.get("focus")         == "left",              f"reset: focus={state.get('focus')!r}"
        assert state.get("leftSelected")  == "0",                 f"reset: leftSelected={state.get('leftSelected')!r}"
        assert state.get("rightSelected") == "0",                 f"reset: rightSelected={state.get('rightSelected')!r}"
        assert state.get("leftPath")      == self._initial_dir,   f"reset: leftPath={state.get('leftPath')!r}"
        assert state.get("rightPath")     == self._initial_dir,   f"reset: rightPath={state.get('rightPath')!r}"
        assert state.get("hRatio")        == "0.5",               f"reset: hRatio={state.get('hRatio')!r}"
        assert state.get("vRatio")        == "0.5",               f"reset: vRatio={state.get('vRatio')!r}"
        assert state.get("modAlt")        == "0",                 f"reset: modAlt={state.get('modAlt')!r}"
        assert state.get("modShift")      == "0",                 f"reset: modShift={state.get('modShift')!r}"
        assert state.get("modCtrl")       == "0",                 f"reset: modCtrl={state.get('modCtrl')!r}"

    def dialog(self, answer):
        """Pre-arm the next confirmation dialog. answer: 'yes' or 'no'."""
        self.send(f"dialog {answer}")

    def state(self):
        self.send("state")
        line = self.proc.stdout.readline().strip()
        return dict(pair.split("=", 1) for pair in line.split())

    def quit(self):
        self.send("quit")
        self.proc.wait(timeout=5)
        if self.proc.returncode != 0:
            raise RuntimeError(f"App exited with code {self.proc.returncode}")


# --- Test base class ---

class TestCase:
    """
    Base class for test suites. Subclass and add test_* methods.

    run(app) takes a TestDriver owned by the caller. Before each test the
    driver sends 'reset' to restore initial state. Tests run in alphabetical
    order; run() returns True if all passed.

    Example:
        class MyTests(TestCase):
            def test_something(self):
                self.assertState({"focus": "left"})
    """

    def __init__(self, executable):
        self._executable = executable
        self.app = None
        self._passed = 0
        self._failed = 0

    def assertState(self, expected):
        """Fetch current state and assert every key in `expected` matches.
        Panel entries are always verified against the actual filesystem."""
        actual = self.app.state()
        full_expected = {
            "leftEntries":  entries_for(actual["leftPath"]),
            "rightEntries": entries_for(actual["rightPath"]),
        }
        full_expected.update(expected)
        for key, value in full_expected.items():
            assert actual.get(key) == value, (
                f"{key}: expected {value!r}, got {actual.get(key)!r}"
            )

    def run(self, app):
        tests = sorted(
            name for name in dir(self)
            if name.startswith("test_") and callable(getattr(self, name))
        )
        suite = type(self).__name__
        print(f"\n[{suite}] {len(tests)} test(s)")
        self.app = app
        for name in tests:
            self.app.reset()
            try:
                getattr(self, name)()
                print(f"  PASS  {name}")
                self._passed += 1
            except AssertionError as e:
                print(f"  FAIL  {name}: {e}")
                self._failed += 1
            except Exception as e:
                print(f"  ERROR {name}: {type(e).__name__}: {e}")
                self._failed += 1
        self.app = None
        return self._failed == 0


# --- Runner ---

def _run_all(executable):
    from test_initial_state import InitialStateTests
    from test_focus import FocusTests
    from test_navigation import NavigationTests
    from test_activation import ActivationTests
    from test_right_panel import RightPanelTests
    from test_fkey_bar import FKeyBarTests
    from test_modifiers import ModifierTests

    suites = [
        InitialStateTests,
        FocusTests,
        NavigationTests,
        ActivationTests,
        RightPanelTests,
        FKeyBarTests,
        ModifierTests,
    ]

    app = TestDriver(executable, _TESTS_DIR)
    total_passed = total_failed = 0
    try:
        for cls in suites:
            instance = cls(executable)
            instance.run(app)
            total_passed += instance._passed
            total_failed += instance._failed
    except Exception:
        app.proc.kill()
        raise

    print(f"\n{'='*40}")
    print(f"Total: {total_passed} passed, {total_failed} failed")

    # --- Normal exit via F10 (also verifies cancel then confirm) ---
    # Step 1: cancel -- app must stay alive
    app.dialog("no")
    app.send("keydown f10")
    try:
        app.state()
        print("  PASS  teardown: F10 cancel keeps app alive")
        total_passed += 1
    except Exception as e:
        print(f"  FAIL  teardown: F10 cancel keeps app alive: {e}")
        total_failed += 1

    

    # Step 2: confirm -- app must exit cleanly
    app.dialog("yes")
    app.send("keydown f10")
    try:
        app.proc.wait(timeout=5)
        if app.proc.returncode == 0:
            print("  PASS  teardown: F10 confirm exits zero")
            total_passed += 1
        else:
            print(f"  FAIL  teardown: F10 confirm exits zero: code {app.proc.returncode}")
            total_failed += 1
    except Exception as e:
        print(f"  FAIL  teardown: F10 confirm exits zero: {e}")
        total_failed += 1
        app.proc.kill()

    print(f"\n{'='*40}")
    print(f"Grand total: {total_passed} passed, {total_failed} failed")
    return total_failed == 0


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} <path/to/unicommander>")
        sys.exit(1)
    ok = _run_all(sys.argv[1])
    sys.exit(0 if ok else 1)
