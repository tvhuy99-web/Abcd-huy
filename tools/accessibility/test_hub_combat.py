"""Accessibility regressions for fast, non-modal combat feedback."""

import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "src"


class CombatHubTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.avatar = (SRC / "avatar_action.cpp").read_text("utf-8")
        cls.actions = (SRC / "handle_action.cpp").read_text("utf-8")

    def test_autoattack_reports_actual_chosen_target_without_prompt(self):
        body = self.avatar.split(
            "void avatar_action::autoattack(", 1
        )[1].split("bool avatar_action::can_fire_weapon(", 1)[0]
        self.assertIn("std::max_element( critters.begin()", body)
        self.assertIn("best.disp_name()", body)
        self.assertIn("direction_name( direction_from(", body)
        self.assertIn("rl_dist( you.pos_bub(), best.pos_bub() )", body)
        self.assertIn("Autoattack: no target in reach.", body)
        self.assertIn("you.pause();", body)
        self.assertNotIn("query_yn(", body)
        self.assertLess(body.index("best.disp_name()"), body.index(
            "move( you, m, tripoint_rel_ms("))

    def test_fire_modes_report_shots_without_melee_mislabeling(self):
        burst = self.actions.split(
            "case ACTION_FIRE_BURST:", 1
        )[1].split("case ACTION_SELECT_DEFAULT_AMMO:", 1)[0]
        self.assertIn("Burst mode:", burst)
        self.assertIn("Firing mode:", burst)
        self.assertIn("mode.qty", burst)
        self.assertIn("mode.melee()", burst)
        self.assertIn("no burst or automatic firing mode.", burst)
        self.assertIn("gun_cycle_mode();", burst)
        self.assertIn("avatar_action::fire_wielded_weapon(", burst)
        self.assertNotIn("query_yn(", burst)

    def test_recast_is_one_action_with_spoken_cost_and_time(self):
        body = self.actions.split(
            "static void cast_spell(", 1
        )[1].split("bool Character::cast_spell(", 1)[0]
        self.assertIn("No previous spell to repeat.", body)
        self.assertIn("sp.energy_cost_string(", body)
        self.assertIn("sp.casting_time(", body)
        self.assertIn("time_duration::from_moves(", body)
        self.assertIn("player_character.cast_spell(", body)
        self.assertNotIn("query_yn(", body)

    def test_safe_mode_and_ignore_actions_are_concise(self):
        safe = self.actions.split(
            "case ACTION_TOGGLE_AUTOSAFE:", 1
        )[1].split("case ACTION_WORKOUT:", 1)[0]
        self.assertIn("Automatic safe mode on.", safe)
        self.assertIn("Automatic safe mode off.", safe)
        self.assertIn("Enemy warning ignored for now.", safe)
        self.assertIn("No active enemy warning to ignore.", safe)
        self.assertIn("Future warnings for this type may be skipped.", safe)
        self.assertNotIn("query_yn(", safe)

    def test_upstream_combined_defense_keeps_accessible_combat_messages(self):
        melee = (SRC / "melee.cpp").read_text("utf-8")
        monster = (SRC / "monster.cpp").read_text("utf-8")
        creature = (SRC / "creature.cpp").read_text("utf-8")
        monster_attacks = (SRC / "mattack_actors.cpp").read_text("utf-8")

        self.assertIn("float Character::block_roll(", melee)
        self.assertIn("const float block_share = last_block / ( last_block + last_dodge );", melee)
        self.assertIn("t.last_dodge + t.last_block", melee)
        self.assertIn("last_dodge = dodge;", creature)
        self.assertIn("last_block = block;", creature)
        self.assertIn("last_hit_spread = hit_spread;", creature)
        self.assertIn("melee_hit_range( acc ), blockable", monster_attacks)
        self.assertIn("float monster::block_roll(", monster)
        # Keep existing native narration and distinguish blocked/dodged hits.
        self.assertIn("Messages::mute_combat_speech", monster)
        self.assertIn("a11y::android::announce(", monster)
        self.assertIn('_( "You block %s." )', monster)
        self.assertIn('_( "You dodge %s." )', monster)

    def test_upstream_combined_defense_feedback_is_localized(self):
        import re

        catalog = (SRC.parent / "lang/po/accessibility/vi.po").read_text(
            encoding="utf-8")
        melee = (SRC / "melee.cpp").read_text("utf-8")
        monster = (SRC / "monster.cpp").read_text("utf-8")
        messages = (
            "You block %1$s of the attack with your %2$s!",
            "<npcname> blocks %1$s of the attack with their %2$s!",
            "You parry %1$s of the attack with your %2$s!",
            "<npcname> parries %1$s of the attack with their %2$s!",
            "The %s blocks the attack!",
            "The %s parries the attack!",
            "You block %s.",
            "You dodge %s.",
            "%1$s blocks %2$s attack.",
            "%1$s dodges %2$s attack.",
        )
        for message in messages:
            with self.subTest(message=message):
                self.assertTrue(
                    any('_( "' + message + '" )' in source
                        for source in (melee, monster)))
                entry = '#, c-format\\nmsgid "' + message + '"\\nmsgstr "'
                self.assertIn(entry, catalog)
                translated = catalog.split(entry, 1)[1].split('"', 1)[0]
                self.assertTrue(translated.strip())
                placeholders = lambda text: sorted(
                    re.findall(r"%(?:\\d+\\$)?[sdif]", text))
                self.assertEqual(placeholders(message), placeholders(translated))

    def test_throw_wielded_never_gets_an_extra_confirmation(self):
        throwing = self.actions.split(
            "case ACTION_THROW_WIELDED:", 1
        )[1].split("case ACTION_FIRE:", 1)[0]
        self.assertIn("avatar_action::plthrow_wielded(", throwing)
        self.assertNotIn("query_yn(", throwing)


if __name__ == "__main__":
    unittest.main()
