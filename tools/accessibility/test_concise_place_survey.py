"""Speech contracts for the agreed concise, coordinate-first place survey."""

import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SCENE = (ROOT / "src/a11y_scene.cpp").read_text(encoding="utf-8")
HEADER = (ROOT / "src/a11y_scene.h").read_text(encoding="utf-8")
TESTS = (ROOT / "tests/a11y_scene_test.cpp").read_text(encoding="utf-8")
WORLD = (ROOT / "src/a11y_world.cpp").read_text(encoding="utf-8")
VI = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")


class ConcisePlaceSurveyTests(unittest.TestCase):
    def test_current_coordinates_keep_values_with_tts_spacing(self):
        self.assertIn("void announce_player_coordinates()", WORLD)
        self.assertIn(
            'msgstr "Tọa độ khu vực X %1$d, Y %2$d. '
            'Bản đồ chung X %3$d, Y %4$d, Z %5$d."', VI
        )
        self.assertNotIn(
            'msgstr "Tọa độ khu vực X%1$d, Y%2$d. '
            'Bản đồ chung X%3$d, Y%4$d, Z%5$d."', VI
        )

    def test_report_starts_with_real_site_size_floors_and_player_position(self):
        body = SCENE.split("std::string build_survey()", 1)[1]
        self.assertLess(body.index('_( "Place survey: %s." )'),
                        body.index('_( "Area width %1$d'))
        self.assertLess(body.index('_( "Area width %1$d'),
                        body.index('_( "Map levels: %d." )'))
        self.assertLess(body.index('_( "Map levels: %d." )'),
                        body.index('_( "You are at local X %1$d'))
        self.assertIn("site.identified && site.complete", body)
        self.assertIn('_( "X %1$d, Y %2$d" )', SCENE)
        self.assertNotIn('_( "X%1$d, Y%2$d" )', SCENE)

    def test_windows_are_edge_connected_not_individual_squares(self):
        self.assertIn("representative_tiles(", HEADER)
        self.assertIn("std::set<point> remaining", SCENE)
        self.assertIn("remaining.erase( neighbor )", SCENE)
        self.assertIn("by_name[feature.label]", SCENE)
        self.assertIn('found.windows, player, landmark_local, 48, true', SCENE)
        self.assertIn("a11y_place_survey_groups_connected_window_squares_once",
                      TESTS)
        self.assertIn("a11y_place_survey_connected_group_representative_is_stable",
                      TESTS)

    def test_landmarks_speak_only_name_and_coordinates(self):
        body = SCENE.split("void append_features(", 1)[1].split(
            "void append_groups(", 1
        )[0]
        self.assertIn("entry.label + \", \" +", body)
        self.assertIn("point_speech( entry.position, player, local )", body)
        self.assertNotIn("compass_direction", body)
        self.assertNotIn("tiles by position", body)
        furniture = SCENE.split("void append_groups(", 1)[1].split(
            "void append_adjacent_places(", 1
        )[0]
        self.assertIn("ordered[i].first", furniture)
        self.assertIn("point_speech( ordered[i].second.first", furniture)
        self.assertNotIn("positions or stacks", furniture)

    def test_report_has_no_items_or_conclusion_or_statistical_lecture(self):
        body = SCENE.split("std::string build_survey()", 1)[1].split(
            "} // namespace", 1
        )[0]
        self.assertNotIn("append_groups( report, _( \"Item types\" )", body)
        self.assertNotIn("items", body)
        self.assertNotIn("full_loaded", body)
        self.assertNotIn("wall tiles", body)
        self.assertNotIn("orientation summary", body.lower())
        self.assertNotIn("End of survey.", body)
        self.assertNotIn("Kết thúc khảo sát", body)
        self.assertNotIn("here.i_at( p )", SCENE)
        self.assertNotIn("item_stacks", SCENE)

    def test_existing_single_action_and_safety_boundary_preserved(self):
        self.assertIn("maximum_survey_tiles = 20000", SCENE)
        self.assertIn('a11y::android::announce( "A11Y_SCENE:" + build_survey(), 1 )',
                      SCENE)
        self.assertIn("overmap_buffer.ter_existing( *next_place )", SCENE)


if __name__ == "__main__":
    unittest.main()
