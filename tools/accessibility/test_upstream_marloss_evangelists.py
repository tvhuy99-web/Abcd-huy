"""Guard self-contained upstream Marloss evangelist NPC definitions."""

import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
MARLOSS = ROOT / "data" / "json" / "npcs" / "marloss"


class UpstreamMarlossEvangelists(unittest.TestCase):
    def test_new_evangelist_chat_and_classes(self):
        cases = (
            ("MARLOSS_2_Sera_Nameko.json", "marloss_evan_2"),
            ("MARLOSS_3_Amaides.json", "marloss_evan_3"),
        )
        for filename, npc_id in cases:
            with self.subTest(filename=filename):
                records = json.loads((MARLOSS / filename).read_text(encoding="utf-8"))
                character = next(x for x in records if x.get("type") == "npc" and x.get("id") == npc_id)
                topics = set()
                for item in records:
                    if item.get("type") == "talk_topic":
                        topic_id = item["id"]
                        topics.update(topic_id if isinstance(topic_id, list) else [topic_id])
                classes = {r["id"] for r in records if r.get("type") == "npc_class"}
                self.assertIn(character["chat"], topics)
                self.assertIn(character["class"], classes)


if __name__ == "__main__":
    unittest.main()
