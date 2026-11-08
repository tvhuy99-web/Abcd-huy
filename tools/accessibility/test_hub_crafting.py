"""Regression checks for a fast accessible crafting and building hub."""

import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "src"


class CraftingHubTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.craft = (SRC / "crafting.cpp").read_text("utf-8")
        cls.menu = (SRC / "crafting_gui.cpp").read_text("utf-8")
        cls.construction = (SRC / "construction.cpp").read_text("utf-8")
        cls.inventory = (SRC / "inventory_ui.cpp").read_text("utf-8")
        cls.zones = (SRC / "zone_manager_ui.cpp").read_text("utf-8")

    def test_recraft_reuses_the_last_recipe_without_extra_confirmation(self):
        body = self.craft.split("void Character::recraft(", 1)[1].split(
            "void Character::long_craft(", 1
        )[0]
        self.assertIn("No previous recipe to repeat.", body)
        self.assertIn("lastrecipe->result_name()", body)
        self.assertIn("last_craft->execute( loc );", body)
        self.assertNotIn("query_yn(", body)

    def test_missing_craft_requirements_are_spoken(self):
        body = self.craft.split(
            "bool Character::making_would_work(", 1
        )[1].split("int Character::available_assistant_count(", 1)[0]
        self.assertIn("simple_requirements().list_missing()", body)
        self.assertIn("if( a11y::enabled() )", body)
        self.assertIn("add_msg_if_player( m_info", body)
        self.assertIn("popup( buffer, PF_NONE );", body)

    def test_continuous_craft_stays_one_action(self):
        body = self.craft.split(
            "void Character::long_craft(", 1
        )[1].split("bool Character::making_would_work(", 1)[0]
        self.assertIn("Continuous crafting:", body)
        self.assertIn("make_all_craft(", body)
        self.assertNotIn("query_yn(", body)

    def test_recipe_row_is_short_with_details_on_demand(self):
        snapshot = self.menu.split(
            "a11y::semantic_snapshot crafting_ui_impl::reader_snapshot(", 1
        )[1].split("crafting_ui_impl::reader_intent(", 1)[0]
        self.assertIn("Estimated time:", snapshot)
        self.assertIn("node.selected && !rec.is_nested()", snapshot)
        self.assertIn("craft:requirements", snapshot)
        self.assertIn("craft:conditions", snapshot)

    def test_construction_directions_are_prevalidated(self):
        body = self.construction.split(
            "void place_construction(", 1
        )[1].split("void complete_construction(", 1)[0]
        self.assertIn("if( a11y::enabled() )", body)
        self.assertIn("for( const auto &entry : valid )", body)
        self.assertIn("directions.query();", body)
        self.assertIn("pnt_ = valid_positions[directions.ret];", body)
        self.assertIn("place_construction", self.construction)
        self.assertIn("choose_adjacent(", body)

    def test_disassemble_recovery_is_labeled(self):
        self.assertIn("Potential recovered materials:", self.inventory)
        self.assertIn("Disassembly time:", self.inventory)
        self.assertIn('owner.title == _( "Disassemble item" )',
                      self.inventory)
        self.assertIn("You recover %1$d %2$s.", self.craft)
        self.assertIn("You fail to recover %1$d %2$s.", self.craft)

    def test_zone_default_name_skips_only_the_name_prompt(self):
        self.assertEqual(self.zones.count(
            "std::optional<std::string>( default_name ) : "
            "mgr.query_name( default_name )"), 2)
        self.assertIn("Zone created:", self.zones)
        self.assertIn("query_position();", self.zones)
        self.assertIn("query_personal_position();", self.zones)
        self.assertIn('query_ynq( _( "Save changes?" ) )', self.zones)


if __name__ == "__main__":
    unittest.main()
