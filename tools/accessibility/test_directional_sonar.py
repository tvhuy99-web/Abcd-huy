"""Regression checks for opt-in eight-ray Android accessibility sonar."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
RAW = ROOT / "android/app/src/main/res/raw"


class DirectionalSonarContracts(unittest.TestCase):
    def test_independent_visible_creature_and_item_rays(self):
        header = (SRC / "a11y_perception.h").read_text()
        scan = (SRC / "a11y_perception.cpp").read_text()
        self.assertIn("std::array<int, 8> creatures;", header)
        self.assertIn("std::array<int, 8> items;", header)
        self.assertIn("distance <= 16", scan)
        self.assertIn("direction < 8", scan)
        self.assertIn("dx[8] = { 0, 1, 1, 1, 0, -1, -1, -1 }", scan)
        self.assertIn("dy[8] = { -1, -1, 0, 1, 1, 1, 0, -1 }", scan)
        self.assertIn("seen.source != observation_source::visible", scan)
        self.assertIn("!seen.seen_creatures.empty()", scan)
        self.assertIn("!seen.seen_items.empty()", scan)

    def test_both_native_paths_use_same_observer(self):
        game = (SRC / "game.cpp").read_text()
        engine = (SRC / "handle_action.cpp").read_text()
        for text in (game, engine):
            self.assertIn("a11y::scan_directional_sonar(", text)
            self.assertIn("a11y::android::directional_sonar_enabled()", text)
            self.assertIn("a11y::android::directional_sonar_targets(", text)
        bridge = (SRC / "a11y_android.cpp").read_text()
        self.assertIn('"onAccessibilityDirectionalSonar"', bridge)
        self.assertIn('"([I[I)V"', bridge)
        self.assertIn("directional_sonar_opt_in.load(", bridge)

    def test_android_separate_sound_and_opt_in(self):
        player = (JAVA / "AlignmentSonarPlayer.java").read_text()
        preferences = (JAVA / "ReaderPreferences.java").read_text()
        ui = (JAVA / "AccessibilitySettingsActivity.java").read_text()
        self.assertIn("R.raw.a11y_item_beep", player)
        self.assertIn("R.raw.a11y_alignment_sonar", player)
        self.assertIn("creatures[heading] > 0", player)
        self.assertIn("items[heading] > 0", player)
        self.assertIn("setMaxStreams(DIRECTIONS * 2)", player)
        self.assertIn("private boolean enabled;", player)
        self.assertIn('getBoolean("directional_sonar", false)', preferences)
        self.assertIn("prefs.setDirectionalSonarEnabled(checked)", ui)
        self.assertIn("nativeConfigureDirectionalSonar(checked)", ui)
        self.assertTrue((RAW / "a11y_item_beep.wav").exists())
        self.assertTrue((RAW / "a11y_alignment_sonar.ogg").exists())


if __name__ == "__main__":
    unittest.main()
