"""Regression checks for the accessible equipment-and-repairs hub."""
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "src"


class EquipmentHubTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.avatar = (SRC / "avatar_action.cpp").read_text("utf-8")
        cls.character = (SRC / "character.cpp").read_text("utf-8")
        cls.armor = (SRC / "armor_layers.cpp").read_text("utf-8")
        cls.game = (SRC / "game.cpp").read_text("utf-8")
        cls.activity = (SRC / "activity_actor.cpp").read_text("utf-8")

    def test_reader_mending_lists_carried_and_worn_repairable_items(self):
        mend = self.avatar.split("void avatar_action::mend(", 1)[1].split(
            "bool avatar_action::eat_here(", 1)[0]
        self.assertIn("if( a11y::enabled() )", mend)
        self.assertIn("g->inv_map_splice(", mend)
        self.assertIn("you.has_item( *candidate )", mend)
        self.assertIn("candidate->faults", mend)
        self.assertIn("fault->get_fixes()", mend)
        self.assertIn("you.mend_item( item_location( loc ) )", mend)
        self.assertIn("Status: ready to mend.", self.character)
        self.assertIn("Status: cannot mend yet;", self.character)
        self.assertIn("Fault: %s.", self.character)

    def test_reload_identifies_actual_selected_ammunition_and_completion(self):
        reload = self.game.split("void game::reload( item_location &loc", 1)[1].split(
            "class reload_selector_preset", 1)[0]
        self.assertIn("opt.ammo->tname()", reload)
        self.assertIn("Preparing to reload", reload)
        self.assertIn("reload_activity_actor( std::move( opt ), extra_moves )", reload)
        self.assertIn("else if( finished && a11y::enabled() )", self.activity)
        self.assertIn("Reload complete:", self.activity)

    def test_armor_reader_announces_real_reordering_only(self):
        self.assertIn("const bool changed_layer = selected != leftListIndex;", self.armor)
        self.assertIn("worn.splice( to, worn, tmp_worn[selected] );", self.armor)
        self.assertIn("if( changed_layer && a11y::enabled() )", self.armor)
        self.assertIn("Armor layer changed:", self.armor)

    def test_martial_arts_distinguishes_active_from_focused(self):
        style = self.character.split(
            "bool character_martial_arts::pick_style(", 1
        )[1]
        self.assertIn("kmenu.entries.back().a11y_value = selected ?", style)
        self.assertIn("Currently active fighting style.", style)
        self.assertIn("Not currently active.", style)
        self.assertIn("Active fighting style: %s.", style)
        self.assertIn("Keep hands free enabled.", style)


if __name__ == "__main__":
    unittest.main()
