"""Regression guards for the Android spoken tutorial combat target."""
import json
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class TutorialCombatDummyContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tutorial = (
            ROOT / "src/gamemode_tutorial.cpp"
        ).read_text(encoding="utf-8")
        cls.layout = json.loads(
            (ROOT / "data/mods/dda_tutorial/mapgen.json").read_text(
                encoding="utf-8"
            )
        )
        cls.english = (
            ROOT / "android/app/src/main/res/values/strings.xml"
        ).read_text(encoding="utf-8")
        cls.vietnamese = (
            ROOT / "android/app/src/main/res/values-vi/strings.xml"
        ).read_text(encoding="utf-8")

    def test_android_restores_only_missing_tutorial_dummies(self):
        source = self.tutorial.split("bool tutorial_game::init()", 1)[1]
        source = source.split("void tutorial_game::per_turn()", 1)[0]
        self.assertIn("#if defined(__ANDROID__)", source)
        self.assertIn("g->update_map( player_character );", source)
        self.assertIn("tutorial_map.inbounds( tile ) && g->is_empty( tile )", source)
        self.assertIn("g->place_critter_at( tutorial_dummy, tile )", source)
        self.assertIn('mtype_id tutorial_dummy( "mon_dummy_tutorial" )', source)
        self.assertIn('diagnostic_event( "TUTORIAL_DUMMY"', source)
        self.assertNotIn("g->place_critter_at(", self.tutorial.split(
            "void tutorial_game::per_turn()", 1
        )[1])

    def test_restored_positions_match_actual_mapgen_dummy_tiles(self):
        tutorial = next(
            item for item in self.layout
            if item.get("type") == "mapgen"
            and item.get("om_terrain") == "tutorial"
        )
        rows = tutorial["object"]["rows"]
        expected = {
            (x, y)
            for y, row in enumerate(rows)
            for x, char in enumerate(row)
            if char == "d"
        }
        code = self.tutorial.split(
            "const std::array<point, 4> dummy_offsets", 1
        )[1].split("};", 1)[0]
        actual = {
            (int(x), int(y))
            for x, y in re.findall(r"point\(\s*(\d+),\s*(\d+)\s*\)", code)
        }
        self.assertEqual(actual, expected)
        self.assertIn((21, 9), expected)

    def test_reader_explains_adjacent_melee_and_empty_tile_movement(self):
        self.assertIn('name="a11yCombatOn"', self.english)
        self.assertIn("adjacent creature", self.english)
        self.assertIn("empty tile can still move you", self.english)
        self.assertIn('name="a11yCombatOn"', self.vietnamese)
        # The new combat hint distinguishes gunfire, melee and walking
        # without implying that empty tiles are always attacked.
        combat = self.vietnamese.split(
            'name="a11yCombatOn">', 1)[1].split("</string>", 1)[0]
        for required in ("bắn súng", "đánh", "ô trống", "có thể bước"):
            self.assertIn(required, combat)
        self.assertIn('name="a11yGuideAttackNorth">Đánh hoặc bước',
                      self.vietnamese)


if __name__ == "__main__":
    unittest.main()
