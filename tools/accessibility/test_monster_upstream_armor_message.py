"""Guard upstream NPC armor wording and fork's accessible player cues."""

from pathlib import Path
import unittest


SOURCE = Path(__file__).resolve().parents[2] / "src" / "monster.cpp"


class MonsterArmorMessageIntegration(unittest.TestCase):
    def test_npc_blocked_hit_matches_upstream(self):
        text = SOURCE.read_text(encoding="utf-8")
        self.assertIn(
            "%1$s hits %2$s in the %3$s but is stopped by the %4$s.", text
        )
        self.assertNotIn(
            "%1$s hits %2$s %3$s but is stopped by %4$s %5$s.", text
        )

    def test_avatar_spoken_armor_and_combat_filters_remain(self):
        text = SOURCE.read_text(encoding="utf-8")
        self.assertIn("Armor blocks the damage.", text)
        self.assertIn("Messages::mute_combat_speech", text)
        self.assertIn("target.is_avatar()", text)


if __name__ == "__main__":
    unittest.main()
