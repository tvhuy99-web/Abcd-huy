"""Flat pocket browsing in the PICKUP interface.

The inventory pickup list (category "inventory", pickup selector with a
quantity form) shows containers whose contents are published as
"pocket-entry:" checkbox rows under each "pocket:" status row. Those rows
are the staged picker's draft rows: on their own they accept only focus
and read_detail, and a set_value sent to one of them is refused by the
native side. Flat browsing still works, because the reader never sends
a set_value to a pocket-entry row: it includes the row in the swipe list
only when its parent "pocket:" row accepts the atomic set_value commit,
and double-tap selects the row by committing a single-entry payload
("<entry id>=<amount>") to that parent, exactly as the staged picker
commits its drafts on Confirm -- except a flat row commits immediately,
like a top-level pickup row, instead of staging a draft.

The other pickup screen, "pickup-here" (ground items in the tile the
player stands on), publishes no pockets at all: ground containers are
single rows picked up whole, so there is nothing to flatten there.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CTRL = (ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/"
        "GameReaderController.java").read_text(encoding="utf-8")
NATIVE = (ROOT / "src/inventory_ui.cpp").read_text(encoding="utf-8")
HANDLE = (ROOT / "src/handle_action.cpp").read_text(encoding="utf-8")


class PickupFlatPocketBrowsingContracts(unittest.TestCase):
    def test_pickup_checkbox_rows_join_the_flat_list_when_the_parent_accepts_commit(self):
        helper = CTRL.split(
            "private List<ReaderSnapshot.Node> flattenedPocketContents(", 1)[1].split(
            "private List<ReaderSnapshot.Node> visibleNodes()", 1
        )[0]
        # No blanket checkbox exclusion any more: a checkbox row is
        # skipped only when its parent pocket cannot take the commit.
        self.assertIn('"checkbox".equals(node.role) && pickupPocketParent(node) == null',
                      helper)
        self.assertNotIn('!node.id.startsWith("pocket-entry:") || "checkbox".equals(node.role)',
                         helper)
        parent = CTRL.split(
            "private ReaderSnapshot.Node pickupPocketParent(", 1)[1].split(
            "private boolean isFlatPickupPocketEntry(", 1
        )[0]
        self.assertIn('node.id.startsWith("pocket-entry:")', parent)
        self.assertIn('parent.id.startsWith("pocket:")', parent)
        self.assertIn('parent.actions.contains("set_value")', parent)

    def test_flat_pickup_rows_do_not_enter_a_pocket_layer(self):
        guard = CTRL.split("private boolean isFlattenedPocketEntry(", 1)[1].split(
            "private ReaderSnapshot.Node pickupPocketParent(", 1
        )[0]
        self.assertIn('node.id.startsWith("pocket-entry:")', guard)
        self.assertIn('|| pickupPocketParent(node) != null', guard)
        self.assertIn("flattenedInventoryBrowse()", guard)
        self.assertIn("if (!isFlattenedPocketEntry(currentFocus)) {", CTRL)

    def test_double_tap_selects_a_flat_pickup_row_through_the_parent_pocket(self):
        flat = CTRL.split("private boolean isFlatPickupPocketEntry(", 1)[1].split(
            "private List<ReaderSnapshot.Node> flattenedPocketContents(", 1
        )[0]
        self.assertIn('"checkbox".equals(node.role)', flat)
        self.assertIn("pickupPocketParent(node) != null", flat)
        self.assertIn("flattenedInventoryBrowse()", flat)
        activate = CTRL.split("public void activate()", 1)[1].split(
            "private void openEditor(", 1
        )[0]
        self.assertIn("if (isFlatPickupPocketEntry(node)) {", activate)
        self.assertIn("activateFlatPickupPocketEntry(node);", activate)
        # The flat-row branch runs before the generic form-field handling:
        # these rows carry an int field but no set_value action of their own.
        self.assertLess(activate.index("if (isFlatPickupPocketEntry(node)) {"),
                        activate.index("// Form fields (R02.3)"))
        select = CTRL.split("private void activateFlatPickupPocketEntry(", 1)[1].split(
            "private void commitFlatPickupAmount(", 1
        )[0]
        self.assertIn("if (item.fieldMax > 1) {", select)
        self.assertIn("openFlatPickupQuantityEditor(item);", select)
        self.assertIn("commitFlatPickupAmount(item, pocket, pocketAmount(item) == 0 ? 1 : 0);",
                      select)
        commit = CTRL.split("private void commitFlatPickupAmount(", 1)[1].split(
            "private void openFlatPickupQuantityEditor(", 1
        )[0]
        # The commit targets the parent pocket row, never the entry row.
        self.assertIn("focus = pocket.id;", commit)
        self.assertIn('send("set_value", item.id + "=" + amount);', commit)
        self.assertIn("focus = previous;", commit)

    def test_quantity_editor_commits_a_stack_to_the_parent_pocket(self):
        editor = CTRL.split("private void openFlatPickupQuantityEditor(", 1)[1].split(
            "private void navigatePocketPicker(", 1
        )[0]
        self.assertIn("new ReaderTextEditor(activity", editor)
        self.assertIn("true, true, 0.0, item.fieldMax, 1.0, true", editor)
        # The parent is re-resolved from the fresh snapshot on submit.
        self.assertIn("pickupPocketParent(current)", editor)
        self.assertIn("focus = parent.id;", editor)
        self.assertIn('formRequest = send("set_value", itemId + "=" + amount);', editor)
        # No new strings: the existing quantity/pocket strings are reused.
        self.assertIn("R.string.a11yQuantityInstructions", editor)
        self.assertIn("R.string.a11yPocketSelectionError", editor)

    def test_native_side_accepts_only_the_parent_pocket_commit(self):
        # Pickup publishes checkbox pocket rows bound to real item
        # locations, but with focus/read_detail only: no set_value and
        # no activate of their own.
        self.assertIn('contained.role = "checkbox";', NATIVE)
        self.assertIn('contained.actions = { "focus", "read_detail" };', NATIVE)
        self.assertIn("pocket_binding.items.push_back( { contained.id, column, child } );",
                      NATIVE)
        # Only the parent "pocket:" row accepts set_value, and its
        # handler validates every entry against the live pocket before
        # applying any of it through select_count.
        self.assertIn('pocket_node.actions = { "focus", "set_value" };', NATIVE)
        self.assertIn("select_count( *multi, change.choice->column,", NATIVE)
        # A set_value sent straight to a pocket-entry row is refused:
        # the pocket-entry branch ends in the atomic-parent model.
        pocket_entry = NATIVE.split(
            'if( intent.node.rfind( "pocket-entry:", 0 ) == 0 ) {', 1)[1].split(
            "const auto binding = std::find_if( bindings.begin(), bindings.end(),", 1
        )[0]
        self.assertIn("Pickup/drop quantity forms keep their existing atomic", pocket_entry)
        self.assertIn("return std::nullopt;", pocket_entry)
        # Detail on a flat pickup row still works: it resolves through
        # the same pocket selections the picker uses.
        self.assertIn("for( const pocket_selection_binding &pocket : pocket_selections )",
                      pocket_entry)

    def test_staged_picker_still_owns_its_drafts(self):
        # Flat browsing only runs with the picker closed.
        guard = CTRL.split("private boolean flattenedInventoryBrowse()", 1)[1].split(
            "private boolean isFlattenedPocketEntry(", 1
        )[0]
        self.assertIn("pocketStage == 0", guard)
        navigate = CTRL.split("public void navigate(int delta)", 1)[1].split(
            "public boolean adjustFocusedSetting(", 1
        )[0]
        self.assertIn("if (pocketStage != 0) {", navigate)
        self.assertIn("navigatePocketPicker(delta);", navigate)
        # The picker still stages drafts locally and commits them as one
        # batch to the parent pocket on Confirm.
        commit = CTRL.split("private void commitPocketChoices()", 1)[1].split(
            "private void activatePocketPicker()", 1
        )[0]
        self.assertIn('long request = send("set_value", payload.toString());', commit)
        self.assertIn("focus = pocketCurrentId;", commit)
        items = CTRL.split("private List<ReaderSnapshot.Node> pocketItems()", 1)[1].split(
            "private int pocketAmount(", 1
        )[0]
        self.assertIn("pocketStage != 3", items)

    def test_vertical_swipes_still_browse_on_flat_pickup_rows(self):
        # A flat pickup row publishes no set_value action of its own, so
        # the step-gesture gate (which requires set_value on the focused
        # node) does not claim vertical swipes on it; they fall through
        # to navigation, like every other browse row. Top-level
        # multi-count rows used to adjust on a vertical swipe, but since
        # the stacked-row unification (see
        # test_stacked_row_vertical_swipe.py) they browse too: the
        # isItemQuantityRow() gate in adjustFocusedSetting() declines
        # them, and the amount is chosen by double-tapping the row.
        self.assertIn('contained.actions = { "focus", "read_detail" };', NATIVE)
        adjust = CTRL.split("public boolean adjustFocusedSetting(", 1)[1].split(
            "private boolean isCreatorMainStep()", 1
        )[0]
        self.assertIn('!node.actions.contains("set_value")', adjust)
        self.assertIn("if (isItemQuantityRow(node)) { return false; }", adjust)

    def test_pickup_here_screen_has_no_pockets_to_flatten(self):
        self.assertIn('"pickup-here"', HANDLE)
        self.assertIn('node.id = "pickup-here:item:"', HANDLE)
        self.assertIn('confirm.id = "pickup-here:confirm";', HANDLE)
        self.assertNotIn("pocket-entry", HANDLE)
        self.assertNotIn('"pocket:"', HANDLE)
        # Flat browsing is gated on the inventory category, so the
        # ground-pickup screen is untouched by it.
        guard = CTRL.split("private boolean flattenedInventoryBrowse()", 1)[1].split(
            "private boolean isFlattenedPocketEntry(", 1
        )[0]
        self.assertIn('"inventory".equals(snapshot.category)', guard)


if __name__ == "__main__":
    unittest.main()
