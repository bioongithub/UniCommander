"""
Verify help overlay behaviour: F1 toggles it, Escape closes it,
and it does not affect panel navigation state.
"""
import sys
from driver import TestCase, TestDriver


class HelpTests(TestCase):

    def test_f1_opens_help(self):
        self.app.send("keydown f1")
        self.assertState({"helpVisible": "1"})

    def test_f1_toggles_off(self):
        self.app.send("keydown f1")
        self.app.send("keydown f1")
        self.assertState({"helpVisible": "0"})

    def test_escape_closes_help(self):
        self.app.send("keydown f1")
        self.app.send("keydown escape")
        self.assertState({"helpVisible": "0"})

    def test_escape_without_help_is_noop(self):
        self.app.send("keydown escape")
        self.assertState({"helpVisible": "0"})

    def test_fkeyclick_1_opens_help(self):
        self.app.send("fkeyclick 1")
        self.assertState({"helpVisible": "1"})

    def test_help_does_not_affect_panel_state(self):
        self.app.send("keydown down")
        self.app.send("keydown f1")
        self.assertState({"leftSelected": "1", "helpVisible": "1"})

    def test_reset_closes_help(self):
        # driver.reset() already asserts helpVisible=0; this makes it explicit.
        self.app.send("keydown f1")
        s = self.app.state()
        assert s["helpVisible"] == "1", f"expected 1, got {s['helpVisible']}"
        # reset is called automatically before the next test; verify it here too.
        self.app.reset()
        s = self.app.state()
        assert s["helpVisible"] == "0", f"expected 0 after reset, got {s['helpVisible']}"


if __name__ == "__main__":
    import os
    _TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
    app = TestDriver(sys.argv[1], _TESTS_DIR)
    ok = HelpTests(sys.argv[1]).run(app)
    app.quit()
    sys.exit(0 if ok else 1)
