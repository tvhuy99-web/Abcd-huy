"""Source contracts for the direct four-way Android reader gesture design."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
GESTURES = JAVA / "ReaderGestureSurface.java"
CONTROLLER = JAVA / "GameReaderController.java"
HUB = JAVA / "GameActionHub.java"
WORLD = ROOT / "src/a11y_world.cpp"
HANDLER = ROOT / "src/handle_action.cpp"
ACTIONS = ROOT / "src/action.cpp"


class WorldGestureContracts(unittest.TestCase):
    def test_cardinal_swipes_need_no_fast_mode_opt_in(self):
        gestures = GESTURES.read_text(encoding="utf-8")
        controller = CONTROLLER.read_text(encoding="utf-8")
        self.assertIn("listener.moveDirection(dx > 0 ? 1 : -1, 0)", gestures)
        self.assertIn("listener.moveDirection(0, dy > 0 ? 1 : -1)", gestures)
        guard = controller.split("public boolean worldMovementEnabled()", 1)[1]
        guard = guard.split(
            "private static boolean hasCardinalDirections", 1
        )[0]
        self.assertIn("snapshot.swipeMovementAllowed", guard)
        self.assertNotIn("preferences.isFastMovementEnabled()", guard)
        self.assertNotIn("hasCompletedMovementPractice()", guard)

    def test_multitap_sequences_are_mutually_exclusive(self):
        gestures = GESTURES.read_text(encoding="utf-8")
        self.assertIn("queueMultiTap(owner, maxPointers)", gestures)
        self.assertIn("dispatchMultiTap(fingers, 3)", gestures)
        self.assertIn("listener.twoFingerTap(count)", gestures)
        self.assertIn("listener.threeFingerTap()", gestures)
        self.assertIn("ViewConfiguration.getDoubleTapTimeout()", gestures)
        self.assertIn('if (maxPointers == 2)', gestures)
        self.assertIn('listener.scanDirection(', gestures)
        self.assertIn('maxPointers, direction', gestures)
        self.assertIn("maxPointers <= 4", gestures)
        self.assertIn("listener.handleCustomGesture(gesture)", gestures)
        self.assertNotIn("listener.stopSpeech();\\n", gestures)

    def test_unrestricted_scan_is_ordered_and_read_only(self):
        world = WORLD.read_text(encoding="utf-8")
        scan = world.split("void announce_linear_scan(", 1)[1]
        scan = scan.split("void announce_player_coordinates()", 1)[0]
        self.assertIn("distance = 1; distance <= 16; ++distance", scan)
        self.assertIn("here.tername( pos )", scan)
        self.assertIn("get_creature_tracker().creature_at( pos, true )", scan)
        self.assertIn("here.i_at( pos )", scan)
        self.assertNotIn("observe_tile(", scan)
        self.assertIn("A11Y_SCAN:", scan)
        handler = HANDLER.read_text(encoding="utf-8")
        self.assertIn("intent.node == \"reader:scan:north\"", handler)
        self.assertIn("a11y::announce_player_coordinates()", handler)

    def test_sixteen_spoken_root_items(self):
        src = HUB.read_text(encoding="utf-8")
        group_titles = src.split(
            "static final int[] TITLES = {", 1
        )[1].split("};", 1)[0]
        self.assertEqual(group_titles.count("R.string.a11yHub"), 16)
        self.assertIn(
            "GameActionHub.OPEN_ALL",
            CONTROLLER.read_text(encoding="utf-8"),
        )
        self.assertIn("addCommandMatches(result, snapshot, query)", src)

    def test_curated_actions_exist_in_the_game(self):
        src = HUB.read_text(encoding="utf-8")
        ids = src.split("private static final String[][] ACTIONS = {", 1)[1]
        ids = ids.split("private static String normalized", 1)[0]
        named = set(re.findall(r'"([^"]+)"', ids))
        action_source = ACTIONS.read_text(encoding="utf-8")
        registered = set(re.findall(r'return "([^"]+)";', action_source))
        self.assertFalse(named - registered, named - registered)


if __name__ == "__main__":
    unittest.main()
