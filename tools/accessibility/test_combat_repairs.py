"""Source-level regression contracts for accessible quick combat and reloads.

The Android touch instrumentation covers recognition; native smoke/CI must
still compile and run the engine. These checks protect the integration path.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
ANDROID = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
INSTRUMENTATION = ROOT / "android/app/src/androidTest/java/com/cleverraven/cataclysmdda"


def load(path):
    return path.read_text(encoding="utf-8")


class CombatRepairContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.handler = load(ROOT / "src/handle_action.cpp")
        cls.ranged = load(ROOT / "src/ranged.cpp")
        cls.world = load(ROOT / "src/a11y_world.cpp")
        cls.game = load(ROOT / "src/game.cpp")
        cls.activity = load(ROOT / "src/activity_actor.cpp")
        cls.adapter = load(ROOT / "src/a11y_input.cpp")
        cls.controller = load(ANDROID / "GameReaderController.java")
        cls.po = load(ROOT / "lang/po/accessibility/vi.po")

    def test_quick_fire_checks_real_gunmode_before_opening_aim(self):
        fire = self.handler.split("static void fire()", 1)[1].split(
            "// try firing turrets", 1)[0]
        quick = fire.split("if( combat_attack_direction >= 0 ) {", 1)[-1]
        self.assertIn("gunmode_checks_common(", quick)
        self.assertIn("gunmode_checks_weapon(", quick)
        self.assertIn("firing_mode_blocked(", quick)
        self.assertIn("rounds loaded. Reload or change weapons.", quick)
        self.assertLess(quick.index("gunmode_checks_weapon("),
                        quick.index("queue_accessible_combat_shot("))
        self.assertLess(quick.index("gunmode_checks_common("),
                        quick.index("queue_accessible_combat_shot("))

    def test_disabled_targeting_direction_never_promises_shot(self):
        view = self.ranged.split('direction.id = "target:fire-direction:"', 1)[1]
        view = view.split("snapshot.nodes.push_back( std::move( direction ) );", 1)[0]
        self.assertIn("if( !direction.enabled )", view)
        self.assertIn("direction.disabled_reason", view)
        self.assertIn("Ammunition remaining: %d.", view)
        self.assertLess(view.index("if( !direction.enabled )"),
                        view.index("No visible creature. You can still fire"))

    def test_important_combat_warning_waits_for_stable_world_context(self):
        self.assertIn("A11Y_COMBAT_ALERT:", self.ranged)
        self.assertIn("a11y::android::announce(", self.ranged)
        self.assertIn("COMBAT_ALERT_PREFIX", self.controller)
        self.assertIn("pendingGameMessages.peekFirst()", self.controller)
        self.assertIn('!"DEFAULTMODE".equals(snapshot.category)', self.controller)
        self.assertIn("replayBufferedGameMessages()", self.controller)
        self.assertIn("speech.setContext(next.contextKey());", self.controller)

    def test_activity_distraction_back_is_explicit_safe_no(self):
        adapter = self.adapter.split('if( intent.action == "cancel" )', 1)[1]
        self.assertIn('context.category_id() == "CANCEL_ACTIVITY_OR_IGNORE_QUERY"', adapter)
        self.assertIn('return "NO";', adapter)
        game = self.game.split("bool game::cancel_activity_or_ignore_query(", 1)[1]
        self.assertIn('.option( "NO", allow_key )', game)
        self.assertIn('if( action == "YES" )', game)
        # The Back action may not map to YES or MANAGER (both cancel work).
        safe = adapter.split("CANCEL_ACTIVITY_OR_IGNORE_QUERY", 1)[1].split("return \"NO\";", 1)[0]
        self.assertNotIn('return "YES";', safe)

    def test_one_refusal_suppresses_all_followup_distractions_in_same_activity(self):
        header = load(ROOT / "src/player_activity.h")
        impl = load(ROOT / "src/player_activity.cpp")
        interrupt = self.game.split("bool game::cancel_activity_or_ignore_query(", 1)[1]
        interrupt = interrupt.split("bool game::portal_storm_query(", 1)[0]
        self.assertIn("bool ignore_followup_distractions = false", header)
        self.assertIn("ignore_followup_distractions ||", impl)
        self.assertIn("ignore_followup_distractions = true;", impl)
        self.assertIn("ignore_followup_distractions = false;", impl)
        self.assertIn("other.ignore_followup_distractions", impl)
        self.assertIn('if( action == "NO" )', interrupt)
        self.assertIn("u.activity.ignore_followup_interruptions();", interrupt)
        self.assertIn("u.activity.ignore_distraction( type );", interrupt)

    def test_player_combat_speech_uses_real_hit_damage_and_suppresses_npc_chatter(self):
        messages = load(ROOT / "src/messages.cpp")
        header = load(ROOT / "src/messages.h")
        creature = load(ROOT / "src/creature.cpp")
        melee = load(ROOT / "src/melee.cpp")
        monster = load(ROOT / "src/monster.cpp")
        self.assertIn("class mute_combat_speech", header)
        self.assertIn("muted_combat_speech_depth > 0", messages)
        self.assertIn("player_messages.add_msg_string", messages)
        self.assertIn("source != nullptr && !is_avatar()", creature)
        self.assertIn("!is_avatar() && !t.is_avatar()", melee)
        self.assertIn("!target.is_avatar()", monster)
        self.assertIn("Only the redundant impact narration is muted", self.ranged)
        self.assertNotIn("Messages::mute_combat_speech original_impact_speech( is_avatar() )", self.ranged)
        self.assertIn("hit.second.second", self.ranged)
        self.assertIn("Your shot did not hit any creature.", self.ranged)
        self.assertIn("Your shot hit %s but dealt no damage.", self.ranged)
        self.assertIn("You grazed %1$s. %2$d damage. %3$d HP remaining.", self.ranged)
        self.assertIn("COMBAT_SHOT", self.ranged)
        self.assertIn("COMBAT_BALLISTIC", creature)
        self.assertNotIn("reducing health by %3$d percent", self.ranged)
        self.assertNotIn("lost %2$d percent health", melee)
        self.assertIn("original_melee_hit( is_avatar() && t.is_monster() )", melee)
        self.assertIn("unrelated_special_attack( !target->is_avatar() )",
                      load(ROOT / "src/mattack_actors.cpp"))
        self.assertIn("You lose %3$d HP.", monster)
        self.assertIn("%3$d HP remaining.", self.world)
        self.assertNotIn("enemy->hp_percentage()", self.world)
        self.assertIn('action == "A11Y_INTERNAL_FIRE_DIRECTION"', self.ranged)
        self.assertIn("nkiller->is_npc()", monster)
        self.assertIn("last_target.lock().get() == this", monster)
        self.assertIn("Your current target %s was killed.", monster)

    def test_deliberate_aimed_fire_is_available_beside_quick_fire(self):
        self.assertIn('intent.node == "target:aimed-shot"', self.ranged)
        self.assertIn('return "AIMED_SHOT";', self.ranged)
        self.assertIn('aimed.id = "target:aimed-shot";', self.ranged)
        self.assertIn('aimed.enabled = status == Status::Good && dst != src;', self.ranged)
        self.assertIn('aimed.label = _( "Aim carefully and fire" );', self.ranged)
        self.assertIn('action_aim_and_shoot( action )', self.ranged)
        self.assertIn('msgstr "Ngắm kỹ rồi bắn"', self.po)

    def test_ammunition_progress_is_from_engine_not_guessed(self):
        self.assertIn("Ammunition remaining: %d.", self.world)
        self.assertIn("held->ammo_remaining()", self.world)
        self.assertIn("Loaded %1$d %2$s into %3$s. %4$d rounds in gun.", self.activity)
        self.assertNotIn('add_msg( m_info, _( "Reload progress:', self.activity)
        self.assertIn("gun->ammo_remaining()", self.activity)
        self.assertIn("Automatic reload complete. %d rounds loaded.", self.game)
        self.assertIn("Automatic reload stopped. %d rounds loaded;", self.game)

    def test_vietnamese_combat_labels_have_translations(self):
        for msg in (
                "Cannot fire: %1$s. %2$d rounds loaded. Reload or change weapons.",
                "The weapon cannot fire right now. Check ammunition and firing mode.",
                "Loaded %1$d %2$s into %3$s. %4$d rounds in gun.",
                "Fighting unarmed.",
                "Wielding %s.",
                "%1$s, %2$s, %3$d HP remaining.",
                "%1$s hits your %2$s. You lose %3$d HP.",
                "Critical hit on %1$s. %2$d damage. %3$d HP remaining.",
                "Hit %1$s. %2$d damage. %3$d HP remaining.",
                "You grazed %1$s. %2$d damage. %3$d HP remaining.",
                "You shot %1$s. %2$d damage. %3$d HP remaining.",
        ):
            self.assertIn('msgid "' + msg + '"', self.po)

    def test_four_finger_counts_are_reserved_for_map_not_attacks(self):
        controller = self.controller.split("public boolean handleCustomGesture(", 1)[1]
        combat = controller.split("if (combatMode && directWorldGesturesEnabled())", 1)[1]
        for gesture, vector in (
            ("swipe_3_up", "0, -1"),
            ("swipe_3_right", "1, 0"),
            ("swipe_3_down", "0, 1"),
            ("swipe_3_left", "-1, 0"),
        ):
            self.assertIn(f'case "{gesture}": moveInCombat({vector}); return true;', combat)
        self.assertIn("dispatchWorldMovement(dx, dy);", self.controller)
        self.assertIn("!combatMode && worldStepAllowed()", self.controller)
        tests = load(INSTRUMENTATION / "GameMenuGestureDispatchTest.java")
        self.assertIn("allFourThreeFingerCombatDirectionsAreDetected", tests)
        self.assertIn("1, actions.savedThreeFingerSwipes", tests)


if __name__ == "__main__":
    unittest.main()
