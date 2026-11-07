"""Regression checks for approved concise Vietnamese accessibility speech.

These check the source speech contract. Native/Android builds and real-device
TalkBack/TTS tests remain separate.
"""

import unittest
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
WORLD = ROOT / "src" / "a11y_world.cpp"
VI_PO = ROOT / "lang" / "po" / "accessibility" / "vi.po"
ANDROID = ROOT / "android" / "app"
JAVA_DIR = ANDROID / "src/main/java/com/cleverraven/cataclysmdda"
SPLASH = JAVA_DIR / "SplashScreen.java"
READER = JAVA_DIR / "GameReaderController.java"
VALUES = ANDROID / "src/main/res"


class ConciseSpeechTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.world = WORLD.read_text(encoding="utf-8")
        cls.po = VI_PO.read_text(encoding="utf-8")
        cls.splash = SPLASH.read_text(encoding="utf-8")
        cls.reader = READER.read_text(encoding="utf-8")

    def test_scan_starts_at_first_tile_without_repeating_direction(self):
        body = self.world.split("void announce_linear_scan(", 1)[1].split(
            "void announce_player_coordinates()", 1
        )[0]
        self.assertIn("std::string spoken;", body)
        self.assertIn(
            "for( int distance = 1; distance <= 16; ++distance )", body
        )
        self.assertIn(
            'string_format( _( "Tile %1$d: %2$s." ), distance, detail )',
            body,
        )
        self.assertNotIn(
            'string_format( _( "Scan %s, 16 tiles, nearest to farthest."',
            body,
        )
        self.assertIn('"A11Y_SCAN:" + spoken', body)
        self.assertIn('msgstr "Ô %1$d, %2$s."', self.po)

    def test_short_action_outcomes_and_coordinates(self):
        for translation in (
            'msgstr "Bị chặn"',
            'msgstr "Bẫy %s"',
            'msgstr "Xe %s"',
            'msgstr "X %1$d, Y %2$d, Z %3$d."',
            'msgstr "Chưa rõ kết quả."',
            'msgstr "%s chảy máu nặng hơn."',
        ):
            self.assertIn(translation, self.po)

    def test_real_game_safety_warnings_stay_available(self):
        # A shortened scan must not silence genuinely urgent events or
        # change how irreversible operations are described.
        self.assertIn('if( part.bleeding > old->bleeding )', self.world)
        self.assertIn('announce_game_message( message, urgent );', self.world)
        self.assertIn("Delete the world", self.po)
        self.assertIn("Reset the world", self.po)
        self.assertIn("This cannot be undone", self.po)
        self.assertIn(
            "a11yBackupImportReady",
            (VALUES / "values-vi/a11y_reader.xml").read_text("utf-8"),
        )

    def test_installation_announces_only_quarter_milestones(self):
        rest = self.splash.split(
            "private void reportBootstrapProgress(", 1
        )[1]
        body = rest.split("private void showCrashAlert()", 1)[0]
        self.assertIn("int bucket = percent / 25;", body)
        self.assertIn("if (bucket > lastBootstrapProgressBucket)", body)
        self.assertIn("if (bucket > 0)", body)
        self.assertIn("bucket * 25", body)
        self.assertNotIn("percent / 10", body)
        self.assertEqual(1, body.count("speakBootstrap(progress)"))
        for locale in ("values", "values-vi"):
            root = ET.parse(VALUES / locale / "a11y_reader.xml").getroot()
            strings = {
                elem.attrib.get("name"): elem.text
                for elem in root.findall("string")
            }
            self.assertIn("%1$d%%", strings["a11yBootstrapProgress"])

    def test_resource_xml_is_valid_and_speech_contract_is_bilingual(self):
        for locale in ("values", "values-vi"):
            strings = ET.parse(VALUES / locale / "strings.xml").getroot()
            reader = ET.parse(VALUES / locale / "a11y_reader.xml").getroot()
            editor = ET.parse(VALUES / locale / "a11y_editor.xml").getroot()
            for root in (strings, reader, editor):
                self.assertTrue(root.findall("string"))
            compact = next(
                elem.text for elem in reader.findall("string")
                if elem.attrib.get("name") == "a11yReaderStale"
            )
            self.assertLess(len(compact), 35)
        self.assertIn("ReaderSpeechQueue.URGENT", self.reader)


if __name__ == "__main__":
    unittest.main()
