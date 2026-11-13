"""Audit Vietnamese reader speech across menus, map, combat and safe dialogs.

Source tests protect all printf placeholders and semantics when shortening
labels. Android device tests check the punctuation normalizer itself.
"""
import re
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
PO = ROOT / "lang/po/accessibility/vi.po"
RES = ROOT / "android/app/src/main/res/values-vi"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


def xml_strings(path):
    return {item.attrib["name"]: item.text or ""
            for item in ET.parse(path).getroot().findall("string")}


def format_types(text):
    return sorted(m[-1] for m in re.findall(
        r"%(?:\d+\$)?[-+.#0-9]*[sdf]", text))


class VietnameseSpokenContentAudit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.po = PO.read_text("utf-8")
        cls.xml = xml_strings(RES / "a11y_reader.xml")
        cls.strings = xml_strings(RES / "strings.xml")
        cls.perception = (ROOT / "src/a11y_perception.cpp").read_text("utf-8")
        cls.world = (ROOT / "src/a11y_world.cpp").read_text("utf-8")
        cls.normalizer = (JAVA / "ReaderSpokenText.java").read_text("utf-8")
        cls.reader = (JAVA / "GameReaderController.java").read_text("utf-8")

    def test_all_single_line_vietnamese_translations_keep_printf_values(self):
        pairs = re.findall(r'^msgid "([^"\n]+)"\nmsgstr "([^"\n]+)"',
                           self.po, re.MULTILINE)
        self.assertGreater(len(pairs), 1100)
        for original, translated in pairs:
            with self.subTest(original=original):
                self.assertEqual(format_types(original), format_types(translated))
        self.assertIn('msgid "%1$s, %2$s, health %3$d percent."', self.po)
        self.assertIn('msgstr "%1$s, %2$s, sức khỏe còn %3$d phần trăm."', self.po)

    def test_all_android_vietnamese_resources_parse_and_keep_format_values(self):
        for name, source in (("a11y_reader.xml", RES / "a11y_reader.xml"),
                             ("strings.xml", RES / "strings.xml")):
            root = ET.parse(source).getroot()
            self.assertGreater(len(root.findall("string")), 50, name)
        for name in ("a11yBackupImportReady", "a11yDiagnosticsLatency",
                     "a11yDiagnosticsCues"):
            self.assertTrue(format_types(self.xml[name]))
        self.assertIn("%1$s", self.strings["accessibilityServicesMessage"])

    def test_concise_gestures_and_item_guidance_retains_controls(self):
        help_text = self.xml["a11yReaderGestureInstructions"]
        self.assertLess(len(help_text), 250)
        for word in ("chạm hai lần", "vuốt một ngón", "hai ngón", "TalkBack"):
            self.assertIn(word, help_text)
        combat = self.strings["a11yCombatOn"]
        self.assertLess(len(combat), 135)
        for word in ("một ngón", "hai ngón", "lên/xuống",
                     "trái/phải", "nạp đạn", "vũ khí"):
            self.assertIn(word, combat.lower())
        self.assertNotIn("ba ngón", combat.lower())
        self.assertRegex(self.po, r'chỗ chứa và (?:"\n")?quyền sở hữu')
        self.assertRegex(self.po, r'tốn lượt hoặc (?:"\n")?làm hỏng tài sản')

    def test_irreversible_backup_warnings_are_still_explicit(self):
        self.assertIn("ghi đè", self.xml["a11yBackupExportGuidance"])
        self.assertIn("không thay đổi bản lưu", self.xml["a11yBackupExportGuidance"])
        self.assertIn("chưa bị thay thế", self.xml["a11yBackupImportGuidance"].lower())
        ready = self.xml["a11yBackupImportReady"]
        for required in ("Xác nhận khôi phục", "Hủy khôi phục",
                         "giữ nguyên", "%1$d", "%2$d"):
            self.assertIn(required, ready)
        self.assertIn("Chưa thay đổi", self.xml["a11yBackupNoSpace"])

    def test_tile_speech_does_not_repeat_closed_door_or_sound_location(self):
        text = self.perception
        self.assertIn("observation.door != door_state::closed", text)
        self.assertIn("observation.door != door_state::locked", text)
        self.assertIn("Heard near here: %s. Position approximate.", text)
        self.assertIn("Nghe gần đây: %s. Vị trí chỉ ước lượng.", self.po)
        self.assertIn("observation.known_traps", text)
        self.assertIn("observation.seen_creatures", text)
        self.assertIn("observation.seen_items", text)

    def test_menu_title_is_read_once_without_losing_choice_data(self):
        self.assertIn("static String menuEntrySpeech(String introduction, String entry)", self.reader)
        self.assertIn("item.equals(title)", self.reader)
        self.assertIn('item.startsWith(title + ". ")', self.reader)
        self.assertIn('return title + ". " + item;', self.reader)
        self.assertIn("ReaderMenuSpeechTest.java",
                      str(ROOT / "android/app/src/androidTest/java/com/cleverraven/"
                          "cataclysmdda/ReaderMenuSpeechTest.java"))

    def test_speech_cleanup_keeps_data_and_deliberate_ellipses(self):
        self.assertIn("DUPLICATE_STOP", self.normalizer)
        self.assertIn("COLOR_TAG.matcher(text).replaceAll", self.normalizer)
        self.assertIn("(?<!\\\\.)\\\\.{2}(?!\\\\.)", self.normalizer)
        device = (ROOT / "android/app/src/androidTest/java/com/cleverraven/"
                  "cataclysmdda/ReaderSpokenTextTest.java").read_text("utf-8")
        self.assertIn("3.14 mét", device)
        self.assertIn(".45 Colt", device)
        self.assertIn("sức khỏe 97%", device)


if __name__ == "__main__":
    unittest.main()
