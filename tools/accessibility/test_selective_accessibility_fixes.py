"""Keep selected accessibility fixes safe without adopting lossy speech policies."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
SRC = ROOT / "src"


class SelectiveAccessibilityFixes(unittest.TestCase):
    def test_autodrive_back_cancels_without_using_a_row_index(self):
        reader = (JAVA / "GameReaderController.java").read_text("utf-8")
        self.assertIn("requestTravelControl(3, nativeState);", reader)
        self.assertIn("private void requestTravelControl(int command, int state)", reader)

    def test_native_editor_keeps_touch_sequence_with_child(self):
        surface = (JAVA / "ReaderGestureSurface.java").read_text("utf-8")
        reader = (JAVA / "GameReaderController.java").read_text("utf-8")
        self.assertIn("nativeChildTouch = listener.shouldPassTouchToChild(event);", surface)
        self.assertIn("|| nativeChildTouch", surface)
        self.assertIn("editor.focusNativeInput();", reader)

    def test_deferred_restore_is_explicit_not_a_live_swap(self):
        backup = (JAVA / "BackupRestoreActivity.java").read_text("utf-8")
        self.assertIn("!backupBusy && stagedBackupReady && !deferredRestorePending", backup)
        self.assertIn("if (!ReaderGameSession.canRestore())", backup)
        self.assertIn("confirmDeferredRestore();", backup)

    def test_snapshot_caches_screen_content_but_keeps_live_messages(self):
        runtime = (SRC / "a11y_runtime.cpp").read_text("utf-8")
        semantic = (SRC / "a11y_semantics.cpp").read_text("utf-8")
        self.assertIn("snapshot_json_with_content( screen.snapshot, screen.content )", runtime)
        self.assertIn("Messages::recent_messages( 12 )", semantic)

    def test_speech_and_sonar_contracts_are_preserved(self):
        queue = (JAVA / "ReaderSpeechQueue.java").read_text("utf-8")
        sonar = (JAVA / "AlignmentSonarPlayer.java").read_text("utf-8")
        self.assertNotIn("urgentGameEvents.size() >= 12", queue)
        self.assertNotIn("orderedGameEvents.size() >= 12", queue)
        self.assertIn("setMaxStreams(DIRECTIONS * 2)", sonar)


if __name__ == "__main__":
    unittest.main()
