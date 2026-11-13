"""Accessibility regression tests for vehicle and journey actions."""

import re
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "src"


class VehicleJourneyHubTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.game = (SRC / "game.cpp").read_text("utf-8")
        cls.action = (SRC / "handle_action.cpp").read_text("utf-8")
        cls.world = (SRC / "a11y_world.cpp").read_text("utf-8")
        cls.vehicle_interact = (SRC / "veh_interact.cpp").read_text("utf-8")

    def test_vehicle_actions_capture_node_id_before_moving_the_node(self):
        bindings = (
            "part_bindings.push_back( { node.id, part_index, row } );",
            "install_bindings.push_back( { node.id, vp } );",
            "part_bindings.push_back( { node.id, part_index, static_cast<int>( j ) } );",
            "part_bindings.push_back( { node.id, part_index, static_cast<int>( i ) } );",
        )
        for binding in bindings:
            with self.subTest(binding=binding):
                self.assertRegex(
                    self.vehicle_interact,
                    re.escape(binding) + r"\s+snapshot\.nodes\.push_back\( std::move\( node \) \);",
                )

    def test_vehicle_selection_names_visible_controls(self):
        self.assertIn("Choose vehicle controls", self.game)
        self.assertIn("candidate_positions.size() == 1", self.game)
        self.assertIn("No visible vehicle controls nearby", self.game)
        self.assertIn('part_with_feature( "CONTROLS", true )', self.game)
        self.assertIn("direction_name( direction_from(", self.game)
        self.assertIn("veh->handle_potential_theft( u )", self.game)

    def test_grab_choices_are_visible_and_filtered(self):
        start = self.action.index("static void grab(")
        end = self.action.index("static void haul()", start)
        body = self.action[start:end]
        self.assertIn("Choose an object to grab", body)
        self.assertIn("No visible movable furniture or vehicle", body)
        self.assertIn("candidates.size() == 1", body)
        self.assertIn("is_movable()", body)
        self.assertIn("a11y::observation_source::visible", body)
        self.assertIn("handle_potential_theft( you )", body)
        self.assertIn("query_yn(", body)
        self.assertIn("you.grab( object_type::NONE );", body)

    def test_release_label_and_directional_swipe_guard(self):
        self.assertIn('node.id == "grab"', self.world)
        self.assertIn('_( "Release %s" )', self.world)
        self.assertIn("swipe_movement_allowed =", self.world)
        self.assertIn("&& !viewer.controlling_vehicle", self.world)

    def test_auto_travel_toggled_with_one_reader_message(self):
        action = self.action.split(
            "case ACTION_TOGGLE_AUTO_TRAVEL_MODE:", 1
        )[1].split("case ACTION_TOGGLE_SAFEMODE:", 1)[0]
        self.assertIn("Auto travel on. Choose a direction", action)
        self.assertIn("Auto travel off.", action)
        self.assertEqual(action.count("add_msg( m_info"), 2)
        self.assertIn("if( a11y::enabled() )", action)

    def test_climb_reports_only_real_z_change(self):
        self.assertIn('_( "Climb up %s" )', self.game)
        self.assertIn('_( "Climb down %s" )', self.game)
        self.assertIn("const int initial_level =", self.action)
        self.assertIn("player_character.posz() != initial_level",
                      self.action)
        self.assertIn('_( "Now on level %d." )', self.action)
        self.assertIn("vertical_move( -1,", self.action)
        self.assertIn("vertical_move( 1,", self.action)


if __name__ == "__main__":
    unittest.main()
