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
        self.assertIn("moves_to_string(", body)
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

    def test_throw_wielded_never_gets_an_extra_confirmation(self):
        throwing = self.actions.split(
            "case ACTION_THROW_WIELDED:", 1
        )[1].split("case ACTION_FIRE:", 1)[0]
        self.assertIn("avatar_action::plthrow_wielded(", throwing)
        self.assertNotIn("query_yn(", throwing)


if __name__ == "__main__":
    unittest.main()
