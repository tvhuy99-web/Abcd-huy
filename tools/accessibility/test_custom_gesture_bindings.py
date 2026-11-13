"""Regression contracts for 36 recognized gestures, only 25 customizable."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
RESOURCE = ROOT / "android/app/src/main/res"


class CustomGestureContracts(unittest.TestCase):
    def test_exactly_36_documented_choices_and_both_locales(self):
        src = (JAVA / "ReaderGestureBindings.java").read_text(
            encoding="utf-8"
        )
        ids_block = src.split("static final String[] IDS = {", 1)[1]
        ids_block = ids_block.split("};", 1)[0]
        choices = re.findall(r'"([^"]+)"', ids_block)
        self.assertEqual(len(choices), 36)
        self.assertEqual(len(set(choices)), 36)
        self.assertEqual(choices[-3:], ["hold_1", "hold_2", "hold_3"])
        for prefix in ("values", "values-vi"):
            xml = (RESOURCE / prefix / "strings.xml").read_text(
                encoding="utf-8"
            )
            for choice in choices:
                label = "a11yBind" + "".join(
                    word.capitalize() for word in choice.split("_")
                )
                self.assertIn('name="' + label + '"', xml)

    def test_newest_binding_replaces_older(self):
        src = (JAVA / "ReaderGestureBindings.java").read_text(
            encoding="utf-8"
        )
        self.assertIn("String displacedTarget = mapping.get(gesture)", src)
        self.assertIn("mapping.remove(displacedGesture)", src)
        self.assertIn("mapping.put(gesture, target)", src)
        self.assertIn("mapping.values().removeAll(", src)
        self.assertIn('putString(KEY, json.toString())', src)
        # STT 21 (XOA MA): restoreGestureDefaults() was deleted with the
        # old setup screen, so there is no restore-defaults path left in
        # prefs. Custom bindings live only under KEY in
        # ReaderGestureBindings (loaded on start, saved on change).
        prefs = (JAVA / "ReaderPreferences.java").read_text(
            encoding="utf-8"
        )
        self.assertNotIn("restoreGestureDefaults", prefs)
        self.assertIn('store.getString(KEY, "{}")', src)

    def test_game_shortcuts_restore_the_comprehensive_menu(self):
        ctrl = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        surf = (JAVA / "ReaderGestureSurface.java").read_text(encoding="utf-8")
        self.assertTrue((JAVA / "GameActionHub.java").exists())
        self.assertIn("showGestureChoices()", ctrl)
        self.assertIn("showComprehensiveMenu()", ctrl)
        self.assertNotIn("showControls()", ctrl)
        self.assertIn('if (target.startsWith("action:"))', ctrl)
        self.assertIn('target.startsWith("hub:group:")', ctrl)
        self.assertIn('"hub:search".equals(target)', ctrl)
        self.assertIn("listener.handleCustomGesture(gesture)", surf)
        self.assertIn(
            'if (gesture == null || listener.handleCustomGesture(gesture))',
            surf,
        )

    def test_hold_gestures_keep_navigation_and_swipes_safe(self):
        src = (JAVA / "ReaderGestureSurface.java").read_text(
            encoding="utf-8"
        )
        bind = (JAVA / "ReaderGestureBindings.java").read_text(
            encoding="utf-8"
        )
        device = (
            ROOT / "android/app/src/androidTest/java"
            / "com/cleverraven/cataclysmdda/GameMenuGestureDispatchTest.java"
        ).read_text(encoding="utf-8")
        self.assertIn("static String idForHold(int fingers)", bind)
        self.assertIn('"hold_" + fingers', bind)
        self.assertIn("private void scheduleMultiHold(", src)
        self.assertIn("private void cancelMultiHold()", src)
        self.assertIn("ViewConfiguration.getLongPressTimeout()", src)
        self.assertIn("multiTrackedCount != fingers || multiFingerMoved", src)
        self.assertIn("if (!listener.directWorldGesturesEnabled()", src)
        self.assertIn("cancelled = true; // A hold must never", src)
        self.assertIn("if (action == MotionEvent.ACTION_POINTER_UP", src)
        self.assertIn("listener.handleCustomGesture(gesture)", src)
        self.assertIn(
            "singleFingerHoldAlwaysOpensContextActions", device
        )
        self.assertIn(
            "twoAndThreeFingerHoldsFireOnceWithoutBecomingTaps", device
        )
        self.assertIn("movingFingersCancelsPendingHold", device)

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
