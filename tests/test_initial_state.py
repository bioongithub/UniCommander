"""
Verify initial panel state against the actual filesystem.
"""
from driver import TestCase


class InitialStateTests(TestCase):

    def test_initial_state(self):
        self.assertState({
            "focus":         "left",
            "leftSelected":  "0",
            "rightSelected": "0",
        })
