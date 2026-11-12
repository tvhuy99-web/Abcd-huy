"""Cross-screen spoken navigation contracts, including Android settings and native menus."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
RES = ROOT / "android/app/src/main/res"


def load(name):
    return (JAVA / name).read_text("utf-8")


class GlobalFocusNavigationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.surface = load("BootstrapSelfVoiceSurface.java")
        cls.tts = load("TtsSettingsActivity.java")
        cls.sound = load("SoundSettingsActivity.java")
        cls.splash = load("SplashScreen.java")
        cls.gesture = load("ReaderGestureSurface.java")
        cls.controller = load("GameReaderController.java")
        cls.queue = load("ReaderSpeechQueue.java")

    def test_right_next_left_previous_on_all_menu_surfaces(self):
        self.assertIn("navigate(dx > 0 ? 1 : -1);", self.surface)
        self.assertIn("focus = Math.floorMod(focus + delta, items.size())", self.surface)
        self.assertNotIn("listener.leftHandedNavigation() ? -delta : delta", self.surface)
        nav = self.gesture.split(
            "static int navigationDeltaForHorizontalSwipe(", 1
        )[1].split("private boolean valid()", 1)[0]
        self.assertIn("return dx > 0 ? 1 : -1;", nav)
        self.assertNotIn("leftHanded ? -delta : delta", nav)
        self.assertIn("listener.moveDirection(dx > 0 ? 1 : -1, 0)", self.gesture)
        self.assertIn("listener.moveDirection(0, dy > 0 ? 1 : -1)", self.gesture)

    def test_new_focus_cancels_old_audio_in_all_android_setting_screens(self):
        self.assertIn("listener.interruptForFocus();", self.surface)
        self.assertIn("listener.speak(\"bootstrap-focus-\" + focus, text);", self.surface)
        self.assertIn("default void interruptForFocus() { stopSpeech(); }", self.surface)
        self.assertIn("public void interruptForFocus()", self.splash)
        self.assertIn("if (bootstrapSpeech != null) { bootstrapSpeech.cancel(); }", self.splash)
        self.assertIn("speech.focus(node.id, spoken);", self.controller)
        self.assertIn("speech.cancel();", self.tts)
        self.assertIn("speech.cancel();", self.sound)
        self.assertIn("(priority == FOCUS && active.priority == FOCUS)", self.queue)

    def test_every_screen_reads_name_type_value_and_position(self):
        self.assertIn("getContext().getString(controlRole(view))", self.surface)
        self.assertIn("R.string.a11yPosition, focus + 1, items.size()", self.surface)
        self.assertIn("item.setAccessibilityDelegate(", self.surface)
        self.assertIn("info.setContentDescription(text + \", \" + position)", self.surface)
        self.assertIn("R.string.a11yControlButton", self.controller)
        self.assertIn("R.string.a11yControlCheckbox", self.controller)
        self.assertIn("R.string.a11yControlChoice", self.controller)
        self.assertIn("R.string.a11yControlSlider", self.controller)
        self.assertIn("R.string.a11yPosition, spokenPosition, spokenTotal", self.controller)
        for locale in ("values", "values-vi"):
            text = (RES / locale / "strings.xml").read_text("utf-8")
            for key in ("a11yControlButton", "a11yControlCheckbox",
                        "a11yControlChoice", "a11yControlSlider"):
                self.assertIn('name="' + key + '"', text)

    def test_tts_choice_list_is_navigable_and_back_safe(self):
        self.assertIn("openChoiceList((Spinner) view);", self.surface)
        self.assertIn("spinner.setSelection(option);", self.surface)
        self.assertIn("showContent(choices, selected);", self.surface)
        self.assertIn("showContent(primaryContent, previousFormFocus);", self.surface)
        self.assertIn("public boolean dismissChoiceList()", self.surface)
        self.assertIn("surface.dismissChoiceList()", self.tts)
        for name in ("engineSpinner", "languageSpinner", "voiceSpinner"):
            self.assertIn(name + " = selector(", self.tts)
        self.assertNotIn("spinner.setSelection((spinner.getSelectedItemPosition() + 1)", self.surface)

    def test_talkback_uses_native_controls_and_internal_voice_uses_surface(self):
        self.assertIn("if (!listener.isActive())", self.surface)
        self.assertIn("return super.dispatchTouchEvent(event);", self.surface)
        self.assertIn("manager.isTouchExplorationEnabled()", self.tts)
        self.assertIn("manager.isTouchExplorationEnabled()", self.sound)
        self.assertIn("manager.isTouchExplorationEnabled()", self.splash)
        self.assertIn("manager.isTouchExplorationEnabled()", self.controller)


if __name__ == "__main__":
    unittest.main()
