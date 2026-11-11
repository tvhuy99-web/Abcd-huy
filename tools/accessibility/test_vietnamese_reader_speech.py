"""Ensure Android dialogs speak their questions and nearby speech is concise."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = (ROOT / "android/app/src/main/java/com/cleverraven"
        "/cataclysmdda/GameReaderController.java")
PO = ROOT / "lang/po/accessibility/vi.po"


class VietnameseReaderSpeechTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.surroundings = (SRC / "surroundings_menu.cpp").read_text("utf-8")
        cls.dialog = (SRC / "popup.cpp").read_text("utf-8")
        cls.java = JAVA.read_text("utf-8")
        cls.po = PO.read_text("utf-8")

    def test_nearby_directions_do_not_repeat_distance(self):
        self.assertIn("direction::NORTHWEST", self.surroundings)
        self.assertIn("std::abs( x ) == std::abs( y )", self.surroundings)
        self.assertIn('node.value = relative_words( entry.relative );',
                      self.surroundings)
        self.assertIn('"%1$s, %2$d tiles"', self.surroundings)
        self.assertIn('"%1$s %2$d tiles, %3$s %4$d tiles"',
                      self.surroundings)

    def test_yes_no_dialog_speaks_question_without_a_fake_row(self):
        self.assertIn('const bool yes_no_prompt = category == "YESNO"', self.dialog)
        self.assertIn('if( !reader_snapshot.summary.empty() && !yes_no_prompt )', self.dialog)
        self.assertIn('question.label = reader_snapshot.summary;', self.dialog)
        self.assertIn('node.detail = reader_snapshot.summary;', self.dialog)
        self.assertNotIn('question.label = _( "Dialog message" );', self.dialog)
        self.assertIn('"YESNO".equals(next.category)', self.java)
        self.assertIn('String introduction = yesNo ? next.summary.trim() : next.title;', self.java)
        self.assertIn('menuEntrySpeech(introduction,', self.java)

    def test_vietnamese_localization(self):
        self.assertIn('msgid "northwest"\nmsgstr "tây bắc"', self.po)
        self.assertIn('msgid "%1$s, %2$d tiles"\nmsgstr "%1$s, %2$d ô"',
                      self.po)
        self.assertIn('msgid "northwest"\nmsgstr "tây bắc"', self.po)
        self.assertNotIn('question.label = _( "Dialog message" );', self.dialog)


if __name__ == "__main__":
    unittest.main()
