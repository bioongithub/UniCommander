"""
Verify F-key bar behaviour: cell clicks fire the corresponding key action,
and unimplemented keys are no-ops.
"""
import os
import sys
from driver import TestCase, TestDriver

_TESTS_DIR = os.path.dirname(os.path.abspath(__file__))


class FKeyBarTests(TestCase):

    def test_fkeyclick_10_cancel_keeps_alive(self):
        # Clicking F10 cell with dialog=no must leave the app running.
        self.app.dialog("no")
        self.app.send("fkeyclick 10")
        # App must still respond to state query.
        s = self.app.state()
        assert s.get("focus") is not None, "app did not respond after F10 cancel"

    def test_fkeyclick_10_confirm_exits(self):
        # Spawn a dedicated driver so the shared suite process is not affected.
        driver = TestDriver(self._executable, _TESTS_DIR)
        try:
            driver.dialog("yes")
            driver.send("fkeyclick 10")
            driver.proc.wait(timeout=5)
            assert driver.proc.returncode == 0, \
                f"expected exit 0, got {driver.proc.returncode}"
        except Exception:
            driver.proc.kill()
            raise

    def test_fkeyclick_2_through_9_are_noop(self):
        # F1 opens help (tested in test_help.py); F2-F9 have no action yet.
        before = self.app.state()
        for n in range(2, 10):
            self.app.send(f"fkeyclick {n}")
        after = self.app.state()
        for key in ("focus", "leftSelected", "rightSelected",
                    "leftPath", "rightPath"):
            assert after[key] == before[key], \
                f"{key} changed after F2-F9 clicks: {before[key]!r} -> {after[key]!r}"


if __name__ == "__main__":
    ok = FKeyBarTests(sys.argv[1]).run()
    sys.exit(0 if ok else 1)
