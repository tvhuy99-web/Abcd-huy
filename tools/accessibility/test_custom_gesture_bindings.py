"""Regression contracts for the 33 persistent world action shortcuts."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
RESOURCE = ROOT / "android/app/src/main/res"


class CustomGestureContracts(unittest.TestCase):
    def test_exactly_33_documented_choices_and_both_locales(self):
        src = (JAVA / "ReaderGestureBindings.java").read_text(
            encoding="utf-8"
        )
        ids_block = src.split("static final String[] IDS = {", 1)[1]
        ids_block = ids_block.split("};", 1)[0]
        choices = re.findall(r'"([^"]+)"', ids_block)
        self.assertEqual(len(choices), 33)
        self.assertEqual(len(set(choices)), 33)
        for prefix in ("values", "values-vi"):
            xml = (RESOURCE / prefix / "strings.xml").read_text(
                encoding="utf-8"
            )
            for choice in choices:
                label = "a11yBind" + "".join(
                    word.capitalize() for word in choice.split("_")
                )
                self.assertIn('name="' + label + '"', xml)

    def test_newest_binding_replaces_older_and_restores_default(self):
        src = (JAVA / "ReaderGestureBindings.java").read_text(
            encoding="utf-8"
        )
        self.assertIn("String displacedTarget = mapping.get(gesture)", src)
        self.assertIn("mapping.remove(displacedGesture)", src)
        self.assertIn("mapping.put(gesture, target)", src)
        self.assertIn("mapping.values().removeAll(", src)
        self.assertIn('putString(KEY, json.toString())', src)

    def test_hold_selects_entry_and_shortcut_dispatch_precedes_default(self):
        ctrl = (JAVA / "GameReaderController.java").read_text(
            encoding="utf-8"
        )
        surf = (JAVA / "ReaderGestureSurface.java").read_text(
            encoding="utf-8"
        )
        hub = (JAVA / "GameActionHub.java").read_text(
            encoding="utf-8"
        )
        self.assertIn("button.setOnLongClickListener", ctrl)
        self.assertIn("showGestureChoices();", ctrl)
        self.assertIn("private void applyGestureChoice()", ctrl)
        self.assertIn("ReaderGestureBindings.IDS.length + 2", ctrl)
        self.assertIn("gestureBindings.assign(", ctrl)
        self.assertIn("gestureBindings.restoreDefault(", ctrl)
        self.assertIn("if (gesture == null ||", surf)
        self.assertIn("listener.handleCustomGesture(gesture)", surf)
        self.assertIn("String shortcutTarget()", hub)

    def test_complex_swipes_and_multi_finger_sequences(self):
        src = (JAVA / "ReaderGestureSurface.java").read_text(
            encoding="utf-8"
        )
        self.assertIn("identifyOneFingerSwipe(", src)
        self.assertIn("first + \"_\" + second", src)
        self.assertIn("maxPointers <= 4", src)
        self.assertIn("if (maxPointers == 2)", src)
        self.assertIn("listener.scanDirection(", src)
        self.assertIn("queueMultiTap(owner, maxPointers)", src)
        self.assertIn("dispatchMultiTap(fingers, 3)", src)


if __name__ == "__main__":
    unittest.main()
