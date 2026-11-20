"""Group-seven accessibility regression checks."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2] / "src"


class HealthHubTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.body = (ROOT / "bodygraph.cpp").read_text("utf-8")
        cls.med = (ROOT / "medical_ui.cpp").read_text("utf-8")
        cls.player = (ROOT / "player_display.cpp").read_text("utf-8")
        cls.food = (ROOT / "game_inventory.cpp").read_text("utf-8")
        cls.inv = (ROOT / "inventory_ui.cpp").read_text("utf-8")
        cls.handle = (ROOT / "handle_action.cpp").read_text("utf-8")
        cls.actor = (ROOT / "activity_actor.cpp").read_text("utf-8")
        cls.actor_h = (ROOT / "activity_actor_definitions.h").read_text("utf-8")
        cls.avatar = (ROOT / "avatar.cpp").read_text("utf-8")

    def test_bodygraph_is_a_real_spoken_list(self):
        self.assertIn('a11y::screen_scope reader( &ctxt, "body-status"', self.body)
        self.assertIn('node.id = "body:part:"', self.body)
        self.assertIn('node.role = "listitem"', self.body)
        self.assertIn("a11y::limb_health_description( *u, parent )", self.body)
        self.assertIn('intent.action != "focus"', self.body)
        self.assertIn("prepare_infolist();", self.body)
        self.assertIn('return "CONFIRM";', self.body)
        self.assertIn('return "TIMEOUT";', self.body)
        self.assertIn("reader.update( std::move( snapshot ) );", self.body)

    def test_medical_triage_and_native_treatment_checks(self):
        self.assertIn("std::stable_sort( urgent_first.begin()", self.med)
        self.assertIn("you->has_effect( effect_infected, part )", self.med)
        self.assertIn("you->has_effect( effect_bleed, part )", self.med)
        self.assertIn('_( "Priority injury: %s" )', self.med)
        self.assertIn('a11y::limb_health_description( *you, part )', self.med)
        self.assertIn("you->pick_wound_fix( bp )", self.med)
        self.assertIn("opt.doable", self.med)
        self.assertIn("a11y::append_requirements(", self.med)

    def test_medical_wounds_sorted_by_severity(self):
        # Port upstream's severe-first display without replacing the
        # accessibility-aware medical screen or mutating the wound list.
        body = self.med.split(
            "std::string medical_ui::get_limb_wounds", 1)[1].split(
                "std::string medical_ui::get_limb_scores_and_modifiers", 1)[0]
        self.assertIn("std::vector<wound> wds = bp->get_wounds();", body)
        self.assertIn("std::stable_sort( wds.begin(), wds.end()", body)
        self.assertIn("a.type->severity > b.type->severity", body)
        self.assertIn("wound_severity::CRITICAL", body)
        self.assertIn("wound_severity::MAJOR", body)
        self.assertEqual(
            body.count("colorize( wd.type->get_name(), severity_color )"), 2)
        # Existing medical treatment remains delegated to native checks.
        self.assertIn("you->pick_wound_fix( bp )", self.med)
        self.assertIn('a11y::limb_health_description( *you, part )', self.med)

    def test_medical_debug_keeps_upstream_wound_diagnostics(self):
        # Normal reader health summaries must not disclose exact HP.
        # Technical wound details are restricted to debug mode.
        body = self.med.split(
            "std::string medical_ui::get_limb_wounds", 1)[1].split(
                "std::string medical_ui::get_limb_scores_and_modifiers", 1)[0]
        self.assertIn('#include "enum_conversions.h"', self.med)
        self.assertIn('if( !debug_mode ) {', body)
        self.assertIn('healing speed multiplier: %.3fx', body)
        self.assertIn('wd.get_wound_healing_modifier( you )', body)
        self.assertIn('wd.get_pain_raw()', body)
        self.assertIn('wd.get_hp_damage()', body)
        self.assertIn('hp damage: %d', body)
        # Access the effective remaining damage through the public method;
        # wound::hp_damage is private and cannot be used from this UI.
        self.assertNotIn('wd.hp_damage', body)
        self.assertIn('wd.type->get_modify_values()', body)
        self.assertIn('io::enum_to_string( mv.type )', body)
        self.assertLess(body.index('if( !debug_mode ) {'),
                        body.index('healing speed multiplier: %.3fx'))
        self.assertIn('a11y::limb_health_description( *you, part )', self.med)

    def test_quick_health_without_raw_hitpoints(self):
        self.assertIn('"pd:quick-health"', self.player)
        self.assertIn('_( "Quick health summary" )', self.player)
        self.assertIn("display::hunger_text_color( you )", self.player)
        self.assertIn("display::sleepiness_text_color( you )", self.player)
        self.assertIn("you.has_effect( effect_bleed, part )", self.player)
        quick = self.player.split(
            "// Readable health overview before navigating", 1)[1]
        quick = quick.split("const auto stat_text", 1)[0]
        self.assertNotIn("get_part_hp_cur", quick)

    def test_consumable_information_before_commit(self):
        self.assertIn("if( using_consume_menu && a11y::enabled() )", self.food)
        self.assertIn("inv_s.item_entry_count()", self.food)
        self.assertIn('owner.title == _( "Consume item" )', self.inv)
        self.assertIn("owner.u.can_eat( *location )", self.inv)
        self.assertIn("location->rotten()", self.inv)
        self.assertIn("location->is_going_bad()", self.inv)
        self.assertIn("compute_effective_nutrients( *location )", self.inv)

    def test_sleep_and_wait_feedback_only_on_real_actions(self):
        self.assertIn("Before sleep.  Sleepiness:", self.handle)
        self.assertIn("here.tername( p )", self.handle)
        self.assertIn("const bool was_asleep = has_effect( effect_sleep )",
                      self.avatar)
        self.assertIn("if( was_asleep && a11y::enabled() )", self.avatar)
        self.assertIn("to_string_time_of_day( calendar::turn )", self.avatar)
        self.assertIn("Waiting interrupted.", self.actor)
        self.assertIn("calendar::once_every( 5_minutes )", self.actor)
        self.assertIn("void canceled( player_activity &act, Character &who )",
                      self.actor_h)
        self.assertIn("who.add_msg_if_player( _( \"You finish waiting.\" ) )",
                      self.actor)

    def test_workout_stays_native_and_cancellable(self):
        self.assertIn('workout_query.addentry_desc( 1,', self.actor)
        self.assertIn('workout_query.addentry_desc( 4,', self.actor)
        self.assertIn("if( length > 0 )", self.actor)
        self.assertIn("if( who.is_avatar() && a11y::enabled() )", self.actor)
        self.assertIn("Workout started: intensity", self.actor)
        self.assertIn("act.set_to_null();", self.actor)


if __name__ == "__main__":
    unittest.main()
