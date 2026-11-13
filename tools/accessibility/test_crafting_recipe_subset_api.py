"""Regression: accessible crafting startup uses recipe_subset's actual API."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class CraftingRecipeSubsetApiTest(unittest.TestCase):
    def test_recipe_subset_nonempty_checks_use_size(self):
        header = (ROOT / "src/recipe_dictionary.h").read_text(encoding="utf-8")
        crafting = (ROOT / "src/crafting_gui.cpp").read_text(encoding="utf-8")
        section = crafting.split(
            "a11y::semantic_snapshot crafting_ui_impl::reader_snapshot(", 1
        )[1].split("a11y::semantic_snapshot documents;", 1)[0]
        subset = header.split("class recipe_subset", 1)[1]
        self.assertIn("size_t size() const", subset)
        self.assertIn("available_recipes->size() > 0", section)
        self.assertIn("available_recipes->size() == 0", section)
        self.assertNotIn("available_recipes->empty()", section)

    def test_legacy_empty_screen_still_describes_real_recipe_count(self):
        crafting = (ROOT / "src/crafting_gui.cpp").read_text(encoding="utf-8")
        self.assertIn('snapshot.focus = "craft:no-recipes";', crafting)
        self.assertIn('static_cast<int>( available_recipes->size() )', crafting)


if __name__ == "__main__":
    unittest.main()
