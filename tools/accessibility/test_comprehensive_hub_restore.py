"""Guard the approved 16-group menu and 33 latest-wins shortcuts."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class ComprehensiveHubRestore(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hub = (JAVA / "GameActionHub.java").read_text(encoding="utf-8")
        cls.ctrl = (JAVA / "GameReaderController.java").read_text("utf-8")
        cls.surface = (JAVA / "ReaderGestureSurface.java").read_text("utf-8")
        cls.bindings = (JAVA / "ReaderGestureBindings.java").read_text(
            encoding="utf-8"
        )

    def test_sixteen_groups_in_order(self):
        section = self.hub.split("static final int[] TITLES = {", 1)[1]
        section = section.split("};", 1)[0]
        titles = re.findall(r"R\.string\.(a11yHub\w+)", section)
        expected = [
            "a11yHubSearch", "a11yHubNearby", "a11yHubExplore",
            "a11yHubMovement", "a11yHubInteraction", "a11yHubInventory",
            "a11yHubHealth", "a11yHubEquipment", "a11yHubCombat",
            "a11yHubCrafting", "a11yHubCharacter", "a11yHubMissions",
            "a11yHubVehicles", "a11yHubAutomation", "a11yHubReader",
            "a11yHubSystem",
        ]
        self.assertEqual(titles, expected)
        self.assertIn("hubSpoken(hubEntries(), 0)", self.ctrl)

    def test_triple_two_finger_tap_opens_hub(self):
        self.assertIn("dispatchMultiTap(fingers, 3)", self.surface)
        self.assertIn("listener.twoFingerTap(count)", self.surface)
        self.assertIn(
            'if (count == 3) { openActionMenu(); return; }', self.ctrl
        )
        self.assertIn("public void showComprehensiveMenu()", self.ctrl)
        self.assertNotIn('requestLiveWorldCommand("action_menu");', self.ctrl)

    def test_hub_swipes_cannot_move_player(self):
        self.assertIn("!comprehensiveMenu && !closed", self.ctrl)
        self.assertIn("hubIndex + delta", self.ctrl)
        self.assertIn("else if (comprehensiveMenu)", self.ctrl)
        self.assertIn("root.setVisibility(View.GONE);", self.ctrl)
        self.assertIn("android.graphics.Color.TRANSPARENT", self.ctrl)

    def test_33_shortcuts_and_long_press_are_restored(self):
        ids = self.bindings.split("static final String[] IDS = {", 1)[1]
        ids = ids.split("};", 1)[0]
        gestures = re.findall(r'"([^"]+)"', ids)
        self.assertEqual(len(gestures), 33)
        self.assertEqual(len(set(gestures)), 33)
        self.assertIn("option.setOnLongClickListener(", self.ctrl)
        self.assertIn("showGestureChoices();", self.ctrl)
        self.assertIn("gestureBindings.assign(", self.ctrl)
        self.assertIn("gestureBindings.restoreDefault(target)", self.ctrl)
        self.assertIn("result.displacedTarget", self.ctrl)
        self.assertIn('target.startsWith("hub:group:")', self.ctrl)

    def test_search_and_gameplay_are_safely_separated(self):
        self.assertIn("Normalizer.normalize(source", self.hub)
        self.assertIn("addCommandMatches(result, snapshot, query)", self.hub)
        self.assertIn("R.string.a11yHubAllCommands", self.hub)
        self.assertIn("private void searchHub()", self.ctrl)
        self.assertIn("snapshot.find(item.node)", self.ctrl)
        self.assertIn("hubIndex = hubRootIndex", self.ctrl)
        self.assertIn("ReaderGestureBindings.IDS.length + 2", self.ctrl)


if __name__ == "__main__":
    unittest.main()
