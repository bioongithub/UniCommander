"""
Verify modifier key indicator sticky state via modclick.
"""
import sys
from driver import TestCase


class ModifierTests(TestCase):

    def _mod_state(self):
        s = self.app.state()
        return s["modAlt"], s["modShift"], s["modCtrl"]

    def test_modclick_alt_toggles(self):
        self.app.send("modclick alt")
        alt, shift, ctrl = self._mod_state()
        assert alt == "1",   f"modAlt: expected 1, got {alt!r}"
        assert shift == "0", f"modShift: expected 0, got {shift!r}"
        assert ctrl == "0",  f"modCtrl: expected 0, got {ctrl!r}"
        self.app.send("modclick alt")
        alt, _, _ = self._mod_state()
        assert alt == "0", f"modAlt after second click: expected 0, got {alt!r}"

    def test_modclick_shift_toggles(self):
        self.app.send("modclick shift")
        alt, shift, ctrl = self._mod_state()
        assert alt == "0",   f"modAlt: expected 0, got {alt!r}"
        assert shift == "1", f"modShift: expected 1, got {shift!r}"
        assert ctrl == "0",  f"modCtrl: expected 0, got {ctrl!r}"
        self.app.send("modclick shift")
        _, shift, _ = self._mod_state()
        assert shift == "0", f"modShift after second click: expected 0, got {shift!r}"

    def test_modclick_ctrl_toggles(self):
        self.app.send("modclick ctrl")
        alt, shift, ctrl = self._mod_state()
        assert alt == "0",   f"modAlt: expected 0, got {alt!r}"
        assert shift == "0", f"modShift: expected 0, got {shift!r}"
        assert ctrl == "1",  f"modCtrl: expected 1, got {ctrl!r}"
        self.app.send("modclick ctrl")
        _, _, ctrl = self._mod_state()
        assert ctrl == "0", f"modCtrl after second click: expected 0, got {ctrl!r}"

    def test_modifiers_are_independent(self):
        self.app.send("modclick alt")
        self.app.send("modclick shift")
        self.app.send("modclick ctrl")
        alt, shift, ctrl = self._mod_state()
        assert alt == "1",   f"modAlt: expected 1, got {alt!r}"
        assert shift == "1", f"modShift: expected 1, got {shift!r}"
        assert ctrl == "1",  f"modCtrl: expected 1, got {ctrl!r}"

    def test_reset_clears_modifiers(self):
        self.app.send("modclick alt")
        self.app.send("modclick shift")
        # reset() sends reset command and asserts modifiers are 0
        self.app.reset()
        alt, shift, ctrl = self._mod_state()
        assert alt == "0",   f"modAlt after reset: expected 0, got {alt!r}"
        assert shift == "0", f"modShift after reset: expected 0, got {shift!r}"
        assert ctrl == "0",  f"modCtrl after reset: expected 0, got {ctrl!r}"


if __name__ == "__main__":
    ok = ModifierTests(sys.argv[1]).run()
    sys.exit(0 if ok else 1)
