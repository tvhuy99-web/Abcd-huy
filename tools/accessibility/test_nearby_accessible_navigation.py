"""Regression coverage for the spoken near-to-far surroundings list."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class NearbyAccessibleNavigationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.menu = (SRC / "surroundings_menu.cpp").read_text("utf-8")
        cls.header = (SRC / "surroundings_menu.h").read_text("utf-8")
        cls.game = (SRC / "game.cpp").read_text("utf-8")
        cls.world = (SRC / "a11y_world.cpp").read_text("utf-8")
        cls.overmap = (SRC / "overmap_ui.cpp").read_text("utf-8")
        cls.hub = (JAVA / "GameActionHub.java").read_text("utf-8")
        cls.reader = (JAVA / "GameReaderController.java").read_text(
            "utf-8"
        )

    def test_visual_list_keeps_original_search(self):
        # The native visual tabs are unchanged; Android reader has its own
        # separate 20-tile square and skips the 60-tile tab initialization.
        self.assertIn("const int radius = 60;", self.header)
        self.assertIn("closest_points_first( pos, radius )", self.menu)
        self.assertIn("get_visible_creatures( radius )", self.menu)
        self.assertIn("if( !a11y::enabled() )", self.menu)

    def test_flat_nearest_first_items_creatures_terrain(self):
        self.assertIn("void surroundings_menu::execute_accessible()",
                      self.menu)
        self.assertIn("a11y::observe_tile( position )", self.menu)
        self.assertIn("tile.seen_items", self.menu)
        self.assertIn("tile.seen_creatures", self.menu)
        self.assertIn("tile.known_traps", self.menu)
        self.assertIn("tile.vehicle", self.menu)
        self.assertIn("std::stable_sort( nearby.begin(), nearby.end()",
                      self.menu)
        self.assertIn("lhs.distance < rhs.distance", self.menu)
        self.assertIn('"nearby:target:"', self.menu)
        # Spoken direction includes the only distance phrase, but the
        # underlying distance still drives nearest-first sorting.
        self.assertIn("node.value = relative_words( entry.relative );",
                      self.menu)

    def test_page_without_shrinking_radius_or_snapshot_overflow(self):
        self.assertIn("constexpr std::size_t page_size = 80;", self.menu)
        self.assertIn('"nearby:page:next"', self.menu)
        self.assertIn('"nearby:page:previous"', self.menu)
        self.assertIn("const std::size_t end = std::min(", self.menu)
        # No spoken position counters on each entry.
        self.assertNotIn("node.total = static_cast<int>( nearby.size() );",
                         self.menu)

    def test_selected_target_uses_safety_aware_pathfinder(self):
        self.assertIn("g->safe_route_to( you, target.target", self.menu)
        self.assertIn("you.set_destination( *route );", self.menu)
        self.assertIn("if( !route )", self.menu)
        self.assertIn("if( route->empty() )", self.menu)
        self.assertIn("here.impassable( position ) ? 1 : 0", self.menu)
        self.assertIn("for( const std::string &creature : tile.seen_creatures )", self.menu)
        self.assertIn("for( const std::string &item_name : tile.seen_items )", self.menu)
        self.assertIn("if( a11y::enabled() )", self.menu)
        self.assertIn("execute_accessible();", self.menu)

    def test_current_tile_alert_is_readable(self):
        self.assertIn('here.role = noteworthy_here ? "alert" : "document"',
                      self.world)
        self.assertIn('|| "world:here".equals(id)', self.hub)
        self.assertIn('|| "world:here".equals(node.id)', self.reader)

    def test_peek_direction_and_cost(self):
        self.assertIn("a11y::screen_scope direction_reader(",
                      self.game)
        self.assertIn('"peek:direction:"', self.game)
        self.assertIn("direction_input.handle_input()", self.game)
        self.assertIn("u.mod_moves( -u.get_speed() * 2 );", self.game)
        self.assertIn('choose_direction( _( "Peek where?" ), true )',
                      self.game)

    def test_sky_weather_obeys_visual_line_of_sight(self):
        self.assertIn("uistate.overmap_visible_weather", self.overmap)
        self.assertIn("viewer.overmap_los( cursor, sight * 2 )",
                      self.overmap)
        self.assertIn('_( "Weather: %s" )', self.overmap)
        self.assertIn('_( "Weather unknown" )', self.overmap)


if __name__ == "__main__":
    unittest.main()
