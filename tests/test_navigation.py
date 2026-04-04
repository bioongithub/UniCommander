"""
Verify arrow key navigation within a directory panel.
"""
from driver import TestCase


class NavigationTests(TestCase):

    def test_down_moves_selection(self):
        self.app.send("keydown down")
        self.assertState({"leftSelected": "1"})

    def test_up_after_down_returns_to_top(self):
        self.app.send("keydown down")
        self.app.send("keydown up")
        self.assertState({"leftSelected": "0"})

    def test_up_at_top_does_not_go_negative(self):
        self.app.send("keydown up")
        self.assertState({"leftSelected": "0"})

    def test_down_at_bottom_does_not_exceed_last(self):
        s = self.app.state()
        count = len(s["leftEntries"].split(","))
        for _ in range(count + 5):
            self.app.send("keydown down")
        self.assertState({"leftSelected": str(count - 1)})

    def test_navigation_affects_focused_panel_only(self):
        self.app.send("keydown tab")   # switch focus to right
        self.app.send("keydown down")
        self.assertState({"leftSelected": "0", "rightSelected": "1"})
