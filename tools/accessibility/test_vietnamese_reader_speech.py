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

    def test_logged_english_speech_has_exact_vietnamese_gettext_entries(self):
        # Real speech leaks from Quick Play, fighting-style selection,
        # crafting navigation and zone management in Android session logs.
        pairs = {
            "Preparing a new world. Please wait.": "Đang chuẩn bị thế giới mới. Vui lòng chờ.",
            "Creating your character and starting the game.": "Đang tạo nhân vật và bắt đầu trò chơi.",
            "Currently active fighting style.": "Đang sử dụng thế võ này.",
            "Not currently active.": "Chưa sử dụng thế võ này.",
            "Active fighting style: %s.": "Thế võ đang sử dụng: %s.",
            "Personal": "Cá nhân",
            "tiles": "ô",
            "Jump to beginning": "Đến đầu danh sách",
            "Jump to end": "Đến cuối danh sách",
            "Showing all zones": "Đang hiển thị tất cả các vùng",
            "Hiding distant zones": "Đang ẩn các vùng ở xa",
            "Keep hands free enabled.": "Đã bật chế độ giữ tay rảnh.",
            "Keep hands free disabled.": "Đã tắt chế độ giữ tay rảnh.",
            "locker": "tủ đựng đồ",
            "Place Arcade Machine": "Đặt máy trò chơi điện tử",
            "Remove Linoleum Tiles": "Bóc gạch lát sàn vải sơn",
        }
        for english, vietnamese in pairs.items():
            with self.subTest(english=english):
                self.assertIn('msgid "' + english + '"\nmsgstr "' + vietnamese + '"',
                              self.po)
        self.assertIn('msgctxt "zones manager"\nmsgid "Personal"\nmsgstr "Cá nhân"',
                      self.po)

    def test_reader_commands_translate_crafting_navigation_and_zone_units(self):
        craft = (SRC / "crafting_gui.cpp").read_text("utf-8")
        zones = (SRC / "zone_manager_ui.cpp").read_text("utf-8")
        style = (SRC / "character.cpp").read_text("utf-8")
        loading_menu = (SRC / "main_menu.cpp").read_text("utf-8")
        game = (SRC / "game.cpp").read_text("utf-8")
        self.assertIn('ctxt.register_action( "HOME", to_translation( "Jump to beginning" ) );',
                      craft)
        self.assertIn('ctxt.register_action( "END", to_translation( "Jump to end" ) );',
                      craft)
        self.assertIn('{ "distance", _( "Distance" ), _( "tiles" ) }', zones)
        self.assertIn('_( "Showing all zones" )', zones)
        self.assertIn('_( "Hiding distant zones" )', zones)
        self.assertIn('_( "Currently active fighting style." )', style)
        self.assertIn('_( "Not currently active." )', style)
        self.assertIn('_( "Active fighting style: %s." )', style)
        self.assertIn('_( "Keep hands free enabled." )', style)
        self.assertIn('_( "Keep hands free disabled." )', style)
        self.assertIn('_( "Preparing a new world. Please wait." )', loading_menu)
        self.assertIn('_( "Creating your character and starting the game." )', game)
        furniture = (ROOT / "data/json/furniture_and_terrain/furniture-storage.json").read_text("utf-8")
        groups = (ROOT / "data/json/construction_group.json").read_text("utf-8")
        self.assertIn('"name": "locker"', furniture)
        self.assertIn('"name": "Place Arcade Machine"', groups)
        self.assertIn('"name": "Remove Linoleum Tiles"', groups)

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
