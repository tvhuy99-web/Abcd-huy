"""Regression checks for menu group 15 speech and safety controls."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


class HubSystemAccessibilityTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.autonote = (SRC / "auto_note.cpp").read_text("utf-8")
        cls.distraction = (SRC / "distraction_manager.cpp").read_text("utf-8")
        cls.handle = (SRC / "handle_action.cpp").read_text("utf-8")
        cls.game = (SRC / "game.cpp").read_text("utf-8")
        cls.debug = (SRC / "debug_menu.cpp").read_text("utf-8")

    def test_notes_reader_lists_each_discovered_item_and_switches_tabs(self):
        self.assertIn('"auto-notes-manager"', self.autonote)
        self.assertIn('"autonotes:item:"', self.autonote)
        self.assertIn('node.checked = entry.second;', self.autonote)
        self.assertIn('entry.first.name()', self.autonote)
        self.assertIn('node.total = static_cast<int>( shown.size() );', self.autonote)
        self.assertIn('bCharacter = intent.node == "autonotes:tab:character";', self.autonote)
        self.assertIn('return intent.action == "activate" ? "CONFIRM" : "TIMEOUT";', self.autonote)
        self.assertIn('if( query_yn( _( "Save changes?" ) ) )', self.autonote)

    def test_distraction_reader_uses_same_native_toggle(self):
        self.assertIn('"distraction-manager"', self.distraction)
        self.assertIn('node.checked = *item.state;', self.distraction)
        self.assertIn('node.detail = _( item.description.c_str() );', self.distraction)
        self.assertIn('return intent.action == "activate" ? "CONFIRM" : "TIMEOUT";', self.distraction)
        self.assertIn('if( action == "CONFIRM" || action == "LEFT" || action == "RIGHT" )', self.distraction)

    def test_quickload_cancel_preserves_original_without_dirty_flag(self):
        body = self.handle.split("case ACTION_QUICKLOAD:", 1)[1].split("case ACTION_PL_INFO:", 1)[0]
        self.assertIn('if( a11y::enabled() &&', body)
        self.assertIn('Any progress not saved will be lost.', body)
        self.assertIn('break;', body)
        self.assertLess(body.index('query_yn('), body.index('quickload();'))
        self.assertLess(body.index('quickload();'), body.index('g->save_is_dirty = true;'))

    def test_view_actions_report_new_state(self):
        self.assertIn('_( "Pixel minimap enabled." )', self.game)
        self.assertIn('_( "Pixel minimap disabled." )', self.game)
        self.assertEqual(self.game.count('_( "World view zoom level: %d." )'), 2)

    def test_debug_confirmation_precedes_any_state_mutation(self):
        body = self.debug.split('void execute_action( debug_menu_index action )', 1)[1]
        self.assertLess(body.index('query_yn('), body.index('achievements.set_enabled( false )'))
        self.assertIn('Debug commands may permanently change', self.debug)
        self.assertIn('return std::nullopt;', self.debug)


if __name__ == "__main__":
    unittest.main()
