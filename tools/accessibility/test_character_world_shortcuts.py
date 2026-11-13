"""Source-based regression checks for fixed three-finger world shortcuts."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WORLD = (ROOT / "src/a11y_world.cpp").read_text(encoding="utf-8")
DISPLAY = (ROOT / "src/display.cpp").read_text(encoding="utf-8")
GESTURES = (ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/ReaderGestureSurface.java").read_text(
    encoding="utf-8"
)
READER = (ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/GameReaderController.java").read_text(
    encoding="utf-8"
)
VI = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")


class ThreeFingerWorldShortcutsTest(unittest.TestCase):
    def test_single_tap_place_coordinates_light_weather_time(self):
        body = WORLD.split("void announce_player_coordinates()", 1)[1].split(
            "semantic_snapshot activity_snapshot", 1)[0]
        self.assertIn("overmap_buffer.seen( omt )", body)
        self.assertIn("overmap_buffer.ter_existing( omt )", body)
        labels = ["Area: %1$s. %2$s", "Lighting: %s", "Weather: %s", "Time: %s"]
        for left, right in zip(labels, labels[1:]):
            self.assertLess(body.index(left), body.index(right))
        self.assertIn("get_map().light_at( viewer.pos_bub() )", body)
        self.assertIn("display::weather_text_color( viewer ).first", body)
        self.assertIn("display::time_string( viewer )", body)
        self.assertIn('game_time == _( "???" ) ? _( "Unknown" )', body)

    def test_clock_never_exposes_unearned_exact_time(self):
        self.assertIn("if( u.has_watch() )", DISPLAY)
        self.assertIn("can_creature_see_sky( u )", DISPLAY)
        self.assertIn('return _( "???" );', DISPLAY)

    def test_double_tap_uses_real_metrics_without_closing_sentence(self):
        body = WORLD.split("std::string character_details = string_format(", 1)[1].split(
            "std::move( character_details )", 1)[0]
        for metric in ["character.get_kcal_percent()", "character.get_focus()",
                       "character.get_speed()", "character.get_moves()",
                       "character.run_cost( 100, false )",
                       "character.weariness_transition_progress()",
                       "display::activity_text_color( character ).first",
                       "display::activity_malus_str( character )", "character.volume"]:
            self.assertIn(metric, body)
        self.assertNotIn("End of character information.", body)
        for metric in ['"Hunger: %s"', '"Thirst: %s"', '"Sleepiness: %s"']:
            self.assertIn(metric, body)

    def test_existing_fixed_gestures_remain(self):
        self.assertIn("listener.threeFingerTap();", GESTURES)
        self.assertIn("listener.characterDetails();", GESTURES)
        self.assertIn('requestLiveWorldCommand("reader:coordinates")', READER)
        self.assertIn('snapshot.find("world:character_detail")', READER)

    def test_vietnamese_speech(self):
        for label in ['msgid "Lighting: %s"\nmsgstr "Ánh sáng: %s"',
                      'msgid "Time: %s"\nmsgstr "Thời gian: %s"',
                      'msgid "Energy reserves: %d%%"\nmsgstr "Dự trữ năng lượng: %d%%"',
                      'msgid "Normal"\nmsgstr "Bình thường"']:
            self.assertIn(label, VI)


if __name__ == "__main__":
    unittest.main()
