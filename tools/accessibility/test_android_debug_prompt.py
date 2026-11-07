"""Android error reports must be spoken without blocking on stale JSON caches."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class DebugPromptAccessibility(unittest.TestCase):
    def test_android_cache_rebuilds_without_a_modal(self):
        source = (ROOT / "src/flexbuffer_cache.cpp").read_text(encoding="utf-8")
        self.assertIn('#if defined(__ANDROID__)', source)
        self.assertIn("Regenerating stale Android JSON cache", source)
        self.assertIn("remove_file( disk_entry->second.flexbuffer_path.u8string() );",
                      source)

    def test_native_bridge_includes_actions_and_wake_event(self):
        native = (ROOT / "src/a11y_android.cpp").read_text(encoding="utf-8")
        debug = (ROOT / "src/debug.cpp").read_text(encoding="utf-8")
        activity = (JAVA / "CataclysmDDA.java").read_text(encoding="utf-8")
        self.assertIn("present_debug_prompt( formatted_report );", debug)
        self.assertIn("take_debug_prompt_response()", debug)
        self.assertIn("dismiss_debug_prompt()", debug)
        self.assertIn("PushWakeEvent();", native)
        self.assertIn("nativeDebugPromptResponse(int action);", activity)
        self.assertIn("onAccessibilityDebugPrompt(String report)", activity)

    def test_controls_are_self_voiced_without_legacy_hub(self):
        reader = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        self.assertIn("public void showDebugPrompt(String report)", reader)
        self.assertIn('speech.say("debug-report", debugReport', reader)
        self.assertIn("submitDebugResponse(debugFocus)", reader)
        self.assertIn("if (debugReport != null) { submitDebugResponse(1); return; }",
                      reader)
        self.assertIn("root.setVisibility(View.VISIBLE);", reader)
        self.assertNotIn("showComprehensiveMenu(", reader)
        self.assertNotIn("showControls(", reader)


if __name__ == "__main__":
    unittest.main()
