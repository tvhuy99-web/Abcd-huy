"""Guard upstream symmetric armor coverage and the fork's screen reader."""

import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "src"


class SymmetricArmorCoverage(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.attire = (SRC / "character_attire.h").read_text("utf-8")
        cls.armor = (SRC / "armor_layers.cpp").read_text("utf-8")
        cls.attire_impl = (SRC / "character_attire.cpp").read_text("utf-8")
        cls.status = (SRC / "body_status_ui.cpp").read_text("utf-8")

    def test_upstream_opposite_limb_coverage_is_implemented(self):
        self.assertIn(
            "bool items_cover_opposite_bp( const Character &who, const bodypart_id &cover ) const;",
            self.attire)
        # The fork already implements this API in character_attire.cpp.
        # The upstream copy in armor_layers.cpp would duplicate a linker symbol.
        self.assertEqual(
            self.attire_impl.count("bool outfit::items_cover_opposite_bp("), 1)
        self.assertNotIn("bool outfit::items_cover_opposite_bp(", self.armor)
        body = self.attire_impl.split(
            "bool outfit::items_cover_opposite_bp(", 1)[1].split(
                "void outfit::splice(", 1)[0]
        self.assertIn("cover == bodypart_str_id::NULL_ID()", body)
        self.assertIn("const bodypart_id opposite = cover->opposite_part.id();", body)
        self.assertIn("for( const item &garment : worn )", body)
        self.assertIn("if( !garment.covers( opposite ) )", body)
        self.assertIn("return any_on_part;", body)

    def test_armor_reader_and_health_view_keep_shared_limb_semantics(self):
        self.assertIn(
            "displayed_character.worn.items_cover_opposite_bp( displayed_character, cover )",
            self.status)
        self.assertIn(
            "worn.items_cover_opposite_bp( displayed_character, cover )", self.status)
        self.assertIn("struct armor_layers_reader", self.armor)
        self.assertIn("a11y::semantic_snapshot snapshot;", self.armor)
        self.assertIn("reader.update();", self.armor)


if __name__ == "__main__":
    unittest.main()
