"""Guard upstream crafting fault inheritance and Android blueprint caching."""

from pathlib import Path
import unittest


SRC = Path(__file__).resolve().parents[2] / "src" / "recipe.cpp"


class UpstreamRecipeFaultsTest(unittest.TestCase):
    def test_result_inherits_supported_component_faults(self):
        code = SRC.read_text(encoding="utf-8")
        result = code.split("std::vector<item> recipe::create_result(", 1)[1]
        result = result.split("std::vector<item> recipe::create_results(", 1)[0]
        self.assertIn("newit.inherit_flags( *used, *this );", result)
        self.assertIn("for( auto &pair : *used )", result)
        self.assertIn("for( item &it : pair.second )", result)
        self.assertIn("for( fault_id some_fault : it.faults )", result)
        self.assertIn("newit.set_fault( some_fault );", result)
        self.assertLess(result.index("newit.set_fault( some_fault );"),
                        result.index("for( const flag_id &flag : flags_to_delete )"))

    def test_fork_blueprint_cache_and_practice_labels_remain(self):
        code = SRC.read_text(encoding="utf-8")
        for marker in ("blueprint_cache::find(", "blueprint_cache::store(",
                       "practice_source_name", "recipe::legacy_byproducts()"):
            with self.subTest(marker=marker):
                self.assertIn(marker, code)


if __name__ == "__main__":
    unittest.main()
