"""The original game Settings tab gains two entries without a duplicate settings hub."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class NativeSettingsAndMapGenerationSpeech(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.menu = (ROOT / "src/main_menu.cpp").read_text(encoding="utf-8")
        cls.android_bridge = (ROOT / "src/a11y_android.cpp").read_text(encoding="utf-8")
        cls.android_header = (ROOT / "src/a11y_android.h").read_text(encoding="utf-8")
        cls.activity = (JAVA / "CataclysmDDA.java").read_text(encoding="utf-8")
        cls.hub = (JAVA / "GameActionHub.java").read_text(encoding="utf-8")
        cls.reader = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        cls.translations = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")

    def test_existing_nine_settings_are_preserved_and_two_appended(self):
        # New master moved these entries into game_settings_items() so the
        # Settings menu can also open in-game. Their order must stay stable.
        if "vSettingsSubItems.clear();" in self.menu:
            start = self.menu.index("vSettingsSubItems.clear();")
            end = self.menu.index("vSettingsHotkeys.clear();", start)
            block = self.menu[start:end]
            names = re.findall(
                r'vSettingsSubItems.emplace_back\\( pgettext\\( "Main Menu\\|Settings", "([^"]+)"',
                block)
        else:
            start = self.menu.index("static std::vector<std::string> game_settings_items()")
            end = self.menu.index("return items;", start)
            block = self.menu[start:end]
            names = re.findall(
                r'items.emplace_back\\( pgettext\\( "Main Menu\\|Settings", "([^"]+)"',
                block)
            self.assertIn("vSettingsSubItems = game_settings_items();", self.menu)
        self.assertEqual(names, [
            "<O|o>ptions", "Ke<y|Y>bindings", "A<u|U>topickup",
            "Sa<f|F>emode", "Colo<r|R>s", "ImGui <S|s>tyles",
            "<I|i>mGui Demo Screen", "TTS Settings", "Sound Settings",
            "Accessibility Settings", "Backup and Restore",
        ])
        for index, expected in ((7, "open_tts_settings"),
                                (8, "open_sound_settings"),
                                (9, "open_accessibility_settings"),
                                (10, "open_backup_restore")):
            self.assertTrue(f"sel2 == {index}" in self.menu
                            or f"choice == {index}" in self.menu)
            self.assertIn("a11y::android::" + expected, self.menu)

    def test_android_bridge_launches_only_original_settings_children(self):
        for item in ("AccessibilitySettings", "BackupRestore"):
            self.assertIn(f"onAccessibilityOpen{item}", self.activity)
            self.assertIn(f'GetMethodID( cls, "onAccessibilityOpen{item}", "()V" )',
                          self.android_bridge)
        self.assertIn("new Intent(this, AccessibilitySettingsActivity.class)", self.activity)
        self.assertIn("new Intent(this, BackupRestoreActivity.class)", self.activity)
        self.assertIn("void open_accessibility_settings();", self.android_header)
        self.assertIn("void open_backup_restore();", self.android_header)

    def test_second_android_settings_menu_is_gone(self):
        self.assertNotIn("OPEN_SETTINGS", self.hub)
        self.assertNotIn("GameSettingsActivity.class", self.reader)
        self.assertNotIn("GameSettingsActivity", (
            ROOT / "android/app/src/main/AndroidManifest.xml").read_text("utf-8"))
        self.assertFalse((JAVA / "GameSettingsActivity.java").exists())
        # The native Settings command was renamed on newer master commits.
        self.assertTrue(any('"' + command + '"' in self.hub for command in (
            "open_options", "open_game_settings"
        )))
        self.assertIn('"hub:settings".equals(target)', self.reader)

    def test_vietnamese_names_have_context_translation(self):
        for original, translated in (("Accessibility Settings", "Cài đặt trợ năng"),
                                     ("Backup and Restore", "Sao lưu và khôi phục")):
            self.assertIn('msgctxt "Main Menu|Settings"\nmsgid "' +
                          original + '"\nmsgstr "' + translated + '"', self.translations)

    def test_only_spoken_map_generation_wait_notice_is_suppressed(self):
        self.assertIn('"Đang tạo khu vực bản đồ mới. Vui lòng chờ.".equals(spokenMessage)',
                      self.reader)
        self.assertIn("MAPGEN_SPEECH_SUPPRESSED", self.reader)
        self.assertIn("pendingGameMessages.addLast(", self.reader)
        self.assertIn("speech.say(\"loading-progress\"", self.reader)
        self.assertIn("speech.say(\"game-message:\"", self.reader)


if __name__ == "__main__":
    unittest.main()
