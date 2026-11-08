"""Contracts for consistent self-voicing gestures across in-game menus."""
import unittest
from pathlib import Path

JAVA = Path(__file__).resolve().parents[2] / (
    "android/app/src/main/java/com/cleverraven/cataclysmdda"
)


class GameMenuGestures(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.surface = (JAVA / "ReaderGestureSurface.java").read_text(
            encoding="utf-8"
        )
        cls.controller = (JAVA / "GameReaderController.java").read_text(
            encoding="utf-8"
        )

    def test_menu_two_finger_down_is_back_but_world_scan_is_preserved(self):
        self.assertIn("TwoFingerBackGesture.matches(event", self.surface)
        self.assertIn("!listener.directWorldGesturesEnabled()", self.surface)
        self.assertIn("listener.goBack();", self.surface)
        self.assertIn("ignoreUntilPointersReleased", self.surface)
        self.assertIn("listener.scanDirection(", self.surface)
        self.assertIn('if (maxPointers == 2)', self.surface)
        self.assertIn("public void goBack() { onBackPressed(); }", self.controller)
        self.assertIn('send("cancel", "");', self.controller)

    def test_value_adjustments_are_limited_to_declared_fields(self):
        self.assertIn("listener.adjustFocusedSetting(dy < 0 ? 1 : -1)",
                      self.surface)
        self.assertIn('"choice".equals(node.fieldType)', self.controller)
        self.assertIn('"int".equals(node.fieldType)', self.controller)
        self.assertIn('"float".equals(node.fieldType)', self.controller)
        self.assertIn("node.hasFieldRange", self.controller)
        self.assertIn("node.fieldStep > 0", self.controller)
        self.assertIn('send("set_value", candidate.stripTrailingZeros().toPlainString())',
                      self.controller)
        self.assertIn('send("select_choice", node.choices.get(next).value)',
                      self.controller)
        self.assertIn("readCurrent();", self.controller)


if __name__ == "__main__":
    unittest.main()
