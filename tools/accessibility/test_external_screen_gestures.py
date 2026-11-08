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
        setup = (JAVA / "ReaderSetupActivity.java").read_text(encoding="utf-8")
        self.assertIn("dy1 >= minimumDistance && dy2 >= minimumDistance", detector)
        self.assertIn("Math.abs(dx1) * 1.35f", detector)
        self.assertIn("ACTION_POINTER_UP", detector)
        for code in (surface, setup):
            self.assertIn("TwoFingerBackGesture.matches(", code)
            self.assertIn("ignoreUntilPointersReleased", code)
            self.assertIn("ACTION_POINTER_UP", code)
        self.assertIn("listener.goBack();", surface)
        self.assertIn("ReaderSetupActivity.this.onBackPressed();", setup)
        self.assertIn("return super.dispatchTouchEvent(event)", setup)

    def test_vertical_swipe_changes_focused_slider_and_reads_value(self):
        surface = (JAVA / "BootstrapSelfVoiceSurface.java").read_text(encoding="utf-8")
        setup = (JAVA / "ReaderSetupActivity.java").read_text(encoding="utf-8")
        tts = (JAVA / "TtsSettingsActivity.java").read_text(encoding="utf-8")
        self.assertIn("adjustFocusedSetting(dy < 0 ? 1 : -1)", surface)
        self.assertIn("bar.setProgress(", surface)
        self.assertIn("speakFocused();", surface)
        self.assertIn("adjustSelfVoiceSetting(dy < 0 ? 1 : -1)", setup)
        self.assertIn("speakFocusedControl();", setup)
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
