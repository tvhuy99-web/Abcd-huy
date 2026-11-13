"""Regression guards for real-device postmortem and mapgen speech defects."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
VI = ROOT / "lang/po/accessibility/vi.po"


class PostmortemAndMapgen(unittest.TestCase):
    def test_end_screen_back_and_visible_continue(self):
        source = (SRC / "end_screen.cpp").read_text(encoding="utf-8")
        self.assertIn('intent.action == "cancel"', source)
        self.assertIn('intent.node == "end:continue"', source)
        self.assertIn('return "TEXT.CONFIRM";', source)
        self.assertIn('continue_button.role = "button";', source)
        self.assertIn('continue_button.actions = { "focus", "activate" };', source)
        self.assertIn('_( "Your character has died" )', source)

    def test_score_rows_name_actual_achievements_and_state(self):
        source = (SRC / "scores_ui.cpp").read_text(encoding="utf-8")
        self.assertIn('ach->name().translated() + ". " + state', source)
        self.assertIn("achievements_labels.emplace_back( spoken_label )", source)
        self.assertIn("conducts_labels.emplace_back( spoken_label )", source)
        self.assertNotIn('_( "Achievement %d" )', source)
        self.assertNotIn('_( "Score %d" )', source)

    def test_document_swipes_and_back_navigate_paragraphs(self):
        controller = (JAVA / "GameReaderController.java").read_text("utf-8")
        reader = (JAVA / "ReaderDocumentReader.java").read_text("utf-8")
        self.assertIn("documentReader.nextParagraph()", controller)
        self.assertIn("documentReader.previousParagraph()", controller)
        self.assertIn("if (documentReader.isOpen()) {", controller)
        self.assertIn('"SCROLLABLE_TEXT".equals(next.category)', controller)
        self.assertIn("documentReader.open(", controller)
        self.assertIn("public String nextParagraph()", reader)
        self.assertIn("public String previousParagraph()", reader)
        self.assertIn("public void clear()", reader)

    def test_mapgen_does_not_skip_generation_and_records_delay(self):
        source = (SRC / "map.cpp").read_text(encoding="utf-8")
        self.assertIn("tmp_map.generate( grid_abs_omt, calendar::turn, true );", source)
        self.assertIn('"WORLD_MAPGEN_PHASE"', source)
        self.assertIn('"Generating a new map area. Please wait."', source)
        self.assertEqual(source.count("const weather_generator &wgen = get_weather().get_cur_weather_gen();"), 1)

    def test_vietnamese_grammar_and_punctuation(self):
        vi = VI.read_text("utf-8")
        controller = (JAVA / "GameReaderController.java").read_text("utf-8")
        self.assertIn('msgstr "Không thể di chuyển do %s."', vi)
        self.assertIn('msgstr "Bạn cố thoát khỏi cú tóm %1$s ở %2$s nhưng thất bại!"', vi)
        self.assertIn("appendTravelSentence(description, active.value);", controller)
        self.assertIn("appendTravelSentence(description, active.detail);", controller)


if __name__ == "__main__":
    unittest.main()
