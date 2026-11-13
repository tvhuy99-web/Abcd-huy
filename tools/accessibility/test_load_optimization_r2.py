"""Structural safeguards for Android load optimization round two."""
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "src"


class LoadOptimizationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mapgen = (SRC / "mapgen.cpp").read_text(encoding="utf-8")
        cls.game = (SRC / "game.cpp").read_text(encoding="utf-8")
        cls.fault = (SRC / "fault.cpp").read_text(encoding="utf-8")
        cls.items = (SRC / "item_factory.cpp").read_text(encoding="utf-8")
        cls.vehicle = (SRC / "veh_type.cpp").read_text(encoding="utf-8")
        cls.recipe = (SRC / "recipe.cpp").read_text(encoding="utf-8")

    def test_saved_character_only_uses_deferred_mapgen(self):
        self.assertIn(
            "set_android_deferred_oter_mapgen_setup( "
            "loading_saved_character );", self.game)
        self.assertIn("std::call_once( first_use", self.mapgen)
        self.assertIn("oter_mapgen.setup();", self.mapgen)
        self.assertIn("iter->second.ensure_prepared();", self.mapgen)

    def test_original_requirement_finalization_remains(self):
        self.assertIn("if( !fault_requirements_finalized )", self.fault)
        self.assertIn("requirement_data::finalize();", self.fault)

    def test_item_material_index_is_rebuilt_each_load(self):
        self.assertIn("repair_materials_by_tool materials_by_tool;", self.items)
        self.assertIn("allowed.insert( opts.begin(), opts.end() );", self.items)
        self.assertIn("materials_by_tool );", self.items)

    def test_vehicle_computation_has_fallback(self):
        self.assertIn("fuel_fits_cache.emplace", self.vehicle)
        self.assertIn("#else\n            const bool fuel_compatible", self.vehicle)

    def test_json_signature_still_hashes_source_bytes(self):
        self.assertIn("mix( cache.signature, chunk.data()", self.recipe)
        self.assertIn("cache.hash_us +=", self.recipe)


if __name__ == "__main__":
    unittest.main()
