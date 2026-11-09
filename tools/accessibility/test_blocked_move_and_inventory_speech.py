"""Check speech for blocked walking and item action menus."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
PO = ROOT / "lang/po/accessibility/vi.po"


class BlockedMoveAndInventorySpeechTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.outcome = (SRC / "a11y_outcome.cpp").read_text("utf-8")
        cls.movement = (SRC / "avatar_action.cpp").read_text("utf-8")
        cls.game = (SRC / "game.cpp").read_text("utf-8")
        cls.uilist = (SRC / "uilist.cpp").read_text("utf-8")
        cls.po = PO.read_text("utf-8")

    def test_blocked_steps_speak_without_position_delta(self):
        phrase = "android::announce( describe_action_outcome("
        self.assertIn(phrase, self.outcome)
        self.assertIn('last_outcome ), 3 );', self.outcome)
        self.assertIn('android::cue( "blocked" );', self.outcome)
        self.assertIn('if( kind == action_outcome_kind::blocked ||',
                      self.outcome)
        self.assertIn('case action_outcome_kind::blocked:', self.outcome)
        self.assertIn('_( "%s. Cannot move." )', self.outcome)
        self.assertIn('msgstr "%s, không thể di chuyển."', self.po)

    def test_movement_names_visible_blocker(self):
        self.assertIn('blocker = m.obstacle_name( dest_loc );',
                      self.movement)
        self.assertIn('if( you.sees( here, *creature ) )',
                      self.movement)
        self.assertIn('a11y::action_outcome_kind::blocked, blocker );',
                      self.movement)
        self.assertIn('_( "your condition" )', self.movement)
        self.assertIn('critter.disp_name() );', self.movement)

    def test_item_menu_keeps_reader_scope_across_swipes(self):
        self.assertIn('action_menu.title = string_format(', self.game)
        self.assertIn('_( "Actions for %s" )', self.game)
        self.assertIn('ui_impl = action_menu.query( a11y::enabled() );',
                      self.game)
        self.assertIn('auto reader = create_reader_scope( ctxt );',
                      self.uilist)
        self.assertIn('} while( loop && ret == UILIST_WAIT_INPUT );',
                      self.uilist)
        self.assertIn('msgstr "Thao tác với %s"', self.po)


if __name__ == "__main__":
    unittest.main()
