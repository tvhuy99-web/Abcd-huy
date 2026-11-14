"""Vertical swipe on a stacked top-level row browses, like every row.

Before this change, a one-finger vertical swipe on a top-level stacked
item row (for example "Wooden arrow x 5" in a pickup/drop quantity form)
changed the chosen amount by one, because the row publishes an int
set_value action and ReaderGestureSurface tries adjustFocusedSetting()
before browsing. Flat rows inside pockets have no set_value, so they
already browsed. Robert decided the swipe must be uniform: up/down
browses to the next/previous item on stacked rows too.

The fix is Java-only, in GameReaderController.adjustFocusedSetting():
a stacked item row is recognised by isItemQuantityRow() and makes the
gesture fall through to navigation. Choosing a different amount is done
by double-tapping the row, which already opens the quantity editor
(openFormEditor's quickQuantity path) -- no new wiring was needed for
that, and this file pins that the path exists.

The gate is deliberately narrow so real settings keep swipe-to-adjust:
settings sliders use role "slider" (world generation) or carry an
"apply" action (options, safemode, smart controller, auto-pickup), and
the standalone number-input popup lives in the NUMBER_INPUT category.
Only checkbox int rows with set_value, no apply, range 0..max>1 step 1,
in the "inventory" or "pickup-here" categories are treated as item
quantity rows.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CTRL = (ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/"
        "GameReaderController.java").read_text(encoding="utf-8")
SURFACE = (ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/"
           "ReaderGestureSurface.java").read_text(encoding="utf-8")
NATIVE = (ROOT / "src/inventory_ui.cpp").read_text(encoding="utf-8")
HANDLE = (ROOT / "src/handle_action.cpp").read_text(encoding="utf-8")
INPUT_POPUP = (ROOT / "src/input_popup.cpp").read_text(encoding="utf-8")
WORLDFACTORY = (ROOT / "src/worldfactory.cpp").read_text(encoding="utf-8")


def adjust_body():
    return CTRL.split("public boolean adjustFocusedSetting(", 1)[1].split(
        "private boolean isCreatorMainStep()", 1)[0]


def gate_body():
    return CTRL.split("private boolean isItemQuantityRow(", 1)[1].split(
        "public boolean adjustFocusedSetting(", 1)[0]


class StackedRowVerticalSwipeContracts(unittest.TestCase):
    def test_vertical_swipe_on_stacked_row_falls_through_to_browse(self):
        # The gate runs before the numeric branch and declines the
        # gesture, so ReaderGestureSurface falls through to the mapped
        # vertical action (navigate by default).
        adjust = adjust_body()
        self.assertIn("if (isItemQuantityRow(node)) { return false; }", adjust)
        gate_index = adjust.index("if (isItemQuantityRow(node)) { return false; }")
        numeric_index = adjust.index('!node.actions.contains("set_value")')
        self.assertLess(gate_index, numeric_index)
        # The surface still tries adjust first and navigates when it
        # declines -- the fall-through itself is unchanged.
        self.assertIn("if (!listener.adjustFocusedSetting(dy < 0 ? 1 : -1)) {",
                      SURFACE)
        self.assertIn("performMappedSwipeAction(listener.verticalSwipeAction(),"
                      " dy > 0 ? 1 : -1);", SURFACE)

    def test_gate_matches_only_stacked_item_rows(self):
        gate = gate_body()
        # Same shape openFormEditor already calls a quick-quantity
        # picker: checkbox, int, ranged 0..max>1, step 1, set_value,
        # no apply.
        self.assertIn('"checkbox".equals(node.role)', gate)
        self.assertIn('"int".equals(node.fieldType)', gate)
        self.assertIn("node.hasFieldRange", gate)
        self.assertIn("node.fieldMin == 0.0", gate)
        self.assertIn("node.fieldMax > 1.0", gate)
        self.assertIn("node.fieldStep == 1.0", gate)
        self.assertIn('node.actions.contains("set_value")', gate)
        self.assertIn('!node.actions.contains("apply")', gate)
        # Scoped to item screens only: inventory (pickup/drop/insert
        # quantity forms) and pickup-here (ground items).
        self.assertIn('"inventory".equals(snapshot.category)', gate)
        self.assertIn('"pickup-here".equals(snapshot.category)', gate)

    def test_native_top_level_stacked_rows_publish_the_gated_shape(self):
        # Inventory quantity form: only stacks larger than one get the
        # int set_value field; a single unit is a plain checkbox.
        self.assertIn("if( quantity_form ) {", NATIVE)
        self.assertIn("if( available > 1 ) {", NATIVE)
        self.assertIn('node.field_type = "int";', NATIVE)
        self.assertIn("node.field_min = 0.0;", NATIVE)
        self.assertIn("node.field_step = 1.0;", NATIVE)
        self.assertIn('node.actions = { "focus", "activate", "set_value", "read_detail" };',
                      NATIVE)
        # Pickup-here publishes the same shape for stacks.
        self.assertIn('node.id = "pickup-here:item:"', HANDLE)
        self.assertIn("if( choice.available > 1 && intent.action == \"set_value\" )",
                      HANDLE)
        self.assertIn("node.field_min = 0.0;", HANDLE)
        self.assertIn("node.field_step = 1.0;", HANDLE)
        self.assertIn('node.actions = { "focus", "activate", "set_value", "read_detail" };',
                      HANDLE)

    def test_double_tap_on_stacked_row_opens_the_quantity_editor(self):
        # activate() routes a field with set_value to openFormEditor
        # before the generic activate send, so double-tapping a stacked
        # row already opens the editor -- this change adds no new path.
        activate = CTRL.split("public void activate() {", 1)[1].split(
            "private void openEditor(", 1)[0]
        self.assertIn('if (node.actions.contains("set_value")) {', activate)
        self.assertIn("openFormEditor(node);", activate)
        # The editor treats exactly this shape as an immediate quantity
        # picker: the submitted number commits via set_value.
        editor = CTRL.split("private void openFormEditor(", 1)[1].split(
            "private long send(", 1)[0]
        self.assertIn('"checkbox".equals(node.role)', editor)
        self.assertIn('"int".equals(node.fieldType)', editor)
        self.assertIn("node.fieldMin == 0.0", editor)
        self.assertIn("node.fieldMax > 1.0", editor)
        self.assertIn("node.fieldStep == 1.0", editor)
        self.assertIn('!node.actions.contains("apply")', editor)
        self.assertIn('formRequest = send("set_value", text);', editor)
        self.assertIn("R.string.a11yQuantityInstructions", editor)

    def test_real_sliders_and_popups_keep_vertical_adjust(self):
        # World-generation sliders are int + set_value with no apply,
        # but they use role "slider", so the checkbox gate never
        # matches them -- and the native comment says vertical swipe
        # adjusts them by design.
        self.assertIn('option.role = "slider";', WORLDFACTORY)
        self.assertIn('option.actions = { "focus", "activate", "set_value" };',
                      WORLDFACTORY)
        self.assertIn("already adjusts ranged integer fields", WORLDFACTORY)
        # The standalone number-input popup publishes the same checkbox
        # int set_value shape, but under the NUMBER_INPUT category,
        # which the gate does not cover.
        self.assertIn('a11y::screen_scope reader( &ctxt, "NUMBER_INPUT",',
                      INPUT_POPUP)
        self.assertIn('field.role = "checkbox";', INPUT_POPUP)
        self.assertIn('field.actions = { "focus", "activate", "set_value" };',
                      INPUT_POPUP)
        # And the numeric branch itself still exists for everything
        # the gate does not claim: a declined row is the only change.
        adjust = adjust_body()
        self.assertIn('send("set_value", candidate.stripTrailingZeros().toPlainString())',
                      adjust)

    def test_horizontal_swipe_and_two_finger_detail_are_untouched(self):
        # Only the vertical branch consults adjustFocusedSetting; a
        # horizontal swipe navigates directly, as before.
        swipe = SURFACE.split("private void handleSwipe(", 1)[1].split(
            "private void performMappedSwipeAction(", 1)[0]
        self.assertEqual(swipe.count("adjustFocusedSetting"), 1)
        # The single adjust call sits in the vertical branch: everything
        # before it already contains the horizontal branch, which
        # navigates directly without consulting the gate.
        before = swipe.split("adjustFocusedSetting", 1)[0]
        self.assertIn("performMappedSwipeAction(listener.horizontalSwipeAction(),",
                      before)
        # Two-finger single tap still reads the focused row's detail;
        # nothing in this change touches that dispatch.
        self.assertIn("listener.twoFingerTap(count);", SURFACE)


if __name__ == "__main__":
    unittest.main()
