"""Regression tests for accessible character abilities and health."""

import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "src"


class CharacterHubTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mutation = (SRC / "mutation_ui.cpp").read_text("utf-8")
        cls.bionics = (SRC / "bionics_ui.cpp").read_text("utf-8")
        cls.activity = (SRC / "activity_actor.cpp").read_text("utf-8")
        cls.player = (SRC / "player_display.cpp").read_text("utf-8")
        cls.medical = (SRC / "medical_ui.cpp").read_text("utf-8")
        cls.action = (SRC / "handle_action.cpp").read_text("utf-8")
        cls.zones = (SRC / "zone_manager_ui.cpp").read_text("utf-8")

    def test_mutations_consistently_check_stamina(self):
        self.assertEqual(self.mutation.count(
            "!mut_data.stamina || get_stamina() >= mut_data.cost"), 2)
        # both activation paths now report the specific shortage
        # through activation_shortage(); the generic message survives
        # only as its fallback, so it appears once, not twice.
        self.assertEqual(self.mutation.count(
            "activation_shortage( mut_data )"), 2)
        self.assertEqual(self.mutation.count(
            "Not enough resources to activate %s."), 1)
        self.assertEqual(self.mutation.count(
            "Cannot activate %s. Focus it to hear the description."), 2)

    def test_bionics_speak_setting_changes(self):
        self.assertIn("Fuel saving for %1$s:", self.bionics)
        self.assertIn("Automatic shutdown for %1$s:", self.bionics)
        self.assertIn("Show %1$s on screen:", self.bionics)
        self.assertIn("tmp->toggle_safe_fuel_mod();", self.bionics)

    def test_workout_repeat_skips_duration_input(self):
        sec = self.activity.split(
            "void workout_activity_actor::start(", 1
        )[1].split("void workout_activity_actor::do_turn(", 1)[0]
        self.assertIn("Repeat last workout:", sec)
        self.assertIn("repeat_previous ? reader_last_duration", sec)
        self.assertIn("if( !repeat_previous )", sec)
        self.assertIn("reader_last_intensity = intensity_modifier;", sec)
        self.assertIn("if( !who.is_avatar() || !a11y::enabled() )", sec)

    def test_health_alerts_and_treatment_feedback(self):
        self.assertIn("std::string quick;", self.player)
        self.assertIn('quick += "\\n" + needs;', self.player)
        self.assertIn("urgent body-part warnings", self.player)
        self.assertIn("report_treatment_issue(", self.medical)
        self.assertIn("add_msg_if_player( m_info", self.medical)
        self.assertIn(
            "assign_activity( fix_wound_activity_actor(", self.medical)

    def test_previous_compilation_faults_fixed(self):
        self.assertIn("here.tername( p )", self.action)
        self.assertNotIn("here.name( p )", self.action)
        self.assertIn("time_duration::from_moves(", self.action)
        self.assertNotIn("moves_to_string( sp.casting_time", self.action)
        self.assertIn('#include "messages.h"', self.zones)


if __name__ == "__main__":
    unittest.main()
