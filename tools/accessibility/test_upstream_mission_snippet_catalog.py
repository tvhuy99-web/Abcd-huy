"""Ensure upstream mission dialogue references only installed speech snippet tags."""

import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
NPC = ROOT / "data" / "json" / "npcs"
TAGS = (
    "<job_offer>", "<multiple_job_offers>", "<more_job_offers>",
    "<one_more_job>", "<no_jobs>", "<no_more_jobs>",
)


class UpstreamMissionSnippetCatalog(unittest.TestCase):
    def test_mission_topics_use_installed_job_speech(self):
        topics = json.loads((NPC / "common_chat" / "TALK_COMMON_MISSION.json").read_text(encoding="utf-8"))
        snippets = json.loads((NPC / "snippets" / "talk_tags_line.json").read_text(encoding="utf-8"))
        mission = next(topic for topic in topics if topic.get("id") == "TALK_MISSION_LIST")
        available = {row["category"]: row for row in snippets if row.get("type") == "snippet"}
        dialogue = json.dumps(mission["dynamic_line"])
        for tag in TAGS:
            with self.subTest(tag=tag):
                self.assertIn(tag, dialogue)
                self.assertIn(tag, available)
                self.assertTrue(available[tag]["text"])
                self.assertTrue(all(isinstance(value, str) and value for value in available[tag]["text"]))


if __name__ == "__main__":
    unittest.main()
