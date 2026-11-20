"""Validate upstream Nether and Gozu Hell map content imported as one bundle."""

import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

DATA_FILES = (
    "data/json/achievements.json",
    "data/json/effects_on_condition/condition_eocs_for_mods.json",
    "data/json/effects_on_condition/nether_eocs/dimensions.json",
    "data/json/furniture_and_terrain/terrain-nether.json",
    "data/json/furniture_and_terrain/terrain-walls.json",
    "data/json/mapgen/gozu_hell.json",
    "data/json/mapgen/map_extras/gozu_hell.json",
    "data/json/mapgen/netherum/labyrinth_stairs_teleporters.json",
    "data/json/monster_special_attacks/spells.json",
    "data/json/monstergroups/nether.json",
    "data/json/monsters/nether.json",
    "data/json/overmap/map_extras_dimensions.json",
    "data/json/overmap/overmap_special/dimensions_misc.json",
    "data/json/overmap/overmap_special/gozu_hell.json",
    "data/json/overmap/overmap_terrain/overmap_terrain_gozu_hell.json",
    "data/json/overmap/special_locations.json",
    "data/json/region_settings/region_settings/dimensions/dimension_regions.json",
    "data/json/region_settings/region_settings/dimensions/dimensions.json",
    "data/json/region_settings/region_settings/dimensions/gozu_hell.json",
    "data/json/traps.json",
    "data/mods/MindOverMatter/effectoncondition/conditions_for_eocs.json",
    "data/mods/MindOverMatter/monsters/monster_eoc_spells.json",
    "data/mods/TEST_DATA/overmap_terrain_coverage_test/overmap_terrain_coverage_whitelist.json",
)

class UpstreamGozuHellMapTest(unittest.TestCase):
    def test_all_synced_json_catalogs_parse(self):
        for path in DATA_FILES:
            with self.subTest(path=path):
                rows = json.loads((ROOT / path).read_text(encoding="utf-8"))
                self.assertIsInstance(rows, (list, dict))
                self.assertTrue(rows)

    def test_new_overmap_terrain_matches_mapgen(self):
        terrain = json.loads((ROOT / "data/json/overmap/overmap_terrain/overmap_terrain_gozu_hell.json").read_text(encoding="utf-8"))
        generated = json.loads((ROOT / "data/json/mapgen/gozu_hell.json").read_text(encoding="utf-8"))
        available = {row["id"] for row in terrain if row.get("type") == "overmap_terrain"}
        referenced = {terrain_id for row in generated for terrain_id in row.get("om_terrain", [])}
        self.assertTrue(referenced)
        self.assertTrue(referenced.issubset(available))

    def test_dimension_region_and_specials_are_present(self):
        region = json.loads((ROOT / "data/json/region_settings/region_settings/dimensions/gozu_hell.json").read_text(encoding="utf-8"))
        specials = json.loads((ROOT / "data/json/overmap/overmap_special/dimensions_misc.json").read_text(encoding="utf-8"))
        self.assertTrue(any(row.get("id") == "gozu_hell" and row.get("type") == "region_settings"
                            for row in region))
        self.assertTrue(any(row.get("id") == "gozu_hell_lava_pool" for row in specials))


if __name__ == "__main__":
    unittest.main()
