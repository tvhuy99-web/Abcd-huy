"""Regression checks for accessible movement/posture hub group four."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class HubMovementTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hub = (JAVA / "GameActionHub.java").read_text("utf-8")
        cls.ctrl = (JAVA / "GameReaderController.java").read_text("utf-8")
        cls.world = (ROOT / "src/a11y_world.cpp").read_text("utf-8")
        cls.handle = (ROOT / "src/handle_action.cpp").read_text("utf-8")
        cls.avatar = (ROOT / "src/avatar.cpp").read_text("utf-8")
        cls.game = (ROOT / "src/game.cpp").read_text("utf-8")
        cls.outcome = (ROOT / "src/a11y_outcome.cpp").read_text("utf-8")

    def test_all_eight_registered_game_directions(self):
        match = re.search(r'DIRECTION_ACTIONS = \{(.*?)\};', self.hub, re.S)
        self.assertIsNotNone(match)
        ids = re.findall(r'"([A-Z]+)"', match.group(1))
        self.assertEqual(ids, [
            "UP", "RIGHTUP", "RIGHT", "RIGHTDOWN", "DOWN",
            "LEFTDOWN", "LEFT", "LEFTUP"])
        self.assertIn("snapshot.find(DIRECTION_ACTIONS[i])", self.hub)
        self.assertIn('node.actions.contains("activate")', self.hub)
        self.assertIn("case GameActionHub.OPEN_DIRECTIONS:", self.ctrl)
        self.assertIn("hubSection = GameActionHub.DIRECTIONS;", self.ctrl)
        self.assertIn("hubSection == GameActionHub.DIRECTIONS", self.ctrl)
        self.assertIn("hubSection = 3;", self.ctrl)
        self.assertIn('target.startsWith("action:")', self.ctrl)

    def test_native_mode_menu_reused(self):
        self.assertIn('"open_movement"', self.hub)
        self.assertIn("case ACTION_OPEN_MOVEMENT:", self.handle)
        self.assertIn("open_movement_mode_menu();", self.handle)

    def test_current_and_selected_stances_are_distinct(self):
        self.assertIn("viewer.current_movement_mode()->name()", self.world)
        self.assertIn("viewer.get_desired_move_mode()->name()", self.world)
        self.assertIn("Now: %1$s. Selected: %2$s.", self.world)
        self.assertIn(
            "Selected movement: %s. Takes effect on your next action.",
            self.avatar)
        self.assertIn("action_outcome_kind::stance_changed", self.outcome)

    def test_auto_route_and_wait_feedback(self):
        self.assertIn("Auto travel: on", self.world)
        self.assertIn("Auto travel: off", self.world)
        self.assertIn("No route in that direction.", self.handle)
        self.assertIn("Choose a direction to follow a path.", self.handle)
        self.assertIn("Waiting: %s", self.handle)
        self.assertIn("Stood still for one turn.", self.handle)
        self.assertIn("vertical_unchanged", self.game)
        self.assertIn("Did not change level.", self.outcome)

    def test_direction_strings_localized(self):
        for filename in ("values", "values-vi"):
            xml = (ROOT / "android/app/src/main/res" /
                   filename / "strings.xml").read_text("utf-8")
            for key in ("a11yHubChooseDirection", "a11yHubMoveNorth",
                        "a11yHubMoveNortheast", "a11yHubMoveEast",
                        "a11yHubMoveSoutheast", "a11yHubMoveSouth",
                        "a11yHubMoveSouthwest", "a11yHubMoveWest",
                        "a11yHubMoveNorthwest"):
                self.assertIn(f'name="{key}"', xml)


if __name__ == "__main__":
    unittest.main()
