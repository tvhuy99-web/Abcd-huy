"""Regression contracts for the consolidated System menu and added settings pages."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class Group16SettingsCleanup(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hub = (JAVA / "GameActionHub.java").read_text("utf-8")
        cls.reader = (JAVA / "GameReaderController.java").read_text("utf-8")
        cls.settings = (ROOT / "src/main_menu.cpp").read_text("utf-8")
        cls.accessibility = (JAVA / "AccessibilitySettingsActivity.java").read_text("utf-8")
        cls.backup = (JAVA / "BackupRestoreActivity.java").read_text("utf-8")
        cls.old_setup = (JAVA / "ReaderSetupActivity.java").read_text("utf-8")
        cls.log = (JAVA / "DebugLogActivity.java").read_text("utf-8")

    def test_group_16_keeps_requested_commands_only(self):
        group = self.hub.split('{ "save", "quicksave", "quickload"', 1)[1]
        group = group.split("}", 1)[0]
        for command in ("open_options", "open_world_mods",
                        "open_distraction_manager", "help", "debug"):
            self.assertIn('"' + command + '"', group)
        for removed in ("main_menu", "open_autopickup", "open_autonotes",
                        "open_safemode", "open_color", "HELP_KEYBINDINGS",
                        "toggle_panel_adm", "toggle_pixel_minimap",
                        "toggle_fullscreen", "zoom_in", "zoom_out"):
            self.assertNotIn('"' + removed + '"', group)
        self.assertEqual(group.count('"debug"'), 1)
        self.assertIn("OPEN_DEBUG_LOG", self.hub)
        self.assertNotIn("OPEN_SETTINGS", self.hub)
        self.assertNotIn("GameSettingsActivity.class", self.reader)
        self.assertIn("onAccessibilityOpenDebugLog()", self.reader)

    def test_existing_native_options_and_tts_are_preserved(self):
        game_options = (ROOT / "src/options.cpp").read_text("utf-8")
        tts = (JAVA / "TtsSettingsActivity.java").read_text("utf-8")
        self.assertIn('snapshot.title = world_options_only ?', game_options)
        self.assertIn('"options:page:"', game_options)
        self.assertIn("saveTtsSettings(", tts)
        self.assertIn('"Main Menu|Settings", "TTS Settings"', self.settings)
        self.assertIn('"Main Menu|Settings", "Sound Settings"', self.settings)
        self.assertIn('"Main Menu|Settings", "Accessibility Settings"', self.settings)
        self.assertIn('"Main Menu|Settings", "Backup and Restore"', self.settings)
        self.assertIn("a11y::android::open_accessibility_settings()", self.settings)
        self.assertIn("a11y::android::open_backup_restore()", self.settings)
        self.assertNotIn("GameSettingsActivity.class", self.reader)

    def test_removed_reader_setup_has_no_duplicate_controls(self):
        self.assertIn("AccessibilitySettingsActivity.class", self.old_setup)
        for legacy in ("new CheckBox(", "new SeekBar(", "probeVoices(",
                       "new ReaderBackupArchive(", "setSpokenOnlyWorldEnabled("):
            self.assertNotIn(legacy, self.old_setup)

    def test_graphics_toggle_only_lives_in_accessibility_settings(self):
        self.assertNotIn("spokenOnlyWorld = new CheckBox(this)", self.log)
        self.assertIn("setSpokenOnlyWorldEnabled(spokenOnly)", self.accessibility)
        self.assertIn("nativeConfigureReaderPerformance(", self.accessibility)
        self.assertIn("setSwipeDistanceLevel(level)", self.accessibility)
        self.assertIn("setSwipeDurationLevel(level)", self.accessibility)
        self.assertEqual(self.accessibility.count("new CheckBox(this)"), 2)
        self.assertIn("setDirectionalSonarEnabled(checked)", self.accessibility)
        self.assertIn("nativeConfigureDirectionalSonar(checked)", self.accessibility)
        self.assertEqual(self.accessibility.count("new SeekBar(this)"), 2)

    def test_backup_remains_two_stage_and_blocks_active_game(self):
        self.assertIn("ReaderGameSession.canRestore()", self.backup)
        self.assertIn("stageImport(stream)", self.backup)
        self.assertIn("restoreStagedImport()", self.backup)
        self.assertIn("cancelStagedImport()", self.backup)
        self.assertIn("backupConfirmRestore.setEnabled(", self.backup)
        self.assertIn("Intent.ACTION_OPEN_DOCUMENT", self.backup)
        self.assertIn("Intent.ACTION_CREATE_DOCUMENT", self.backup)


if __name__ == "__main__":
    unittest.main()
