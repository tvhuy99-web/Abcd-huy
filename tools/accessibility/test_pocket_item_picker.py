"""Accessibility contracts for long-press inventory pocket selection.

The picker is a local reader overlay: browsing and toggling drafts do not
mutate the native pickup selection until the validated pocket commit.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = (ROOT / "src/inventory_ui.cpp").read_text(encoding="utf-8")
CTRL = (ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/"
        "GameReaderController.java").read_text(encoding="utf-8")


class AccessiblePocketPickerContracts(unittest.TestCase):
    def test_double_tap_item_still_selects_not_opens_pocket(self):
        activate = CTRL.split("public void activate()", 1)[1].split(
            "private void openEditor(", 1
        )[0]
        self.assertIn('final boolean actionableItem = node.id.startsWith("item:")', activate)
        self.assertIn("if (node.hasChildren && !actionableItem)", activate)
        self.assertIn("if (pocketStage != 0)", activate)
        self.assertLess(activate.index("if (pocketStage != 0)"),
                        activate.index("final boolean actionableItem"))

    def test_long_press_explicitly_offers_open_pocket_and_other_commands(self):
        method = CTRL.split("public void openContextActions()", 1)[1].split(
            "private void closeContextActions()", 1
        )[0]
        self.assertIn('"inventory".equals(snapshot.category)', method)
        self.assertIn('focused.id.startsWith("item:")', method)
        self.assertIn("!pocketNodes(focused.id).isEmpty()", method)
        self.assertIn("pocketStage = 1;", method)
        self.assertIn("openGlobalContextActions();", method)
        self.assertIn("R.string.a11yPocketOpen", CTRL)
        self.assertIn("R.string.a11yPocketOtherActions", CTRL)

    def test_one_pocket_skips_chooser_and_empty_pocket_speaks(self):
        choices = CTRL.split("private void openPocketChoices()", 1)[1].split(
            "private void selectPocketAmount(", 1
        )[0]
        self.assertIn("if (pockets.size() == 1)", choices)
        self.assertIn("openPocketContents(pockets.get(0));", choices)
        self.assertIn("pocketStage = 2;", choices)
        self.assertIn("if (items.isEmpty())", choices)
        self.assertIn("Integer.parseInt(pocket.appliedValue)", choices)
        self.assertIn("R.string.a11yPocketEmpty", choices)
        self.assertIn("R.string.a11yPocketNotSelectable", choices)
        self.assertIn("R.string.a11yPocketSealed", choices)
        self.assertNotIn('pocket.detail.contains("Contains ")', choices)

    def test_single_row_auto_returns_and_stacks_keep_quantity_picker(self):
        select = CTRL.split("private void selectPocketAmount(", 1)[1].split(
            "private void commitPocketChoices(", 1
        )[0]
        self.assertIn("if (pocketItems().size() == 1)", select)
        self.assertIn("commitPocketChoices();", select)
        self.assertIn("new ReaderTextEditor(activity", select)
        self.assertIn("true, true, 0.0, item.fieldMax, 1.0, true", select)
        self.assertIn("if (item.fieldMax > 1)", CTRL)
        self.assertIn("openPocketQuantityEditor(item);", CTRL)

    def test_many_items_staged_then_confirm_or_cancel(self):
        self.assertIn("private final LinkedHashMap<String, Integer> pocketDrafts", CTRL)
        self.assertIn("private void commitPocketChoices()", CTRL)
        self.assertIn('long request = send("set_value", payload.toString());', CTRL)
        self.assertIn("if (pocketStage == 3 && pocketItems().size() > 1)", CTRL)
        self.assertIn("commitPocketChoices();", CTRL)
        self.assertIn("resetPocketPicker(true); // Back has the same rollback as Cancel.", CTRL)
        self.assertIn("pocketDrafts.clear();", CTRL)
        # No game changes take place during local draft edits or cancellation.
        draft = CTRL.split("private void selectPocketAmount(", 1)[1].split(
            "private void commitPocketChoices()", 1
        )[0]
        self.assertNotIn('send("set_value"', draft)
        self.assertNotIn('send("activate"', draft)

    def test_confirm_and_cancel_are_reachable_by_swiping(self):
        self.assertIn('private static final String POCKET_CONFIRM_NODE', CTRL)
        self.assertIn('private static final String POCKET_CANCEL_NODE', CTRL)
        navigation = CTRL.split("private void navigatePocketPicker(", 1)[1].split(
            "private void renderPocketPicker()", 1
        )[0]
        self.assertIn("choices.size() + (pocketStage == 3 ? 2 : 0)", navigation)
        self.assertIn("POCKET_CONFIRM_NODE", navigation)
        self.assertIn("POCKET_CANCEL_NODE", navigation)
        activate = CTRL.split("private void activatePocketPicker()", 1)[1].split(
            "private void navigatePocketPicker(", 1
        )[0]
        self.assertIn("commitPocketChoices();", activate)
        self.assertIn("resetPocketPicker(true);", activate)

    def test_native_pockets_validate_live_items_and_preserve_sealed_rules(self):
        self.assertIn('pocket_node.id = "pocket:" + rows[i].id', SRC)
        self.assertIn('contained.id = "pocket-entry:" + pocket_binding.id', SRC)
        self.assertIn("pocket->sealed()", SRC)
        self.assertIn('pocket_node.actions = { "focus", "set_value" };', SRC)
        self.assertIn("pocket_node.applied_value = std::to_string( pocket->size() );", SRC)
        self.assertIn("all_inside && entry->is_selectable()", SRC)
        self.assertIn("std::set<std::string> unique_choices;", SRC)
        self.assertIn("a11y::selected_quantity( instruction.substr( equals + 1 )", SRC)
        self.assertIn("loc.parent_item() != pb->container", SRC)
        self.assertLess(
            SRC.index("// All changes have been checked against the live pocket"),
            SRC.index("select_count( *multi, change.choice->column"))

    def test_parent_child_selection_is_not_double_counted(self):
        helper = SRC.split("bool select_count(", 1)[1].split(
            "std::optional<std::string> handle(", 1
        )[0]
        self.assertIn("is_ancestor( picked, other ) || is_ancestor( other, picked )",
                      helper)
        self.assertIn("multi.toggle_entry( **found, 0 );", helper)
        self.assertIn("multi.toggle_entry( **target, count );", helper)
        self.assertLess(helper.index("multi.toggle_entry( **found, 0 );"),
                        helper.index("multi.toggle_entry( **target, count );"))

    def test_pocket_locale_strings_exist(self):
        en = (ROOT / "android/app/src/main/res/values/a11y_editor.xml").read_text("utf-8")
        vi = (ROOT / "android/app/src/main/res/values-vi/a11y_editor.xml").read_text("utf-8")
        for name in ("a11yPocketOpen", "a11yPocketOtherActions", "a11yPocketEmpty",
                     "a11yPocketNotSelectable", "a11yPocketConfirm", "a11yPocketCancel",
                     "a11yPocketRow", "a11yPocketSelectionError"):
            self.assertIn('name="' + name + '"', en)
            self.assertIn('name="' + name + '"', vi)


if __name__ == "__main__":
    unittest.main()
