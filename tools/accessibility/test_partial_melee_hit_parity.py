"""Prevent the player melee path from rejecting upstream partial hits."""

from pathlib import Path
import unittest

SRC = Path(__file__).resolve().parents[2] / "src"


class PartialMeleeHitParity(unittest.TestCase):
    def test_normal_and_mutation_attacks_use_shared_hit_predicate(self):
        melee = (SRC / "melee.cpp").read_text("utf-8")
        creature = (SRC / "creature.cpp").read_text("utf-8")
        monster = (SRC / "monster.cpp").read_text("utf-8")
        self.assertIn("const bool hits = Creature::melee_attack_hit( hit_spread );", melee)
        self.assertIn("if( Creature::melee_attack_hit( hit_spread ) )", melee)
        self.assertNotIn("hit_spread >= 0", melee)
        self.assertIn("return melee_damage_mult( hit_spread ) > 0.0f;", creature)
        self.assertIn("Creature::melee_attack_hit( hitspread )", monster)

    def test_hit_quality_kept_in_native_log_and_accessibility_summary_preserved(self):
        melee = (SRC / "melee.cpp").read_text("utf-8")
        self.assertIn("dealt_dam.wp_hit, hit_spread );", melee)
        self.assertIn("Creature::hit_quality_label( hit_spread )", melee)
        self.assertIn("Messages::mute_combat_speech original_melee_hit", melee)
        self.assertIn("Hit %1$s. %2$d damage. %3$d HP remaining.", melee)

    def test_paid_dodge_uses_upstream_combat_exertion(self):
        character = (SRC / "character.cpp").read_text("utf-8")
        dodge = character.split("void Character::on_try_dodge()", 1)[1].split(
            "void Character::on_dodge(", 1)[0]
        self.assertIn("set_activity_level( COMBAT_EXERCISE );", dodge)
        self.assertNotIn("set_activity_level( EXTRA_EXERCISE );", dodge)

    def test_melee_and_reach_use_upstream_activity_level(self):
        melee = (SRC / "melee.cpp").read_text("utf-8")
        self.assertEqual(melee.count("// Fighting is hard work\n    set_activity_level( COMBAT_EXERCISE );"), 2)


if __name__ == "__main__":
    unittest.main()
