"""Upstream #89643 must fix learning bonus in BOTH the game UI and Android reader."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PLAYER_DISPLAY = ROOT / "src/player_display.cpp"


class RustySkillBonusDisplayTest(unittest.TestCase):
    def test_raw_experience_zero_guard_in_both_skill_descriptions(self):
        source = PLAYER_DISPLAY.read_text(encoding="utf-8")
        self.assertEqual(
            source.count("const int knowledge_xp = level.knowledgeExperience( true );"), 2
        )
        self.assertEqual(source.count("if( knowledge_xp > 0 ) {"), 2)
        self.assertEqual(source.count("level.exercise( true ) / knowledge_xp"), 2)
        self.assertEqual(source.count("float learning_bonus = 100.0f;"), 2)
        self.assertNotIn("level.exercise() / level.knowledgeExperience()", source)

    def test_accessible_skill_details_still_use_repaired_value(self):
        source = PLAYER_DISPLAY.read_text(encoding="utf-8")
        reader = source.split('node.id = "pd:skill:"', 1)[0]
        self.assertIn('node.detail = remove_color_tags( info_text );', source)
        self.assertIn('_( "%s| Learning bonus: %.0f%%" )', reader)
        self.assertIn("a11y::semantic_node node;", reader)


if __name__ == "__main__":
    unittest.main()
