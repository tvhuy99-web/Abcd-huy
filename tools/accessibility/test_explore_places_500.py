"""Regression guards for the 500-tile spoken overmap destinations workflow."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class ExplorePlaces500Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ui = (ROOT / "src/overmap_ui.cpp").read_text(encoding="utf-8")
        cls.header = (ROOT / "src/overmap_ui.h").read_text(encoding="utf-8")
        cls.world = (ROOT / "src/a11y_world.cpp").read_text(encoding="utf-8")
        cls.dispatch = (ROOT / "src/handle_action.cpp").read_text(
            encoding="utf-8"
        )
        cls.hub = (
            ROOT / "android/app/src/main/java/com/cleverraven/"
            "cataclysmdda/GameActionHub.java"
        ).read_text(encoding="utf-8")
        cls.reader = (
            ROOT / "android/app/src/main/java/com/cleverraven/"
            "cataclysmdda/GameReaderController.java"
        ).read_text(encoding="utf-8")
        cls.translations = (ROOT / "lang/po/accessibility/vi.po").read_text(
            encoding="utf-8"
        )
        cls.places = cls.ui.split(
            "void ui::omap::display_accessible_places()", 1
        )[1].split("void ui::omap::display()", 1)[0]

    def test_menu_keeps_old_commands_but_hides_look_and_peek_from_group(self):
        self.assertIn('"reader:places", _( "Explore places" )', self.world)
        self.assertIn('ui::omap::display_accessible_places();', self.dispatch)
        self.assertIn('void display_accessible_places();', self.header)
        self.assertIn(
            '{ "listitems", "reader:places", "map", "sky",'
            ' "center", "zones" }',
            self.hub,
        )
        self.assertIn('node.actions.contains("activate")', self.hub)

    def test_known_500_tile_search_and_individual_buildings(self):
        code = self.places
        self.assertIn("constexpr int scan_tiles = 500;", code)
        self.assertIn("constexpr int tiles_per_omt = 24;", code)
        self.assertIn("const int distance = distance_to( target );", code)
        self.assertIn("distance > scan_tiles", code)
        self.assertIn("overmap_buffer.seen( target )", code)
        self.assertIn("vision <= om_vision_level::outlines", code)
        self.assertIn("overmap_buffer.ter_existing( target )", code)
        self.assertIn("terrain->get_name( vision )", code)
        self.assertIn("ordinary && !natural", code)
        self.assertIn("if( candidate.natural_group.empty() )", code)
        self.assertIn("places.push_back( candidate );", code)
        self.assertIn("std::stable_sort( places.begin(), places.end()", code)
        self.assertIn("overmap_buffer.get_cities_near(", code)

    def test_short_reading_without_preamble_or_position_counter(self):
        code = self.places
        self.assertIn("snapshot.title = _( \"Explore places\" );", code)
        self.assertNotIn("snapshot.summary =", code)
        self.assertNotIn("row.position =", code)
        self.assertNotIn("row.total =", code)
        self.assertIn("row.label = p.name;", code)
        self.assertIn('"places".equals(next.category)', self.reader)
        self.assertIn('snapshot.focus = "places:target:"', code)
        self.assertIn("constexpr std::size_t page_size = 80;", code)
        self.assertIn('"places:page:next"', code)
        self.assertIn('"places:page:previous"', code)

    def test_travel_modes_validate_route_and_current_vehicle(self):
        code = self.places
        self.assertIn('"places:mode:walk"', code)
        self.assertIn('"places:mode:run"', code)
        self.assertIn('"places:mode:drive"', code)
        self.assertIn("you.in_vehicle && you.controlling_vehicle", code)
        self.assertIn("get_overmap_path_to( destination, driving )", code)
        self.assertIn("route.empty()", code)
        self.assertIn("you.omt_path = std::move( route );", code)
        self.assertIn(
            "you.assign_activity( autodrive_activity_actor() );", code
        )
        self.assertIn("you.assign_activity( ACT_TRAVELLING );", code)
        self.assertIn("you.set_desired_movement_mode(", code)
        self.assertIn("if( !shore )", code)
        self.assertIn("you.pos_abs_omt()", code)

    def test_vietnamese_strings(self):
        for phrase in (
            'msgid "Explore places"\nmsgstr "Khám phá địa điểm"',
            'msgid "Walk"\nmsgstr "Đi bộ"',
            'msgid "Run"\nmsgstr "Chạy"',
            'msgid "Drive"\nmsgstr "Đi xe"',
            'msgid "%1$s, about %2$d tiles"',
            'msgid "No known destinations nearby."',
        ):
            self.assertIn(phrase, self.translations)


if __name__ == "__main__":
    unittest.main()
