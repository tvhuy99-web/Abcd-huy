"""Source contracts for the direct four-way Android reader gesture design."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
GESTURES = JAVA / "ReaderGestureSurface.java"
CONTROLLER = JAVA / "GameReaderController.java"
WORLD = ROOT / "src/a11y_world.cpp"
HANDLER = ROOT / "src/handle_action.cpp"


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

    def test_reader_hub_is_on_demand_without_the_old_dashboard(self):
        controller = CONTROLLER.read_text(encoding="utf-8")
        self.assertTrue((JAVA / "GameActionHub.java").exists())
        self.assertIn("android.graphics.Color.TRANSPARENT", controller)
        self.assertIn("public void showComprehensiveMenu()", controller)
        self.assertIn("private void renderHub()", controller)
        self.assertIn("else if (comprehensiveMenu)", controller)
        self.assertIn("root.setVisibility(View.GONE);", controller)
        self.assertNotIn("button(pages,", controller)
        self.assertNotIn("button(tools,", controller)

    def test_direct_shortcuts_include_the_restored_hub(self):
        controller = CONTROLLER.read_text(encoding="utf-8")
        self.assertIn("if (target == null) { return false; }", controller)
        self.assertIn("executeShortcutTarget(target);", controller)
        self.assertIn(
            'String id = target.substring("action:".length());',
            controller,
        )
        self.assertIn('target.startsWith("hub:group:")', controller)
        self.assertIn('send("activate", "");', controller)


if __name__ == "__main__":
    unittest.main()
