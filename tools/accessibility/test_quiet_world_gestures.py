"""World movement is quiet, sound-backed, and has a native triple-tap menu."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class QuietWorld(unittest.TestCase):
    def test_only_noteworthy_native_terrain_is_spoken(self):
        world = (ROOT / "src/a11y_world.cpp").read_text(encoding="utf-8")
        ctrl = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        self.assertIn('here.role = noteworthy_here ? "alert" : "document";',
                      world)
        self.assertIn("here.value = current_tile.terrain;", world)
        self.assertIn('boolean newlyNoteworthy = worldView', ctrl)
        self.assertIn('if (newlyNoteworthy) { feedback(nextTile.value); }', ctrl)
        self.assertNotIn("R.string.a11yEnteredWorld", ctrl)
        self.assertIn("String previous = focus;", ctrl)

    def test_triple_tap_opens_native_action_list(self):
        surface = (JAVA / "ReaderGestureSurface.java").read_text(
            encoding="utf-8")
        ctrl = (JAVA / "GameReaderController.java").read_text(
            encoding="utf-8")
        self.assertIn("queueWorldTap(event, owner)", surface)
        self.assertIn("listener.openActionMenu();", surface)
        self.assertIn("listener.activate();", surface)
        self.assertIn('requestLiveWorldCommand("action_menu");', ctrl)
        self.assertNotIn("showComprehensiveMenu(", ctrl)

    def test_temporary_footstep_is_removed(self):
        engine = (ROOT / "src/handle_action.cpp").read_text(
            encoding="utf-8")
        cue = (JAVA / "ReaderCuePlayer.java").read_text(encoding="utf-8")
        pref = (JAVA / "ReaderPreferences.java").read_text(encoding="utf-8")
        self.assertNotIn('a11y::android::cue( "footstep" );', engine)
        self.assertNotIn("R.raw.a11y_cue_step", cue)
        self.assertIn('values.getBoolean("sound_cues", false)', pref)


if __name__ == "__main__":
    unittest.main()
