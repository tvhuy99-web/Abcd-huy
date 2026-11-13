"""Check that the Android load profiler covers the known bottlenecks."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


class LoadProfileDetailsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.recipe = (SRC / "recipe_dictionary.cpp").read_text("utf-8")
        cls.mapgen = (SRC / "mapgen.cpp").read_text("utf-8")
        cls.init = (SRC / "init.cpp").read_text("utf-8")

    def test_existing_total_group_timings_stay_available(self):
        self.assertIn("A11Y_LOAD_GROUP phase=finalize", self.init)
        self.assertIn("A11Y_LOAD_DETAIL phase=recipe", self.recipe)
        self.assertIn("A11Y_LOAD_DETAIL phase=mapgen", self.mapgen)

    def test_recipe_subphases_and_limited_slow_sample(self):
        for stage in (
            "deferred", "remove_abstract", "finalize_craft_and_prune",
            "finalize_uncraft_and_prune", "book_links_and_reversible",
            "pseudo_uncraft_books", "nested_category_search",
            "build_lookup_indices", "food_loops"
        ):
            self.assertIn('log_step( "' + stage + '" )', self.recipe)
        # The final total is emitted directly with its absolute elapsed time.
        self.assertIn("A11Y_LOAD_DETAIL phase=recipe step=total", self.recipe)
        self.assertIn("cleanup_ms=", self.recipe)
        self.assertIn("finalize_ms=", self.recipe)
        self.assertIn("input_ms=", self.recipe)
        self.assertIn("step=slow_recipe", self.recipe)
        self.assertIn("slowest.size()", self.recipe)
        self.assertIn("nested_comparisons", self.recipe)
        self.assertIn("step=progress kind=", self.recipe)
        self.assertIn("graph_ms=", self.recipe)
        self.assertIn("detect_ms=", self.recipe)

    def test_mapgen_phases_and_bounded_slowest_group(self):
        for stage in ("oter_setup", "oter_parameters", "total"):
            self.assertIn("step=" + stage, self.mapgen)
        for stage in (
            "nested_setup", "update_setup",
            "nested_parameters", "update_parameters"
        ):
            self.assertIn('profile_groups( "' + stage + '"', self.mapgen)
        self.assertIn("slowest_group=", self.mapgen)
        self.assertIn("step=oter_setup_progress", self.mapgen)
        self.assertIn("functions=", self.mapgen)

    def test_profiler_is_android_only_and_leaves_logic_intact(self):
        self.assertIn("#if defined(__ANDROID__)", self.recipe)
        self.assertIn("#if defined(__ANDROID__)", self.mapgen)
        self.assertIn("elem.second.finalize();", self.recipe)
        self.assertIn("inp_mngr.pump_events();", self.recipe)
        self.assertIn("recipe_dict.find_items_on_loops();", self.recipe)
        self.assertIn("ptr.first->setup();", self.mapgen)
        self.assertIn("ptr->finalize_parameters();", self.mapgen)


if __name__ == "__main__":
    unittest.main()
