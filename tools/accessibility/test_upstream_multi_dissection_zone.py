"""Guard full upstream DISSECT-zone menu wiring while preserving a11y actions."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]


class UpstreamMultiDissectionIntegration(unittest.TestCase):
    def test_dissection_zone_menu_and_actor(self):
        source = (ROOT / "src" / "handle_action.cpp").read_text(encoding="utf-8")
        actor = (ROOT / "src" / "activity_actor_definitions.h").read_text(
            encoding="utf-8"
        )
        zones = (ROOT / "data" / "json" / "zones.json").read_text(
            encoding="utf-8"
        )
        self.assertIn('zone_type_DISSECT( "DISSECT" )', source)
        self.assertIn("MultiDissection = 262144", source)
        self.assertIn("player_character.pos_bub() ) ? MultiDissection : 0", source)
        self.assertIn("menu.addentry_desc( MultiDissection", source)
        self.assertIn("case MultiDissection:", source)
        self.assertIn("assign_activity( multi_dissection_activity_actor() )", source)
        self.assertIn("class multi_dissection_activity_actor", actor)
        self.assertIn('"id": "DISSECT"', zones)

    def test_accessible_driving_actions_remain(self):
        source = (ROOT / "src" / "handle_action.cpp").read_text(encoding="utf-8")
        self.assertIn("A11Y_INTERNAL_VEHICLE_BEGIN", source)
        self.assertIn("a11y_continuous_vehicle_ready", source)


if __name__ == "__main__":
    unittest.main()
