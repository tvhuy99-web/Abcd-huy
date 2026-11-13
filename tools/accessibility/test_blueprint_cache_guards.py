"""Static checks for optional Android character blueprint cache."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


class BlueprintCacheTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.recipe = (SRC / "recipe.cpp").read_text(encoding="utf-8")
        cls.menu = (SRC / "main_menu.cpp").read_text(encoding="utf-8")
        cls.init = (SRC / "init.cpp").read_text(encoding="utf-8")

    def test_cache_is_separate_and_optional(self):
        self.assertIn('savegame.base_path() + ".loadcache"', self.menu)
        self.assertIn('temp += ".tmp";', self.recipe)
        self.assertIn("status=miss reason=stale_or_corrupt", self.recipe)

    def test_cache_invalidates_on_input_changes(self):
        self.assertIn("getVersionString(), cache_mod_ids", self.menu)
        self.assertIn("blueprint_cache::record_source(", self.init)
        self.assertIn("blueprint_cache::finish_sources();", self.init)

    def test_fresh_calculation_remains(self):
        self.assertIn("get_changed_ids_from_update(", self.recipe)
        self.assertIn("get_build_reqs_for_furn_ter_ids(", self.recipe)
        self.assertIn("blueprint_cache::store(", self.recipe)

    def test_does_not_write_before_save_loaded(self):
        self.assertIn("if( loaded ) {\n#if defined(__ANDROID__)", self.menu)
        self.assertIn("blueprint_cache::persist();", self.menu)


if __name__ == "__main__":
    unittest.main()
