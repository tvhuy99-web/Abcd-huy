"""Flat inventory browsing: pocket contents are swiped through in sequence.

In the ordinary inventory list, one-finger LEFT/RIGHT (and UP/DOWN) steps
through top-level items; when the sequence reaches a container, the same
swipes keep stepping through the items inside its pockets, flattened one
by one, then on to the next item. There is no pocket-hierarchy UI, no
"open pocket" step, and no separate pocket layer: container, then its
contents, then the next item.

The native snapshot already publishes those contents as "pocket-entry:"
rows under each "pocket:" status row (nested containers flattened with an
ancestor path in the label); the reader only had to include them in the
top-level swipe list. The staged long-press pocket picker used by pickup
quantity selection is untouched: its drafts are still staged and committed
together on Confirm. Pickup checkbox rows join the flat browse list only
when their parent "pocket:" row accepts the atomic set_value commit (see
test_pickup_flat_pocket_browsing.py); any other checkbox row stays out.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CTRL = (ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/"
        "GameReaderController.java").read_text(encoding="utf-8")
NATIVE = (ROOT / "src/inventory_ui.cpp").read_text(encoding="utf-8")


class FlatPocketBrowsingContracts(unittest.TestCase):
    def test_swipe_list_interleaves_pocket_contents_after_their_container(self):
        visible = CTRL.split("private List<ReaderSnapshot.Node> visibleNodes()", 1)[1].split(
            "private void openContextActions()", 1
        )[0]
        self.assertIn("if (flattenedInventoryBrowse()) {", visible)
        self.assertIn("flattened.add(node);", visible)
        self.assertIn("flattened.addAll(flattenedPocketContents(node.id));", visible)
        # The flattening runs before the mixed-screen command filtering,
        # so pocket contents count as content, not as hidden commands.
        self.assertLess(visible.index("if (flattenedInventoryBrowse()) {"),
                        visible.index("boolean containsContent = false;"))

    def test_flattening_covers_all_pockets_and_nested_containers(self):
        helper = CTRL.split(
            "private List<ReaderSnapshot.Node> flattenedPocketContents(", 1)[1].split(
            "private List<ReaderSnapshot.Node> visibleNodes()", 1
        )[0]
        self.assertIn('node.id.startsWith("pocket-entry:")', helper)
        # The walk climbs the parent chain, so contents of nested
        # containers (pocket inside a pocket-entry item) flatten too.
        self.assertIn("while (!cursor.parentId.isEmpty()) {", helper)
        self.assertIn("cursor = snapshot.find(cursor.parentId);", helper)
        self.assertIn("if (cursor.parentId.equals(itemId)) { contents.add(node); break; }",
                      helper)

    def test_pickup_checkbox_rows_stay_in_the_staged_picker(self):
        # A pickup checkbox row joins the flat list only when its parent
        # pocket accepts the atomic set_value commit (see
        # test_pickup_flat_pocket_browsing.py); the blanket exclusion
        # below is now qualified by pickupPocketParent() instead.
        self.assertIn('"checkbox".equals(node.role)', CTRL.split(
            "private List<ReaderSnapshot.Node> flattenedPocketContents(", 1)[1].split(
            "private List<ReaderSnapshot.Node> visibleNodes()", 1
        )[0])
        self.assertIn('"checkbox".equals(node.role)', CTRL.split(
            "private boolean isFlattenedPocketEntry(", 1)[1].split(
            "private List<ReaderSnapshot.Node> flattenedPocketContents(", 1
        )[0])
        # The picker itself still owns swipes while it is open.
        navigate = CTRL.split("public void navigate(int delta)", 1)[1].split(
            "public boolean adjustFocusedSetting(", 1
        )[0]
        self.assertIn("if (pocketStage != 0) {", navigate)
        self.assertIn("navigatePocketPicker(delta);", navigate)

    def test_flat_browse_only_at_top_level_with_picker_closed(self):
        guard = CTRL.split("private boolean flattenedInventoryBrowse()", 1)[1].split(
            "private boolean isFlattenedPocketEntry(", 1
        )[0]
        self.assertIn('"inventory".equals(snapshot.category)', guard)
        self.assertIn("pocketStage == 0", guard)
        self.assertIn("readerLayerParent.isEmpty()", guard)
        self.assertIn("!commandPaletteOpen", guard)

    def test_swiping_onto_pocket_content_does_not_enter_a_pocket_layer(self):
        self.assertIn("if (!isFlattenedPocketEntry(currentFocus)) {", CTRL)
        self.assertIn("readerLayerParent = currentFocus.parentId;", CTRL)
        guard = CTRL.split("private boolean isFlattenedPocketEntry(", 1)[1].split(
            "private List<ReaderSnapshot.Node> flattenedPocketContents(", 1
        )[0]
        self.assertIn('node.id.startsWith("pocket-entry:")', guard)
        self.assertIn("flattenedInventoryBrowse()", guard)

    def test_spoken_position_counts_the_flat_swipe_list(self):
        speech = CTRL.split("int spokenPosition = node.position;", 1)[1].split(
            "private static String formatRawNumber(", 1
        )[0]
        self.assertIn("boolean flattenedInventoryPositions = flattenedInventoryBrowse();",
                      speech)
        self.assertIn("(spokenPosition <= 0 || spokenTotal <= 0 || flattenedInventoryPositions)",
                      speech)
        # The recompute still reads the position from the swipe list.
        self.assertIn("List<ReaderSnapshot.Node> siblings = visibleNodes();", speech)

    def test_native_snapshot_publishes_flattened_pocket_contents(self):
        # Ordinary inventory (not pickup) exposes real pocket rows with
        # nested containers flattened and the path in the spoken label.
        self.assertIn("Flatten nested containers with their path in the", NATIVE)
        self.assertIn('item_node.id = "pocket-entry:" + pocket_node.id + ":"', NATIVE)
        self.assertIn('item_node.label = ancestors + " > " + item_node.label;', NATIVE)
        self.assertIn("contained_item_bindings.emplace_back( item_node.id, contained )",
                      NATIVE)
        # Double-tap on such a row already opens the contained item's
        # original action menu; browsing only had to reach the row.
        self.assertIn('return "A11Y_PICK_POCKET_ITEM";', NATIVE)


if __name__ == "__main__":
    unittest.main()
