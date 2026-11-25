"""Protect old Lakeside Cabin world IDs across the upstream overmap merge."""

import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "data" / "json"


def read(path):
    return json.loads((DATA / path).read_text(encoding="utf-8"))


class UpstreamLakesideWorldMigration(unittest.TestCase):
    def test_old_cabin_specials_migrate_to_unified_id(self):
        rules = read("obsoletion_and_migration_0.K/overmap_special.json")
        mapping = {entry["id"]: entry["new_id"] for entry in rules}
        specials = read("overmap/overmap_special/specials.json")
        names = {entry.get("id") for entry in specials if isinstance(entry.get("id"), str)}
        self.assertIn("Lakeside Cabin", names)
        for number in range(1, 6):
            with self.subTest(number=number):
                old = f"Lakeside Cabin {number}"
                self.assertEqual(mapping[old], "Lakeside Cabin")
                self.assertNotIn(old, names)

    def test_old_cabin_terrain_ids_have_migration(self):
        rules = read("obsoletion_and_migration_0.K/overmap_terrain.json")
        self.assertEqual(len(rules), 1)
        mapping = rules[0]["oter_ids"]
        terrain = read("overmap/overmap_terrain/overmap_terrain.json")
        registered = {entry.get("id") for entry in terrain if isinstance(entry.get("id"), str)}
        self.assertIn("cabin_lakeside", registered)
        self.assertIn("cabin_lakeside_roof", registered)
        for number in range(1, 6):
            for heading in ("north", "east", "south", "west"):
                with self.subTest(number=number, heading=heading):
                    self.assertIn(f"cabin_lakeside{number}_{heading}", mapping)
                    self.assertIn(f"cabin_lakeside{number}_roof_{heading}", mapping)

    def test_mapgen_and_optional_map_color_use_unified_id(self):
        mapgen = read("mapgen/lake_buildings/lakeside_cabin.json")
        self.assertTrue(any(entry.get("om_terrain") == "cabin_lakeside" for entry in mapgen))
        alt_path = ROOT / "data" / "mods" / "alt_map_key" / "overmap_terrain_brown.json"
        alt = json.loads(alt_path.read_text(encoding="utf-8"))
        self.assertTrue(any(entry.get("id") == "cabin_lakeside" for entry in alt))


if __name__ == "__main__":
    unittest.main()
