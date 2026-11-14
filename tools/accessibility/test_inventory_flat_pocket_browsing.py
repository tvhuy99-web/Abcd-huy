"""Regression contracts: two-axis navigation of inventory pocket contents.

The historical filename mentions flat browsing, but the correct behavior is
horizontal top-level item browsing and vertical contained-item browsing.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
CTRL = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
SURFACE = (JAVA / "ReaderGestureSurface.java").read_text(encoding="utf-8")
NATIVE = (ROOT / "src/inventory_ui.cpp").read_text(encoding="utf-8")


class TwoAxisInventoryTests(unittest.TestCase):
    def test_swipes_are_dispatched_before_default_browsing(self):
        call = "listener.handleInventoryAxisSwipe(horizontal, vertical)"
        self.assertIn(call, SURFACE)
        self.assertLess(SURFACE.index(call),
                        SURFACE.index("listener.handleCraftingRecipeSwipe(horizontal)"))
        self.assertIn("public boolean handleInventoryAxisSwipe(", CTRL)

    def test_horizontal_browsing_never_interleaves_pocket_contents(self):
        visible = CTRL.split(
            "private List<ReaderSnapshot.Node> visibleNodes()", 1
        )[1].split("public void openContextActions()", 1)[0]
        self.assertIn("readerLayerParent.equals(node.parentId)", visible)
        self.assertNotIn("flattened.addAll(", visible)
        self.assertNotIn("flattenedPocketContents(node.id)", visible)
        self.assertIn("List<ReaderSnapshot.Node> roots = visibleNodes();", CTRL)
        self.assertIn("focus = roots.get(current < 0 ? 0", CTRL)

    def test_vertical_browses_only_contents_of_focused_container(self):
        self.assertIn("inventoryRootItem(snapshot.find(focus))", CTRL)
        self.assertIn("flattenedPocketContents(root.id)", CTRL)
        self.assertIn("focus = contents.get(next).id;", CTRL)
        self.assertIn("if (contents.isEmpty()) { readCurrent(); return true; }",
                      CTRL)
        self.assertIn("Math.floorMod(current + vertical, contents.size())", CTRL)

    def test_nested_pockets_are_reached_on_vertical_axis(self):
        helper = CTRL.split(
            "private List<ReaderSnapshot.Node> flattenedPocketContents(", 1
        )[1].split("private ReaderSnapshot.Node inventoryRootItem(", 1)[0]
        self.assertIn('node.id.startsWith("pocket-entry:")', helper)
        self.assertIn("cursor = snapshot.find(cursor.parentId);", helper)
        self.assertIn("if (cursor.parentId.equals(itemId))", helper)
        self.assertIn('item_node.id = "pocket-entry:"', NATIVE)

    def test_vertical_focus_does_not_enter_a_nested_layer(self):
        self.assertIn("if (!isFlattenedPocketEntry(currentFocus))", CTRL)
        self.assertIn("readerLayerParent = currentFocus.parentId;", CTRL)

    def test_positions_are_scoped_to_the_active_axis(self):
        body = CTRL.split("int spokenPosition = node.position;", 1)[1]
        body = body.split("private static String formatRawNumber(", 1)[0]
        self.assertIn("isFlattenedPocketEntry(node)", body)
        self.assertIn("spokenTotal = contents.size();", body)
        self.assertIn("List<ReaderSnapshot.Node> siblings = visibleNodes();",
                      body)


if __name__ == "__main__":
    unittest.main()
