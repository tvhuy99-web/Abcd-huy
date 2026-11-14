"""Regression contracts: pickup and ordinary inventory share two axes.

One-finger double-tap toggles a single contained item. Stacks open the
existing quantity editor. Only final pickup confirmation moves items.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
CTRL = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
SURFACE = (JAVA / "ReaderGestureSurface.java").read_text(encoding="utf-8")
NATIVE = (ROOT / "src/inventory_ui.cpp").read_text(encoding="utf-8")
HANDLE = (ROOT / "src/handle_action.cpp").read_text(encoding="utf-8")


class PickupTwoAxisTests(unittest.TestCase):
    def test_both_inventory_and_pickup_use_shared_axis_handler(self):
        self.assertIn('"inventory".equals(snapshot.category)', CTRL)
        self.assertIn("handleInventoryAxisSwipe(int horizontal, int vertical)",
                      CTRL)
        self.assertIn("listener.handleInventoryAxisSwipe(horizontal, vertical)",
                      SURFACE)
        self.assertIn("flattenedPocketContents(root.id)", CTRL)

    def test_direct_pickup_only_uses_selectable_parent_pocket(self):
        self.assertIn('parent.id.startsWith("pocket:")', CTRL)
        self.assertIn('parent.actions.contains("set_value")', CTRL)
        self.assertIn("isFlatPickupPocketEntry(node)", CTRL)
        self.assertIn("activateFlatPickupPocketEntry(node);", CTRL)

    def test_single_item_toggle_and_stack_editor(self):
        self.assertIn("if (item.fieldMax > 1) {", CTRL)
        self.assertIn("openFlatPickupQuantityEditor(item);", CTRL)
        self.assertIn("pocketAmount(item) == 0 ? 1 : 0", CTRL)
        self.assertIn('send("set_value", item.id + "=" + amount);', CTRL)
        self.assertIn("new ReaderTextEditor(activity", CTRL)

    def test_native_parent_commit_keeps_final_pickup_separate(self):
        self.assertIn('contained.actions = { "focus", "read_detail" };',
                      NATIVE)
        self.assertIn('pocket_node.actions = { "focus", "set_value" };',
                      NATIVE)
        self.assertIn("select_count( *multi, change.choice->column,", NATIVE)
        self.assertIn('intent.node == "pickup-here:confirm"', HANDLE)

    def test_original_multi_item_picker_remains_available(self):
        self.assertIn("navigatePocketPicker(delta);", CTRL)
        self.assertIn("commitPocketChoices();", CTRL)
        self.assertIn("if (pocketStage != 0) {", CTRL)


if __name__ == "__main__":
    unittest.main()
