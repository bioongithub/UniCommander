"""
Verify panel focus switching via Tab key.
"""
from driver import TestCase


class FocusTests(TestCase):

    def test_tab_switches_focus_to_right(self):
        self.app.send("keydown tab")
        self.assertState({"focus": "right"})

    def test_tab_switches_focus_back_to_left(self):
        self.app.send("keydown tab")
        self.app.send("keydown tab")
        self.assertState({"focus": "left"})
