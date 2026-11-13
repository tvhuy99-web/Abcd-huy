"""Regression checks for source-grounded Vietnamese practice names in the reader."""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CRAFTING = (ROOT / "src/crafting_gui.cpp").read_text(encoding="utf-8")
HELPERS = (ROOT / "src/crafting_gui_helpers.cpp").read_text(encoding="utf-8")
RECIPE = (ROOT / "src/recipe.cpp").read_text(encoding="utf-8")
RECIPE_H = (ROOT / "src/recipe.h").read_text(encoding="utf-8")
PO = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")
PRACTICE = ROOT / "data/json/recipes/practice"


class PracticeSpeechContract(unittest.TestCase):
    def test_names_in_the_session_log_are_not_original_game_names(self):
        expected = {
            "prac_computer_beg": ("computers.json", "computer (beginner)"),
            "prac_athletics_beg": ("athletics.json", "athletics (beginner)"),
            "prac_dancing": ("melee.json", "dancing"),
            "prac_dodge_beg": ("melee.json", "dodge (beginner)"),
            "prac_unarmed_beg": ("melee.json", "unarmed (beginner)"),
        }
        for recipe_id, (file_name, name) in expected.items():
            with self.subTest(recipe=recipe_id):
                data = json.loads((PRACTICE / file_name).read_text(encoding="utf-8"))
                actual = next(r for r in data if r.get("id") == recipe_id)
                self.assertEqual(actual["name"], name)
                self.assertIn('original == "' + name + '"', CRAFTING)

    def test_speech_titles_use_canonical_source_and_reviewed_translations(self):
        self.assertIn("practice_source_name = jo.has_string", RECIPE)
        self.assertIn("practice_source_description = jo.has_string", RECIPE)
        self.assertIn("std::string practice_source_name;", RECIPE_H)
        self.assertIn("std::string practice_source_description;", RECIPE_H)
        self.assertIn("accessible_crafting_recipe_name( rec )", CRAFTING)
        self.assertIn("accessible_practice_description( rec, *crafter )", CRAFTING)
        self.assertIn('msgid "Computer practice, beginner"', PO)
        self.assertIn('msgstr "Luyện máy tính, cơ bản"', PO)
        self.assertIn('msgid "Dodging practice, beginner"', PO)
        self.assertIn('msgstr "Luyện né tránh, cơ bản"', PO)
        # A new practice recipe can show its true source instead of a
        # misleading construction blueprint from the bad translation catalog.
        self.assertIn("rec.practice_source_description", CRAFTING)

    def test_spoken_darkness_and_failure_terms_are_natural(self):
        self.assertIn('_( "Cannot be done in the dark." )', HELPERS)
        self.assertIn('_( "Chance of ruining the item: %.2f%%" )', HELPERS)
        self.assertIn('msgstr "Không thể thực hiện trong bóng tối."', PO)
        self.assertIn('msgstr "Nguy cơ làm hỏng vật phẩm: %.2f%%"', PO)
        # Non-accessible presentation retains upstream formatting.
        self.assertIn('_( "Catastrophic Failure Chance:', HELPERS)
        self.assertIn('_( "Craftable in the dark?', HELPERS)
        self.assertIn('if( a11y::enabled() )', HELPERS)


if __name__ == "__main__":
    unittest.main()
