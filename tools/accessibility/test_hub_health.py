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
        self.assertIn("Before sleep. Sleepiness:", self.handle)
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
