"""Directional combat selects native legal targets and never moves the character."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


class DirectionalCombatRangeTests(unittest.TestCase):
    def test_native_effective_range_including_mods_bayonets_and_unarmed(self):
        ranged = (SRC / "ranged.cpp").read_text(encoding="utf-8")
        melee = ranged.split("target_handler::mode_accessible_melee(", 1)[1].split(
            "target_handler::mode_turret_manual(", 1)[0]
        self.assertIn("you.used_weapon()", melee)
        self.assertIn("weapon->current_reach_range( you ).first", melee)
        self.assertIn("MELEE_RANGE_MODIFIER", melee)
        self.assertIn("you.get_wielded_item() ? 1", melee)
        self.assertIn("gun.target->gun_range( &you )", ranged)

    def test_nearest_native_target_on_exact_ray_no_empty_fallback(self):
        ranged = (SRC / "ranged.cpp").read_text(encoding="utf-8")
        pick = ranged.split("tripoint_bub_ms target_ui::directional_combat_target(", 1)[1].split(
            "tripoint_bub_ms target_ui::directional_shot_target(", 1)[0]
        self.assertIn("for( int step = 1; step <= range; ++step )", pick)
        self.assertIn("tripoint( step * direction, 0 )", pick)
        self.assertIn("dist_fn( tile ) > range", pick)
        self.assertIn("std::find( targets.begin(), targets.end(), tile )", pick)
        self.assertIn("return src;", pick)
        self.assertNotIn("return fallback;", pick)
        self.assertIn("get_targetable_creatures( range, mode == TargetMode::Reach )", ranged)
        self.assertIn("automatic_combat_shot && !prompt_friendlies_in_lof()", ranged)

    def test_no_target_or_refused_confirmation_never_executes_melee(self):
        handler = (SRC / "handle_action.cpp").read_text(encoding="utf-8")
        body = handler.split("mode_accessible_melee( you, combat_attack_direction )", 1)[1].split(
            "if( a11y::enabled() && combat_attack_direction < 0 )", 1)[0]
        self.assertLess(body.index("if( traj.empty() )"), body.index("you.melee_attack("))
        self.assertLess(body.index("if( target == nullptr )"), body.index("you.reach_attack("))
        self.assertNotIn("avatar_action::move(", body)
        ranged = (SRC / "ranged.cpp").read_text(encoding="utf-8")
        select = ranged.split("if( mode == TargetMode::Reach && accessible_melee_direction )", 1)[1].split(
            "if( mode == TargetMode::Reach && targets.empty() )", 1)[0]
        self.assertIn("!confirm_non_enemy_target()", select)
        self.assertIn("return {};", select)
        self.assertIn("return traj;", select)


if __name__ == "__main__":
    unittest.main()
