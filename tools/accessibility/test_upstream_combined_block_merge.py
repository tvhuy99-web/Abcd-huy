"""Guard combined melee defenses and Android speech hooks after upstream merge."""

from pathlib import Path
import unittest

SRC = Path(__file__).resolve().parents[2] / "src"


class UpstreamCombinedBlockMergeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.character = (SRC / "character.h").read_text(encoding="utf-8")
        cls.creature = (SRC / "creature.cpp").read_text(encoding="utf-8")
        cls.melee = (SRC / "melee.cpp").read_text(encoding="utf-8")
        cls.monster = (SRC / "monster.cpp").read_text(encoding="utf-8")
        cls.special = (SRC / "mattack_actors.cpp").read_text(encoding="utf-8")
        cls.map_header = (SRC / "map.h").read_text(encoding="utf-8")

    def test_character_block_state_and_both_block_implementations(self):
        for phrase in ("float block_roll( const Creature *source ) override;",
                       "bodypart_id block_bp;",
                       "std::string thing_blocked_with;",
                       "bool block_item = false;"):
            with self.subTest(phrase=phrase):
                self.assertIn(phrase, self.character)
        self.assertIn("float Character::block_roll( const Creature *source )",
                      self.melee)
        self.assertIn("float monster::block_roll( const Creature * )",
                      self.monster)

    def test_dodge_and_block_both_reduce_melee_hit_spread(self):
        for phrase in ("int Creature::deal_melee_attack( Creature *source, int hitroll, bool allow_block )",
                       "block = block_roll( source );",
                       "hitroll - dodge - block - size_melee_penalty()",
                       "last_dodge = dodge;",
                       "last_block = block;",
                       "last_hit_spread = hit_spread;"):
            with self.subTest(phrase=phrase):
                self.assertIn(phrase, self.creature)

    def test_special_attacks_respect_blockability_and_fork_speech_policy(self):
        self.assertIn(
            "target->deal_melee_attack( &z, melee::melee_hit_range( acc ), blockable )",
            self.special,
        )
        self.assertIn(
            "Messages::mute_combat_speech unrelated_special_attack( !target->is_avatar() );",
            self.special,
        )

    def test_blocking_and_dodging_feed_combat_reporting(self):
        self.assertIn("t.last_dodge + t.last_block", self.melee)
        self.assertIn("const float block_share = last_block / ( last_block + last_dodge );",
                      self.monster)
        self.assertIn("target.last_block > target.last_dodge", self.monster)

    def test_upstream_map_damage_header_contract(self):
        self.assertIn("// TODO: this function should not be using dozens of individual terrain IDs",
                      self.map_header)
        self.assertIn("bool hit_with_acid( const tripoint_bub_ms &p );",
                      self.map_header)


if __name__ == "__main__":
    unittest.main()
