"""Source regression coverage for the separate Android audio settings."""

import unittest
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
SOUND = JAVA / "SoundSettingsActivity.java"
BRIDGE = ROOT / "src/a11y_android.cpp"
MENU = ROOT / "src/main_menu.cpp"
RES = ROOT / "android/app/src/main/res"


class SoundSettingsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.sound = SOUND.read_text(encoding="utf-8")
        cls.menu = MENU.read_text(encoding="utf-8")
        cls.bridge = BRIDGE.read_text(encoding="utf-8")
        cls.app = (JAVA / "CataclysmDDA.java").read_text(encoding="utf-8")

    def test_four_controls_in_requested_order(self):
        controls = (
            "musicEnabled = new CheckBox(this);",
            "musicVolume = new SeekBar(this);",
            "effectsEnabled = new CheckBox(this);",
            "effectsVolume = new SeekBar(this);",
        )
        indices = [self.sound.index(control) for control in controls]
        self.assertEqual(indices, sorted(indices))
        self.assertIn("musicVolume.setMax(128)", self.sound)
        self.assertIn("effectsVolume.setMax(128)", self.sound)
        self.assertIn("getSharedPreferences(PREFS, MODE_PRIVATE)", self.sound)

    def test_native_options_are_applied_from_game_thread_only(self):
        self.assertIn("nativeQueueSoundSettings", self.sound)
        self.assertIn("nativeQueueSoundSettings", self.app)
        self.assertIn("queue_sound_settings", self.bridge)
        self.assertIn("apply_pending_sound_settings", self.bridge)
        self.assertIn(
            "a11y::android::apply_pending_sound_settings();",
            self.menu,
        )
        self.assertIn('get_option( "MUSIC_VOLUME" ).setValue(', self.bridge)
        self.assertIn(
            'get_option( "SOUND_EFFECT_VOLUME" ).setValue(',
            self.bridge,
        )
        self.assertIn('get_option( "SOUND_ENABLED" ).setValue(', self.bridge)
        self.assertIn("get_options().save();", self.bridge)
        self.assertIn(
            'pgettext( "Main Menu|Settings", "Sound Settings" )',
            self.menu,
        )
        self.assertIn("onAccessibilityOpenSoundSettings", self.app)

    def test_mute_remembers_slider_levels(self):
        self.assertIn('putInt("music_level", music)', self.sound)
        self.assertIn('putInt("effects_level", effects)', self.sound)
        self.assertIn("musicOn ? music : 0", self.sound)
        self.assertIn("effectsOn ? effects : 0", self.sound)
        self.assertIn("musicOn || effectsOn", self.sound)

    def test_native_ui_and_localizations(self):
        manifest = (
            RES.parent / "AndroidManifest.xml"
        ).read_text(encoding="utf-8")
        self.assertIn('android:name="SoundSettingsActivity"', manifest)
        names = {
            "a11ySoundSettingsTitle",
            "a11yMusicEnabled",
            "a11yMusicVolumeValue",
            "a11yEffectsEnabled",
            "a11yEffectsVolumeValue",
        }
        for locale in ("values", "values-vi"):
            root = ET.parse(RES / locale / "strings.xml").getroot()
            found = {x.attrib.get("name") for x in root.findall("string")}
            self.assertTrue(names.issubset(found))
        translation = (
            ROOT / "lang/po/accessibility/vi.po"
        ).read_text(encoding="utf-8")
        self.assertIn('msgid "Sound Settings"', translation)
        self.assertIn('msgstr "Cài đặt âm thanh"', translation)


if __name__ == "__main__":
    unittest.main()
