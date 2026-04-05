"""
Tests for F10 quit with confirmation dialog.
"""
from driver import TestDriver, TestCase


class QuitTests(TestCase):

    def test_f10_cancel_keeps_running(self):
        """F10 + dialog No: app stays alive, state unchanged."""
        self.app.dialog("no")
        self.app.send("keydown f10")
        self.assertState({"focus": "left"})

    def test_f10_confirm_exits_zero(self):
        """F10 + dialog Yes: app exits cleanly with return code 0.
        Uses a fresh process so the shared driver is not terminated."""
        fresh = TestDriver(self._executable, self.app._initial_dir)
        fresh.dialog("yes")
        fresh.send("keydown f10")
        fresh.proc.wait(timeout=5)
        assert fresh.proc.returncode == 0, \
            f"expected exit code 0, got {fresh.proc.returncode}"
