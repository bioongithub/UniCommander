"""
Verify Enter key activates directories and navigates to parent via '..'.
"""
import os
from driver import TestCase
from utils import entries_for


class ActivationTests(TestCase):

    def test_enter_on_directory_changes_path(self):
        s = self.app.state()
        # Find the first directory entry (skip '..')
        entries = s["leftEntries"].split(",")
        dir_entry = next((e for e in entries if e.endswith("/") and e != "../"), None)
        assert dir_entry is not None, "No subdirectory available to enter"

        dir_name = dir_entry.rstrip("/")
        expected_path = os.path.join(s["leftPath"], dir_name)

        # Navigate to it and activate
        idx = entries.index(dir_entry)
        for _ in range(idx):
            self.app.send("keydown down")
        self.app.send("keydown return")

        self.assertState({
            "leftPath":     expected_path,
            "leftSelected": "0",
        })

    def test_enter_on_dotdot_goes_to_parent(self):
        # Go into first available subdirectory first
        s = self.app.state()
        entries = s["leftEntries"].split(",")
        dir_entry = next((e for e in entries if e.endswith("/") and e != "../"), None)
        assert dir_entry is not None, "No subdirectory available to enter"

        idx = entries.index(dir_entry)
        for _ in range(idx):
            self.app.send("keydown down")
        self.app.send("keydown return")

        original_path = s["leftPath"]

        # Now selection is at 0 which is '..', activate it
        self.app.send("keydown return")
        self.assertState({"leftPath": original_path})

    def test_enter_on_file_does_not_change_path(self):
        s = self.app.state()
        entries = s["leftEntries"].split(",")
        file_entry = next((e for e in entries if not e.endswith("/")), None)
        assert file_entry is not None, "No file available in panel"

        idx = entries.index(file_entry)
        for _ in range(idx):
            self.app.send("keydown down")
        self.app.send("keydown return")

        self.assertState({"leftPath": s["leftPath"]})
