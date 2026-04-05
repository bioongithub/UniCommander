"""
Verify navigation and activation in the right panel.
"""
import os
from driver import TestCase


class RightPanelTests(TestCase):

    # --- Navigation ---

    def test_down_moves_right_selection(self):
        self.app.send("keydown tab")
        self.app.send("keydown down")
        self.assertState({"leftSelected": "0", "rightSelected": "1"})

    def test_up_at_top_does_not_go_negative(self):
        self.app.send("keydown tab")
        self.app.send("keydown up")
        self.assertState({"rightSelected": "0"})

    def test_up_after_down_returns_to_top(self):
        self.app.send("keydown tab")
        self.app.send("keydown down")
        self.app.send("keydown up")
        self.assertState({"rightSelected": "0"})

    def test_down_at_bottom_does_not_exceed_last(self):
        self.app.send("keydown tab")
        s = self.app.state()
        count = len(s["rightEntries"].split(","))
        for _ in range(count + 5):
            self.app.send("keydown down")
        self.assertState({"rightSelected": str(count - 1)})

    def test_navigation_does_not_affect_left_panel(self):
        self.app.send("keydown tab")
        self.app.send("keydown down")
        self.app.send("keydown down")
        self.assertState({"leftSelected": "0", "rightSelected": "2"})

    # --- Activation ---

    def test_enter_on_directory_changes_right_path(self):
        self.app.send("keydown tab")
        s = self.app.state()
        entries = s["rightEntries"].split(",")
        dir_entry = next((e for e in entries if e.endswith("/") and e != "../"), None)
        assert dir_entry is not None, "No subdirectory available in right panel"

        dir_name = dir_entry.rstrip("/")
        expected_path = os.path.join(s["rightPath"], dir_name)

        idx = entries.index(dir_entry)
        for _ in range(idx):
            self.app.send("keydown down")
        self.app.send("keydown return")

        self.assertState({"rightPath": expected_path, "rightSelected": "0"})

    def test_enter_on_dotdot_goes_to_parent(self):
        self.app.send("keydown tab")
        s = self.app.state()
        entries = s["rightEntries"].split(",")
        dir_entry = next((e for e in entries if e.endswith("/") and e != "../"), None)
        assert dir_entry is not None, "No subdirectory available in right panel"

        idx = entries.index(dir_entry)
        for _ in range(idx):
            self.app.send("keydown down")
        self.app.send("keydown return")

        original_path = s["rightPath"]
        self.app.send("keydown return")
        self.assertState({"rightPath": original_path})

    def test_enter_on_file_does_not_change_right_path(self):
        self.app.send("keydown tab")
        s = self.app.state()
        entries = s["rightEntries"].split(",")
        file_entry = next((e for e in entries if not e.endswith("/")), None)
        assert file_entry is not None, "No file available in right panel"

        idx = entries.index(file_entry)
        for _ in range(idx):
            self.app.send("keydown down")
        self.app.send("keydown return")

        self.assertState({"rightPath": s["rightPath"]})

    def test_activation_does_not_affect_left_panel(self):
        self.app.send("keydown tab")
        s = self.app.state()
        left_path = s["leftPath"]
        entries = s["rightEntries"].split(",")
        dir_entry = next((e for e in entries if e.endswith("/") and e != "../"), None)
        assert dir_entry is not None, "No subdirectory available in right panel"

        idx = entries.index(dir_entry)
        for _ in range(idx):
            self.app.send("keydown down")
        self.app.send("keydown return")

        self.assertState({"leftPath": left_path, "leftSelected": "0"})
