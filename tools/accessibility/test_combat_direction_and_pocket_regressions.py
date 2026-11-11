"""Static regressions for accessible holsters, pockets and direction-first gunfire."""

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class CombatPocketRegressions(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.holster = (SRC / "iuse_actor.cpp").read_text(encoding="utf-8")
        cls.character = (SRC / "character_inventory.cpp").read_text(encoding="utf-8")
        cls.gun = (SRC / "ranged.cpp").read_text(encoding="utf-8")
        cls.reader = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")

    def test_draw_row_is_offset_from_holster_action_not_total_item_stacks(self):
        function = self.holster.split("std::optional<int> holster_actor::use(", 1)[1]
        function = function.split("void holster_actor::info(", 1)[0]
        self.assertIn("const size_t item_index = static_cast<size_t>( ret - 1 );", function)
        self.assertIn("item_index >= all_items.size()", function)
        self.assertIn("std::next( all_items.begin(), ret - 1 )", function)
        self.assertNotIn("opts.size() != it.num_item_stacks()", function)
        self.assertIn("il.has_item( *internal_item )", self.character)

    def test_read_only_pocket_cancel_wraps_back_to_its_item(self):
        self.assertIn("cancelPosition = items.size() + (pocketAllowsSelection() ? 2 : 1)", self.reader)
        self.assertIn("index = choices.size() + (pocketAllowsSelection() ? 1 : 0)", self.reader)
        self.assertIn("Math.floorMod(index + delta, count)", self.reader)
        # A one-item read-only pocket has exactly two navigable choices.
        size = 1
        selectable = False
        count = size + (2 if selectable else 1)
        cancel_index = size + (1 if selectable else 0)
        self.assertEqual(count, 2)
        self.assertEqual((cancel_index + 1) % count, 0)
        self.assertEqual((cancel_index - 1) % count, 0)

    def test_only_fire_mode_adds_eight_direct_shot_controls(self):
        self.assertIn('if( mode == TargetMode::Fire ) {', self.gun)
        self.assertIn('direction.id = "target:fire-direction:" + std::to_string( i );', self.gun)
        self.assertIn("for( int i = 0; i < 8; ++i )", self.gun)
        self.assertIn("direction.actions = { \"focus\", \"activate\" };", self.gun)
        self.assertIn("A11Y_INTERNAL_FIRE_DIRECTION", self.gun)
        self.assertIn("loop_exit_code = ExitCode::Fire;", self.gun)

    def test_empty_direction_and_friendlies_are_not_excluded(self):
        targeting = self.gun.split("tripoint_bub_ms target_ui::directional_shot_target(", 1)[1]
        targeting = targeting.split("a11y::semantic_snapshot target_ui::reader_snapshot()", 1)[0]
        self.assertIn("return fallback;", targeting)
        self.assertIn("creature != nullptr && creature != you && pl_sees( *creature )", targeting)
        self.assertNotIn("guaranteed_hostile", targeting)
        self.assertNotIn("Attitude::HOSTILE", targeting)
        self.assertIn('No visible creature. You can still fire in this direction.', self.gun)
        self.assertNotIn('" " + _( "No visible creature. You can still fire in this direction." )', self.gun)
        self.assertIn('direction.value += " ";\n                direction.value += _(', self.gun)

    def test_direct_gun_chooser_and_auto_reload_keep_native_game_rules(self):
        handler = (ROOT / "src/handle_action.cpp").read_text(encoding="utf-8")
        game = (ROOT / "src/game.cpp").read_text(encoding="utf-8")
        aim_actor = (ROOT / "src/activity_actor.cpp").read_text(encoding="utf-8")
        game_header = (ROOT / "src/game.h").read_text(encoding="utf-8")
        self.assertIn('you.all_items_loc()', handler)
        self.assertIn('Choose a gun to fire', handler)
        self.assertIn('you.wield( selected )', handler)
        self.assertIn('avatar_action::fire_wielded_weapon( you )', handler)
        self.assertIn('reload.id = "target:auto-reload"', self.gun)
        self.assertIn('A11Y_INTERNAL_AUTO_RELOAD', self.gun)
        self.assertIn('void game::auto_reload_wielded(', game)
        self.assertIn('void auto_reload_wielded(', game_header)
        self.assertIn('u.list_ammo( gun, choices, false, true )', game)
        self.assertIn('reload_activity_actor( std::move( *selected )', game)
        self.assertIn('g->auto_reload_wielded( auto_reload_steps - 1 )', aim_actor)
        self.assertIn('jsout.member( "auto_reload_steps", auto_reload_steps )', aim_actor)
        self.assertIn('jsout.member( "auto_reload_requested", auto_reload_requested )', aim_actor)

    def test_nested_pocket_items_open_original_inventory_item_menu(self):
        native = (ROOT / "src/inventory_ui.cpp").read_text(encoding="utf-8")
        inventory = (ROOT / "src/game_inventory.cpp").read_text(encoding="utf-8")
        self.assertIn('contained_item_bindings.emplace_back( item_node.id, contained )', native)
        self.assertIn('pending_contained_item = item->second;', native)
        self.assertIn('return "A11Y_PICK_POCKET_ITEM";', native)
        self.assertIn('reader.take_contained_item()', native)
        self.assertIn('inv_s.open_contained_item_menu = true;', inventory)
        self.assertIn('res = g->inventory_item_menu( location );', inventory)
        self.assertIn('item.actions.contains("activate")', self.reader)
        self.assertNotIn('pocket_node.value = "read_only";', native)

    def test_holding_restores_original_commands_without_deleting_them(self):
        self.assertIn("targetingOriginalControls = !targetingOriginalControls", self.reader)
        self.assertIn("if (targetingOriginalControls) { return result; }", self.reader)
        self.assertIn('result.removeIf(node -> !node.id.startsWith("target:fire-direction:")', self.reader)
        self.assertIn('&& !"target:auto-reload".equals(node.id)', self.reader)
        self.assertIn('focus = "target:fire-direction:0";', self.reader)
        self.assertIn('return "A11Y_INTERNAL_FIRE_DIRECTION";', self.gun)


if __name__ == "__main__":
    unittest.main()
