"""Directional combat must reuse native attack mechanics, not movement."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class NativeBumpCombatGestures(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.handler = (SRC / "handle_action.cpp").read_text(encoding="utf-8")
        cls.avatar = (SRC / "avatar_action.cpp").read_text(encoding="utf-8")
        cls.world = (SRC / "a11y_world.cpp").read_text(encoding="utf-8")
        cls.nearby = (SRC / "surroundings_menu.cpp").read_text(encoding="utf-8")
        cls.android = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        cls.vietnamese = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")

    def test_combat_swipe_dispatches_only_native_attack_action(self):
        native = self.handler.split(
            'action.rfind( "A11Y_INTERNAL_COMBAT_ATTACK_", 0 ) == 0', 1
        )[1].split('A11Y_INTERNAL_VEHICLE_BEGIN', 1)[0]
        self.assertIn("act = ACTION_FIRE;", native)
        self.assertIn("combat_attack_direction = dir;", native)
        self.assertNotIn("ACTION_MOVE_", native)
        self.assertNotIn("ACTION_AUTOATTACK", native)
        self.assertNotIn("step_actions", native)
        fire = self.handler.split("static void fire()", 1)[1].split(
            "if( a11y::enabled() && combat_attack_direction < 0 )", 1)[0]
        self.assertIn("mode_accessible_melee( you, combat_attack_direction )", fire)
        self.assertIn("you.melee_attack( *target, true );", fire)
        self.assertIn("you.reach_attack( traj.back() );", fire)
        self.assertNotIn("avatar_action::move(", fire)

    def test_native_autoattack_is_not_overridden_by_hostile_only_melee(self):
        body = self.handler.split("case ACTION_AUTOATTACK:", 1)[1].split("default:", 1)[0]
        self.assertIn("avatar_action::autoattack( player_character, here );", body)
        self.assertNotIn("Creature::Attitude::HOSTILE", body)
        self.assertNotIn("No visible hostile creature in that direction", body)
        self.assertNotIn("player_character.melee_attack", body)
        self.assertNotIn("avatar_action::move(", body)

    def test_normal_game_controls_are_responsible_for_melee_or_walking(self):
        dispatch = self.handler.split("case ACTION_MOVE_FORTH:", 1)[1].split(
            "case ACTION_MOVE_DOWN:", 1
        )[0]
        self.assertIn("move_or_drive( act, player_character, weapon,", dispatch)
        movement = self.handler.split("void game::move_or_drive(", 1)[1].split(
            "bool game::do_regular_action(", 1
        )[0]
        self.assertIn("get_delta_from_movement_action( act, iso_rotate::yes )", movement)
        self.assertIn("avatar_action::move( player_character, here", movement)
        native = self.avatar.split("bool avatar_action::move(", 1)[1].split(
            "void avatar_action::swim(", 1
        )[0]
        self.assertIn("g->check_safe_mode_allowed()", native)
        self.assertIn("creatures.creature_at<monster>( dest_loc, true )", native)
        self.assertIn("you.melee_attack( critter, true )", native)
        self.assertIn("if( !np.is_enemy() )", native)
        self.assertIn("g->npc_menu( np );", native)
        self.assertIn("if( g->walk_move( dest_loc, via_ramp ) )", native)

    def test_reader_describes_actual_gun_or_melee_behavior(self):
        self.assertIn('_( "Attack %s" )', self.world)
        self.assertIn('_( "Fire %s" )', self.world)
        self.assertIn('!combat_held->gun_current_mode().melee()', self.world)
        self.assertIn("Never walk.", self.world)
        self.assertIn('"reader:combat:attack:" + std::to_string( i )', self.world)
        self.assertIn('requestCombatCommand("reader:combat:attack:" + direction)', self.android)
        for direction in range(8):
            self.assertIn(f"direction = {direction};", self.android)
        for sentence in (
            "Attack %s",
            "Fire %s",
            "Attack the nearest valid target in this direction within the current weapon range. Never walk.",
        ):
            self.assertIn('msgid "' + sentence + '"', self.vietnamese)
        self.assertNotIn('msgid "No visible hostile creature in that direction.', self.vietnamese)

    def test_nearby_direction_description_is_declared_before_use(self):
        # This is the C++ error that broke ARM64 compilation in run
        # 43193229259: a local lambda declared below the reader handler.
        definition = self.nearby.index("const auto relative_words = [](")
        handler = self.nearby.index('a11y::screen_scope reader( &ctxt, "surroundings"')
        first_use = self.nearby.index("relative_words( target.relative )")
        self.assertLess(definition, handler)
        self.assertLess(definition, first_use)


if __name__ == "__main__":
    unittest.main()
