"""Protect the spoken nearby interaction list and pickup flows."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


class HubInteractionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.action = (SRC / "action.cpp").read_text("utf-8")
        cls.action_h = (SRC / "action.h").read_text("utf-8")
        cls.game = (SRC / "game.cpp").read_text("utf-8")
        cls.handle = (SRC / "handle_action.cpp").read_text("utf-8")
        cls.world = (SRC / "a11y_world.cpp").read_text("utf-8")
        cls.inventory = (SRC / "inventory_ui.cpp").read_text("utf-8")
        cls.menu = (SRC / "game_inventory.cpp").read_text("utf-8")

    def test_examine_lists_all_nine_nearby_positions(self):
        self.assertIn("choose_accessible_nearby_target(", self.action_h)
        chooser = self.action.split(
            "choose_accessible_nearby_target(", 1)[1]
        chooser = chooser.split("action_id handle_interact", 1)[0]
        self.assertIn("std::array<tripoint_rel_ms, 9>", chooser)
        self.assertIn("std::array<std::string, 9>", chooser)
        self.assertIn("menu.query();", chooser)
        self.assertIn("return allowed( chosen )", chooser)
        self.assertIn("a11y::observe_tile( target )", chooser)
        self.assertIn("observation_source::visible", chooser)
        self.assertNotIn("move( chosen )", chooser)
        examine = self.game.split(
            "void game::examine( bool with_pickup )", 1)[1]
        examine = examine.split("void game::get_fire_fuel", 1)[0]
        self.assertIn("if( a11y::enabled() )", examine)
        self.assertIn("choose_accessible_nearby_target(", examine)
        self.assertIn("can_examine_at( here, target, with_pickup )",
                      examine)

    def test_interact_lists_visible_tiles_but_uses_native_actions(self):
        self.assertIn(
            '_( "Interact with nearby objects" )', self.handle)
        self.assertIn("act = handle_interact( here, *mouse_target );",
                      self.handle)
        self.assertIn("open_tile_activity_actor( mouse_target )",
                      self.handle)
        self.assertIn("pos != get_avatar().pos_bub( here )",
                      self.action)

    def test_pickup_one_tile_versus_all_adjacent(self):
        self.assertIn("pick_s.add_map_items( target );", self.menu)
        self.assertIn("pick_s.add_nearby_items();", self.menu)
        self.assertIn("game_menus::inv::pickup( {where_.value()} )",
                      self.game)
        self.assertIn("game_menus::inv::pickup() );", self.game)
        self.assertIn("const bool pickup_mode", self.inventory)
        self.assertIn("location.describe( &owner.u )", self.inventory)
        self.assertIn("units::to_gram( location->weight() )",
                      self.inventory)
        self.assertIn("units::to_milliliter( location->volume() )",
                      self.inventory)
        self.assertIn("total_selected += row.entry->chosen_count",
                      self.inventory)

    def test_haul_and_butchery_report_actual_engine_state(self):
        self.assertIn('_( "Hauling: %1$s. Items: %2$d.', self.handle)
        self.assertIn("player.is_hauling()", self.handle)
        self.assertIn("viewer.is_autohauling()", self.world)
        self.assertIn("res.did_smash", self.handle)
        self.assertIn('_( "Destroyed %s." )', self.handle)
        self.assertIn("disassembly_stacks.size()", self.game)
        self.assertIn("salvage_stacks.size()", self.game)


if __name__ == "__main__":
    unittest.main()
