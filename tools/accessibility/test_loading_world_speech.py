"""Speech must cover Quick Play loading before world semantic snapshots exist."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class LoadingWorldSpeech(unittest.TestCase):
    def test_loading_imgui_screen_publishes_throttled_progress(self):
        code = (ROOT / "src/loading_ui.cpp").read_text(encoding="utf-8")
        self.assertIn('a11y::android::announce( "A11Y_LOADING:" + message, 1 );',
                      code)
        self.assertIn("std::chrono::seconds( 8 )", code)
        self.assertIn("std::chrono::seconds( 30 )", code)
        self.assertIn("last_spoken_loading_status", code)
        self.assertIn("if( first && !gLUI->tip.empty() )", code)
        self.assertIn("a11y::android::suspended()", code)

    def test_quick_play_world_creation_has_early_confirmation(self):
        menu = (ROOT / "src/main_menu.cpp").read_text(encoding="utf-8")
        world = (ROOT / "src/game.cpp").read_text(encoding="utf-8")
        self.assertIn("const bool is_play_now = sel2 == 3 || sel2 == 4;", menu)
        self.assertIn("Preparing a new world. Please wait.", menu)
        self.assertIn("Loading world data. Please wait.", world)
        self.assertIn("Creating your character and starting the game.", world)

    def test_loading_is_spoken_without_snapshot_and_world_entry_is_explicit(self):
        reader = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        self.assertIn('message.startsWith("A11Y_LOADING:")', reader)
        self.assertIn("(snapshot == null && !loading)", reader)
        self.assertIn("lastLoadingStatus = spokenMessage;", reader)
        self.assertIn("handler.postDelayed(loadingSilenceWarning, 45000);", reader)
        self.assertIn("lastLoadingStatus = \"\";", reader)
        self.assertIn('ReaderSnapshot.Node nextTile = next.find("world:here")', reader)
        self.assertNotIn("R.string.a11yEnteredWorld", reader)
        self.assertIn("loadingSilenceWarning", reader)

    def test_read_again_speaks_last_stage_while_waiting(self):
        reader = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        self.assertIn('if (snapshot == null && !lastLoadingStatus.isEmpty()) {',
                      reader)
        self.assertIn("R.string.a11yLoadingStatus, lastLoadingStatus", reader)


if __name__ == "__main__":
    unittest.main()
