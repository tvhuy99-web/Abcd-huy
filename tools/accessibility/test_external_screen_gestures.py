"""Static contracts for gesture navigation on Android screens outside SDL."""
import unittest
from pathlib import Path

JAVA = Path(__file__).resolve().parents[2] / (
    "android/app/src/main/java/com/cleverraven/cataclysmdda"
)


class ExternalScreenGestures(unittest.TestCase):
    def test_two_finger_down_goes_back_without_one_finger_fallthrough(self):
        detector = (JAVA / "TwoFingerBackGesture.java").read_text(encoding="utf-8")
        surface = (JAVA / "BootstrapSelfVoiceSurface.java").read_text(encoding="utf-8")
        # The retired ReaderSetupActivity redirects to AccessibilitySettingsActivity.
        # The two new screens use the shared, tested voice surface.
        accessibility = (JAVA / "AccessibilitySettingsActivity.java").read_text(encoding="utf-8")
        backup = (JAVA / "BackupRestoreActivity.java").read_text(encoding="utf-8")
        self.assertIn("dy1 >= minimumDistance && dy2 >= minimumDistance", detector)
        self.assertIn("Math.abs(dx1) * 1.35f", detector)
        self.assertIn("ACTION_POINTER_UP", detector)
        self.assertIn("TwoFingerBackGesture.matches(", surface)
        self.assertIn("ignoreUntilPointersReleased", surface)
        self.assertIn("ACTION_POINTER_UP", surface)
        self.assertIn("listener.goBack();", surface)
        for code in (accessibility, backup):
            self.assertIn("BootstrapSelfVoiceSurface", code)
            self.assertIn("surface.setContent(controls);", code)
            self.assertIn("@Override public void goBack() { finish(); }", code)

    def test_vertical_swipe_changes_focused_slider_and_reads_value(self):
        surface = (JAVA / "BootstrapSelfVoiceSurface.java").read_text(encoding="utf-8")
        accessibility = (JAVA / "AccessibilitySettingsActivity.java").read_text(encoding="utf-8")
        tts = (JAVA / "TtsSettingsActivity.java").read_text(encoding="utf-8")
        self.assertIn("adjustFocusedSetting(dy < 0 ? 1 : -1)", surface)
        self.assertIn("bar.setProgress(", surface)
        self.assertIn("speakFocused();", surface)
        self.assertIn("distance.setOnSeekBarChangeListener(", accessibility)
        self.assertIn("duration.setOnSeekBarChangeListener(", accessibility)
        self.assertIn("bar.setContentDescription(description)", accessibility)
        for name in ("rate", "pitch", "volume"):
            self.assertIn(name + ".setContentDescription(", tts)

    def test_navigation_respects_talkback_and_discards_draft_on_back(self):
        surface = (JAVA / "BootstrapSelfVoiceSurface.java").read_text(encoding="utf-8")
        splash = (JAVA / "SplashScreen.java").read_text(encoding="utf-8")
        tts = (JAVA / "TtsSettingsActivity.java").read_text(encoding="utf-8")
        self.assertIn("if (!listener.isActive())", surface)
        self.assertIn("TtsSettingsActivity.this.finish();", tts)
        self.assertIn("bootstrapBackAction.run();", splash)
        self.assertIn("if (installationRunning)", splash)


if __name__ == "__main__":
    unittest.main()
