"""Regression checks for opt-in eight-ray Android accessibility sonar."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
RAW = ROOT / "android/app/src/main/res/raw"


class DirectionalSonarContracts(unittest.TestCase):
    def test_eight_unrestricted_rays_keep_original_range_and_nearest_targets(self):
        header = (SRC / "a11y_perception.h").read_text()
        scan = (SRC / "a11y_perception.cpp").read_text()
        sonar = scan.split("directional_sonar_result scan_directional_sonar(", 1)[1]
        sonar = sonar.split("tile_observation observe_memory_tile(", 1)[0]
        self.assertIn("std::array<int, 8> creatures;", header)
        self.assertIn("std::array<int, 8> items;", header)
        self.assertIn("distance <= 16", sonar)
        self.assertIn("direction < 8", sonar)
        self.assertIn("dx[8] = { 0, 1, 1, 1, 0, -1, -1, -1 }", sonar)
        self.assertIn("dy[8] = { -1, -1, 0, 1, 1, 1, 0, -1 }", sonar)
        self.assertIn("const unrestricted_tile tile = scan_unrestricted_tile( target );", sonar)
        self.assertIn("!tile.creatures.empty()", sonar)
        self.assertIn("!tile.items.empty()", sonar)
        self.assertIn("result.creatures[direction] < 0", sonar)
        self.assertIn("result.items[direction] < 0", sonar)
        # A chair, wall, closed door or poor visibility must never end the
        # ray or suppress items/creatures on later cells.
        self.assertNotIn("observe_tile( target )", sonar)
        self.assertNotIn("seen.source != observation_source::visible", sonar)
        self.assertNotIn("tile.impassable", sonar)
        self.assertIn("if( !here.inbounds( target ) )", sonar)
        self.assertIn("result.creatures[direction] > 0 && result.items[direction] > 0", sonar)

    def test_uses_same_unrestricted_tile_reader_as_manual_scan(self):
        assistive = (SRC / "a11y_world.cpp").read_text()
        sonar = (SRC / "a11y_perception.cpp").read_text()
        self.assertIn("unrestricted_tile scan_unrestricted_tile(", assistive)
        self.assertIn("const unrestricted_tile tile = scan_unrestricted_tile( pos );", assistive)
        self.assertIn("const unrestricted_tile tile = scan_unrestricted_tile( target );", sonar)
        self.assertIn('#include "a11y_world.h"', sonar)

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
