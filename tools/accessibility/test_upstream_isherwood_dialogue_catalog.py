"""Keep synchronized Isherwood rescue dialogue files parseable and populated."""

import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
NPCS = ROOT / "data" / "json" / "npcs" / "isherwood_farm"
FILES = (
    "Isherwood_Rescue_NPC_Duplicates.json",
    "NPC_Barry_Isherwood.json",
    "NPC_Carlos_Isherwood.json",
    "NPC_Chris_Isherwood.json",
    "NPC_Claire_Isherwood.json",
    "NPC_Eddie_Isherwood.json",
    "NPC_Jack_Isherwood.json",
    "NPC_Jesse_Isherwood.json",
    "NPC_Lisa_Isherwood.json",
    "NPC_Luke_Isherwood.json",
)


class UpstreamIsherwoodDialogueCatalog(unittest.TestCase):
    def test_all_rescue_dialogue_files_remain_valid(self):
        for filename in FILES:
            with self.subTest(filename=filename):
                records = json.loads((NPCS / filename).read_text(encoding="utf-8"))
                self.assertIsInstance(records, list)
                self.assertGreater(len(records), 1)
                self.assertTrue(all(isinstance(r, dict) and r.get("type") for r in records))

    def test_chris_rescue_dialogue_is_present(self):
        records = json.loads((NPCS / "NPC_Chris_Isherwood.json").read_text(encoding="utf-8"))
        self.assertTrue(any(r.get("id") == "TALK_ISHERWOOD_CHRIS_RESCUE_BARRY_PLAN"
                            for r in records))


if __name__ == "__main__":
    unittest.main()
