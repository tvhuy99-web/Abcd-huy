"""Android nearby list: a 41x41 square with meaningful, concise spoken rows."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src/surroundings_menu.cpp"
VI = ROOT / "lang/po/accessibility/vi.po"


class NearbySquareScanTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = SRC.read_text("utf-8")
        cls.reader = cls.source.split(
            "void surroundings_menu::execute_accessible()", 1)[1].split(
            "void surroundings_menu::execute()", 1
        )[0]

    def test_square_is_twenty_tiles_in_every_direction(self):
        code = self.reader
        self.assertIn("constexpr int scan_half_width = 20;", code)
        self.assertIn("distance <= scan_half_width", code)
        self.assertIn("dy = -distance; dy <= distance", code)
        self.assertIn("dx = -distance; dx <= distance", code)
        self.assertIn("std::max( std::abs( dx ), std::abs( dy ) ) != distance",
                      code)
        self.assertIn("here.inbounds( position )", code)
        self.assertNotIn("closest_points_first(", code)
        # 41x41 positions, including four corners, center, and each cardinal
        # endpoint: this is not a circular radius or 20 output entries.
        coords = {
            (x, y)
            for radius in range(21)
            for y in range(-radius, radius + 1)
            for x in range(-radius, radius + 1)
            if max(abs(x), abs(y)) == radius
        }
        self.assertEqual(len(coords), 41 * 41)
        for point in [(0, 0), (-20, -20), (20, 20),
                      (0, -20), (0, 20), (-20, 0), (20, 0)]:
            self.assertIn(point, coords)
        self.assertNotIn((21, 0), coords)

    def test_no_ordinary_ground_or_walls(self):
        s = self.source
        self.assertIn("static bool noteworthy_nearby_feature(", s)
        self.assertIn("TFLAG_DOOR", s)
        self.assertIn("TFLAG_GOES_UP", s)
        self.assertIn("TFLAG_GOES_DOWN", s)
        self.assertIn("TFLAG_CONSOLE", s)
        self.assertIn("type.can_examine( position )", s)
        self.assertNotIn("TFLAG_FLAT ) ||", s.split(
            "static bool noteworthy_nearby_feature(", 1
        )[1].split("void surroundings_menu::execute_accessible()", 1)[0])
        self.assertIn("noteworthy_nearby_feature( terrain, position )", s)
        # Mundane terrain remains filtered, but a locker without special
        # flags is still a meaningful furniture object.
        self.assertIn("!tile.furniture.empty()", self.reader)
        self.assertNotIn("noteworthy_nearby_feature( furniture, position )",
                         self.reader)

    def test_detected_objects_only_and_no_category_priority(self):
        code = self.reader
        self.assertIn("a11y::observe_tile( position )", code)
        self.assertIn("a11y::scan_unrestricted_tile( position )", code)
        self.assertNotIn("tile.source != a11y::observation_source::visible", code)
        for field in ("items", "creatures", "traps", "vehicle", "furniture"):
            self.assertIn("tile." + field, code)
        self.assertIn("observed.heard_marker", code)
        self.assertNotIn("add_row( tile.terrain, relative, 0 );", code)
        self.assertIn("seen_vehicles.insert(", code)
        self.assertIn("entry.second.is_dangerous()", code)
        self.assertIn("return lhs.distance < rhs.distance;", code)
        self.assertNotIn("return lhs.label < rhs.label;", code)

    def test_short_spoken_entries_no_preamble(self):
        code = self.reader
        self.assertIn('snapshot.title = _( "Nearby" );', code)
        self.assertNotIn("snapshot.summary =", code)
        self.assertNotIn("node.detail =", code)
        self.assertNotIn("node.position =", code)
        self.assertIn("node.value = relative_words( entry.relative );", code)
        vi = VI.read_text("utf-8")
        self.assertIn('msgid "Nearby"\nmsgstr "Xung quanh"', vi)
        self.assertIn('msgid "Nothing noteworthy nearby."', vi)


if __name__ == "__main__":
    unittest.main()
