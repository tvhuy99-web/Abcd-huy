"""Save reload must never be mistaken for Quick Play new-character creation."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class SaveLoadDistinction(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.menu = (ROOT / "src/main_menu.cpp").read_text(encoding="utf-8")
        cls.engine = (ROOT / "src/game.cpp").read_text(encoding="utf-8")
        cls.io = (ROOT / "src/game_io.cpp").read_text(encoding="utf-8")
        cls.header = (ROOT / "src/game.h").read_text(encoding="utf-8")
        cls.vi = (ROOT / "lang/po/accessibility/vi.po").read_text(
            encoding="utf-8")

    def test_loading_saved_game_never_calls_start_game(self):
        load = self.menu.split("bool main_menu::load_game(", 1)[1]
        load = load.split("bool main_menu::load_character_tab(", 1)[0]
        self.assertIn("g->setup( true );", load)
        self.assertIn("loaded = g->load( savegame );", load)
        self.assertNotIn("g->start_game()", load)
        self.assertIn("if( loaded )", load)
        self.assertIn("Save loaded: %s.", load)
        self.assertIn("cleanup.cancel();", load)

    def test_new_character_remains_explicit_and_resumable(self):
        new = self.menu.split("bool main_menu::new_character_tab(", 1)[1]
        new = new.split("bool main_menu::load_game(", 1)[0]
        self.assertIn("const bool is_play_now = sel2 == 3 || sel2 == 4;",
                      new)
        self.assertIn("has_saves && !query_yn", new)
        self.assertIn("Create another new character?", new)
        self.assertIn("g->start_game()", new)
        self.assertIn("Creates a NEW character.", self.menu)

    def test_engine_distinguishes_world_reload_without_fake_progress(self):
        self.assertIn(
            "void setup( bool loading_saved_character = false );",
            self.header)
        self.assertIn(
            "void game::setup( bool loading_saved_character )", self.engine)
        self.assertIn('_( "Loading saved character." )', self.engine)
        self.assertIn('_( "Preparing new character." )', self.engine)
        self.assertIn("g->setup( true );", self.io)
        self.assertIn("loading_ui::show( _( \"Loading the save…\" )",
                      self.io)

    def test_slow_phases_are_measured_without_changing_saves(self):
        self.assertIn('A11Y_LOAD_TIMING mode=', self.engine)
        self.assertIn("core_ms=", self.engine)
        self.assertIn("mods_ms=", self.engine)
        self.assertIn("character_and_world_ms", self.engine)
        self.assertIn('A11Y_LOAD_TIMING mode=resume stage=', self.io)
        self.assertIn("save_total ms=", self.io)
        self.assertIn("if( !save_player_data()", self.io)
        self.assertIn("world_generator->active_world->add_save(", self.io)

    def test_vietnamese_loading_labels(self):
        self.assertIn('msgid "Loading saved character."', self.vi)
        self.assertIn('msgstr "Đang tải nhân vật đã lưu."', self.vi)
        self.assertIn('msgid "Preparing new character."', self.vi)
        self.assertIn('msgstr "Đang tạo nhân vật mới."', self.vi)
        self.assertIn("Đã tải bản lưu: %s.", self.vi)


if __name__ == "__main__":
    unittest.main()
