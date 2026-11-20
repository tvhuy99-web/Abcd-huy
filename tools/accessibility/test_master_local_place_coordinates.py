"""Protect accessible local-place coordinates and absolute-world fallback."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class LocalPlaceCoordinateContracts(unittest.TestCase):
    def test_local_position_is_optional_and_world_position_is_kept(self):
        world = (ROOT / "src/a11y_world.cpp").read_text(encoding="utf-8")
        self.assertIn('#include "a11y_location.h"', world)
        self.assertIn("local_coordinates_at( pos )", world)
        self.assertIn("local->x, local->y, pos.x(), pos.y(), pos.z()", world)
        self.assertIn("Coordinates.  X %1$d. Y %2$d. Z %3$d.", world)
        self.assertIn("overmap_buffer.seen( omt )", world)

    def test_unknown_edges_never_have_a_guessed_local_origin(self):
        api = (ROOT / "src/a11y_location.h").read_text(encoding="utf-8")
        body = (ROOT / "src/a11y_location.cpp").read_text(encoding="utf-8")
        self.assertIn("std::optional<local_place_coordinates> local_coordinates_at(", api)
        self.assertIn("local_coordinates_at( const tripoint_abs_ms &pos )", body)
        self.assertIn("find_road_section(", api)
        self.assertIn("local_from_omt_corner(", api)

    def test_readout_is_translated_to_vietnamese(self):
        catalog = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")
        self.assertIn('msgid "Local coordinates X %1$d, Y %2$d. World map X %3$d, Y %4$d, Z %5$d."', catalog)
        self.assertIn('msgstr "Tọa độ khu vực X %1$d, Y %2$d. Bản đồ chung X %3$d, Y %4$d, Z %5$d."', catalog)


if __name__ == "__main__":
    unittest.main()
