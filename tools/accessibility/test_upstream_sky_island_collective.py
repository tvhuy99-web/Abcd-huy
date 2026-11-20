"""Keep Sky Island collective-center raid protection after upstream merges."""

import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class UpstreamSkyIslandCollective(unittest.TestCase):
    def test_collective_center_is_not_a_raid_spawn(self):
        path = ROOT / "data/mods/Sky_Island/mapgen/faction_bases.json"
        bases = json.loads(path.read_text(encoding="utf-8"))
        by_id = {row["id"]: row for row in bases}
        self.assertEqual(len(bases), len(by_id))
        collective = by_id["collective center"]
        self.assertEqual(collective["copy-from"], "collective center")
        self.assertIn("SKY_ISLAND_NO_RAID_SPAWN", collective["extend"]["flags"])
        self.assertIn("Isherwood Farm Mutable", by_id)


if __name__ == "__main__":
    unittest.main()
