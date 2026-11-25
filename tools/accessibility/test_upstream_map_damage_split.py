"""Keep upstream map damage split link-safe without losing Android map narration."""

from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
DAMAGE_DEFINITIONS = (
    "map::get_map_damage",
    "map::set_map_damage",
    "map_common_bash_info::damage_to",
    "map_common_bash_info::hp",
    "map::bash_rating_internal",
    "map::is_bashable",
    "map::is_bashable_ter",
    "map::is_bashable_furn",
    "map::is_bashable_ter_furn",
    "map::bash_strength",
    "map::bash_resistance",
    "map::bash_rating",
    "map::smash_items",
    "furn_is_supported",
    "map::bash_ter_furn",
    "map::bash",
    "map::bash_items",
    "map::bash_vehicle",
    "map::bash_field",
    "map::drop_bash_results",
    "map::destroy",
    "map::destroy_furn",
    "map::destroy_vehicle",
    "map::batter",
    "map::crush",
    "map::shoot",
    "map::hit_with_acid",
    "map::hit_with_fire",
)


class UpstreamMapDamageSplitTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.core = (SRC / "map.cpp").read_text(encoding="utf-8")
        cls.damage = (SRC / "map_damage.cpp").read_text(encoding="utf-8")
        cls.cache = (SRC / "map_cache.cpp").read_text(encoding="utf-8")

    def test_no_duplicate_damage_definitions(self):
        for name in DAMAGE_DEFINITIONS:
            pattern = rf"(?m)^[^\s\n][^\n]*\b{re.escape(name)}\s*\("
            with self.subTest(name=name):
                self.assertEqual(len(re.findall(pattern, self.core)), 0)
                self.assertEqual(len(re.findall(pattern, self.damage)), 2 if name == "map::bash" else 1)

    def test_original_terrain_and_reader_cache_still_work(self):
        self.assertIn("clear_original_terrain_at( p );", self.damage)
        self.assertIn("map::clear_original_terrain_at(", self.core)
        self.assertIn("a11y::android::", self.core)
        self.assertIn("a11y::android::performance_profiling_enabled()", self.cache)
        self.assertIn('diagnostic_event( "WORLD_CACHE_PHASE"', self.cache)

    def test_new_cpp_compiled_by_existing_globs(self):
        cmake = (SRC / "CMakeLists.txt").read_text(encoding="utf-8")
        make = (ROOT / "Makefile").read_text(encoding="utf-8")
        self.assertIn("file(GLOB CATACLYSM_DDA_SOURCES", cmake)
        self.assertIn("$(wildcard $(SRC_DIR)/*.cpp)", make)


if __name__ == "__main__":
    unittest.main()
