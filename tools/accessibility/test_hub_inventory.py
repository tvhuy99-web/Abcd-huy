"""Regression checks for the accessible inventory and item hub."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
ANDROID = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class InventoryHubTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.inv = (SRC / "inventory_ui.cpp").read_text("utf-8")
        cls.adv = (SRC / "advanced_inv.cpp").read_text("utf-8")
        cls.compare = (SRC / "game_inventory.cpp").read_text("utf-8")
        cls.game = (SRC / "game.cpp").read_text("utf-8")
        cls.world = (SRC / "a11y_world.cpp").read_text("utf-8")
        cls.act = (SRC / "item_action.cpp").read_text("utf-8")
        cls.handle = (SRC / "handle_action.cpp").read_text("utf-8")
        cls.hub = (ANDROID / "GameActionHub.java").read_text("utf-8")

    def test_inventory_details_use_real_item_state(self):
        self.assertIn("weight_to_string( location->weight() )", self.inv)
        self.assertIn("vol_to_string( location->volume() )", self.inv)
        self.assertIn("owner.u.is_wielding( *location )", self.inv)
        self.assertIn("owner.u.is_worn( *location )", self.inv)
        self.assertIn("location->is_favorite", self.inv)
        self.assertIn("location->invlet", self.inv)
        self.assertIn("node.disabled_reason", self.inv)
        self.assertIn("Container chain:", self.inv)

    def test_selection_totals_and_commit_separation(self):
        self.assertIn("Items marked for insertion", self.inv)
        self.assertIn("Items marked for dropping", self.inv)
        self.assertIn("total_selected += row.entry->chosen_count", self.inv)
        self.assertIn('node.actions = { "focus", "activate", "set_value"', self.inv)
        self.assertIn('a11y::selected_quantity( intent.value', self.inv)
        self.assertIn('select_count( *multi, binding->column, binding->location, *amount );', self.inv)
        self.assertIn('multi.toggle_entry( **target, count );', self.inv)
        self.assertIn('entry.get_available_count() == 1', self.inv)
        self.assertNotIn('intent.action == "apply"', self.inv)
        self.assertNotIn('"cancel_draft"', self.inv)

    def test_advanced_inventory_source_destination(self):
        self.assertIn("Source: %1$s. Destination: %2$s.", self.adv)
        self.assertIn("command.id == \"MOVE_VARIABLE_ITEM\"", self.adv)
        self.assertIn("return \"MOVE_VARIABLE_ITEM\"", self.adv)
        self.assertIn("if( exit && a11y::enabled() )", self.adv)
        self.assertIn("Transfer requested:", self.adv)
        self.assertIn("if( !query_destination( destarea ) )", self.adv)
        self.assertIn("if( !query_charges( destarea,", self.adv)

    def test_comparison_uses_real_two_item_documents(self):
        self.assertIn('#include "a11y_document.h"', self.compare)
        self.assertIn('a11y::screen_scope reader( &ctxt, "item-compare"', self.compare)
        self.assertIn('format_item_info( first_info, second_info )', self.compare)
        self.assertIn('format_item_info( second_info, first_info )', self.compare)
        self.assertIn('snapshot.focus = "compare:first"', self.compare)

    def test_drop_and_container_use_original_engine(self):
        self.assertIn("game_menus::inv::multidrop( u )", self.game)
        self.assertIn("u.drop( chosen, pnt )", self.game)
        self.assertIn("u.drop( chosen, *pnt )", self.game)
        self.assertIn("game_menus::inv::insert_items( item_loc );", self.game)
        self.assertIn("Transfer requested:", self.adv)
        self.assertIn("Selected container:", self.game)
        self.assertIn("Favorite items and liquids remain", self.game)

    def test_action_menu_and_shortcut_feedback(self):
        self.assertIn("kmenu.desc_enabled = true;", self.act)
        self.assertIn("Using %1$s: %2$s.", self.act)
        self.assertIn("Using held item: %s.", self.handle)
        self.assertIn("Inventory shortcut cleared for %s.", self.compare)
        self.assertIn("Inventory shortcut for %1$s: %2$c.", self.compare)
        self.assertIn('node.id == "item_action_menu"', self.world)
        self.assertIn('"insert", "unload_container", "listitems" }', self.hub)


if __name__ == "__main__":
    unittest.main()
