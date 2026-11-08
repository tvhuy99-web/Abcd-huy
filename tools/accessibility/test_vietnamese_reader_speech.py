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

    def test_dialog_question_is_navigable_and_announced(self):
        self.assertIn('question.id = "dialog:message";', self.dialog)
        self.assertIn('question.value = reader_snapshot.summary;', self.dialog)
        self.assertIn('"YESNO".equals(next.category)', self.java)
        self.assertIn('introduction += ". " + next.summary.trim();', self.java)
        self.assertIn('!"dialog:message".equals(node.id)', self.java)

    def test_vietnamese_localization(self):
        self.assertIn('msgid "northwest"\nmsgstr "tây bắc"', self.po)
        self.assertIn('msgid "%1$s, %2$d tiles"\nmsgstr "%1$s, %2$d ô"',
                      self.po)
        self.assertIn('msgid "Dialog message"\nmsgstr "Nội dung câu hỏi"',
                      self.po)


if __name__ == "__main__":
    unittest.main()
