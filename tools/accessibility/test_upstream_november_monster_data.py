"""Contract for synchronized upstream monster blocking and Sky Island equipment."""

import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
MONSTERS = ROOT / "data" / "json" / "monsters"


class UpstreamNovemberMonsterData(unittest.TestCase):
    def test_new_monster_block_stats(self):
        cases = {
            "feral_humans.json": ("mon_feral_sapien_spear", "mon_feral_survivalist"),
            "mutant.json": ("mon_mutant_experimental", "mon_mutant_alpha"),
            "nether.json": ("mon_gozu",),
        }
        for filename, ids in cases.items():
            with self.subTest(filename=filename):
                entries = json.loads((MONSTERS / filename).read_text(encoding="utf-8"))
                monsters = {entry.get("id"): entry for entry in entries}
                for mid in ids:
                    self.assertIn(mid, monsters)
                    block = monsters[mid]["block"]
                    self.assertGreater(block["chance"], 0)
                    self.assertGreater(block["effectiveness"], 0)
                    self.assertGreater(block["count"], 0)
                    self.assertIs(block["ranged"], False)
        monster_cpp = (ROOT / "src" / "monster.cpp").read_text(encoding="utf-8")
        self.assertIn("type->block.chance", monster_cpp)
        self.assertIn("type->block.effectiveness", monster_cpp)

    def test_sky_island_clothes_fit(self):
        path = ROOT / "data" / "mods" / "Sky_Island" / "items.json"
        entries = json.loads(path.read_text(encoding="utf-8"))
        kit = next(item for item in entries if item.get("id") == "starterkit_items")
        clothing = ("pants", "tshirt", "socks", "sneakers")
        for name in clothing:
            with self.subTest(item=name):
                row = next(item for item in kit["entries"] if item.get("item") == name)
                self.assertIn("FIT", row.get("custom-flags", []))


if __name__ == "__main__":
    unittest.main()
