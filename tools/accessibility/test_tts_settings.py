"""Static checks for the Android TTS menu and persisted controls."""

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android" / "app" / "src" / "main" / "java"
JAVA = JAVA / "com" / "cleverraven" / "cataclysmdda"
SOURCE = ROOT / "src"


class TtsSettingsWiringTest(unittest.TestCase):
    def test_menu_launches_native_tts_settings(self):
        menu = (SOURCE / "main_menu.cpp").read_text(encoding="utf-8")
        bridge = (SOURCE / "a11y_android.cpp").read_text(encoding="utf-8")
        activity = (JAVA / "CataclysmDDA.java").read_text(
            encoding="utf-8"
        )
        self.assertIn('pgettext( "Main Menu|Settings", "TTS Settings" )', menu)
        self.assertIn("a11y::android::open_tts_settings()", menu)
        self.assertIn("onAccessibilityOpenTtsSettings", bridge)
        self.assertIn("new Intent(this, TtsSettingsActivity.class)", activity)

    def test_enable_toggle_is_first_tts_setting(self):
        settings = (JAVA / "TtsSettingsActivity.java").read_text(
            encoding="utf-8"
        )
        ordered = (
            "column.addView(enabled);",
            "engineSpinner = selector(",
            "languageSpinner = selector(",
            "voiceSpinner = selector(",
            "rate = slider(",
            "pitch = slider(",
            "volume = slider(",
            "sampleButton = button(",
            "button(column, R.string.a11yTtsCancel",
            "button(column, R.string.a11yTtsSave",
        )
        positions = [settings.index(part) for part in ordered]
        self.assertEqual(positions, sorted(positions))
        self.assertNotIn("column.addView(online)", settings)
        self.assertNotIn("a11yTtsRestoreDefaults, view ->", settings)
        self.assertNotIn("a11yAndroidVoiceSettings, view ->", settings)
        self.assertIn("saved.isEnabled()", settings)
        self.assertIn("previewPassed", settings)
        self.assertIn("saveTtsSettings(", settings)

    def test_pitch_and_volume_reach_real_android_speech(self):
        driver = (JAVA / "AndroidSpeech.java").read_text(
            encoding="utf-8"
        )
        queue = (JAVA / "ReaderSpeechQueue.java").read_text(
            encoding="utf-8"
        )
        self.assertIn("engine.setPitch(pitch)", driver)
        self.assertIn("TextToSpeech.Engine.KEY_PARAM_VOLUME", driver)
        self.assertIn("initPitch = preferences.getPitch()", queue)
        self.assertIn("initVolume = preferences.getVolume()", queue)

    def test_setup_saves_atomically_and_mute_survives_restart(self):
        preferences = (JAVA / "ReaderPreferences.java").read_text(
            encoding="utf-8"
        )
        self.assertIn('editor.putBoolean("enabled", enabled)', preferences)
        self.assertIn('editor.putFloat("tts_pitch"', preferences)
        self.assertIn('editor.putFloat("tts_volume"', preferences)
        self.assertIn("editor.apply()", preferences)


if __name__ == "__main__":
    unittest.main()
