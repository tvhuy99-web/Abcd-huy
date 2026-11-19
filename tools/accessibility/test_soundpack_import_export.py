"""Source-level contracts for the app-owned soundpack management screen."""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
CPP = ROOT / "src"


class SoundpackImportExportContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ui = (JAVA / "SoundSettingsActivity.java").read_text()
        cls.archive = (JAVA / "SoundpackArchive.java").read_text()
        cls.bridge = (CPP / "a11y_android.cpp").read_text()
        cls.menu = (CPP / "main_menu.cpp").read_text()
        cls.audio = (CPP / "sdlsound.cpp").read_text()

    def test_controls_in_requested_order(self):
        names = [
            "selectedButton = addButton",
            "a11ySoundpackImport, view -> chooseZip(false)",
            "musicEnabled = new CheckBox",
            "musicVolume = new SeekBar",
            "effectsEnabled = new CheckBox",
            "effectsVolume = new SeekBar",
            "a11ySoundpackExport, view -> chooseZip(true)",
        ]
        positions = [self.ui.index(item) for item in names]
        self.assertEqual(positions, sorted(positions))

    def test_double_tap_and_long_hold_universal_guide(self):
        self.assertIn("new BootstrapSelfVoiceSurface", self.ui)
        self.assertIn("openContextActions() { showContextActions(); }", self.ui)
        self.assertIn("a11yGestureGuideTitle", self.ui)
        self.assertIn("showPanel(homeControls);", self.ui)

    def test_uses_saf_and_streaming_bounded_zip(self):
        self.assertIn("Intent.ACTION_OPEN_DOCUMENT", self.ui)
        self.assertIn("Intent.ACTION_CREATE_DOCUMENT", self.ui)
        self.assertIn("ZipInputStream", self.archive)
        self.assertIn("ZipOutputStream", self.archive)
        self.assertIn("MAX_EXPANDED", self.archive)
        self.assertIn("safeRelative(name)", self.archive)
        self.assertIn("inside(stage, target)", self.archive)
        self.assertIn("!source.renameTo(destination)", self.archive)
        self.assertIn("if (destination.exists())", self.archive)

    def test_selected_soundpack_export_not_arbitrary_file(self):
        self.assertIn("final String id = selectedSoundpack;", self.ui)
        self.assertIn("SoundpackArchive.exportZip(output, userSound, bundledSound, id)", self.ui)
        self.assertIn("selected.folder", self.archive)

    def test_native_selection_applied_by_game_thread_with_rollback(self):
        self.assertIn("nativeQueueSoundpackSelection", self.ui)
        self.assertIn("queue_soundpack_selection", self.bridge)
        self.assertIn("apply_pending_soundpack_selection();", self.menu)
        self.assertIn('get_option( "SOUNDPACKS" ).setValue( previous )', self.bridge)
        self.assertIn("reload_soundset();", self.bridge)
        self.assertIn("void load_soundset()", self.audio)
        self.assertIn("bool reload_soundset()", self.audio)

    def test_sound_does_not_touch_speech_reader_settings(self):
        self.assertIn("AndroidSpeech", self.ui)
        self.assertNotIn("nativeConfigureReader", self.ui)
        self.assertNotIn("TtsSettingsActivity", self.archive)


if __name__ == "__main__":
    unittest.main()
