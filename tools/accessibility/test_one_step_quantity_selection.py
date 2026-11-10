"""One-step quantity selection across accessible item workflows.

The real native count parser is also compiled and exercised in
tests/a11y_context_test.cpp. This source contract ensures that every native
entry point and the Android reader share the same complete-number behavior.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
RES = ROOT / "android/app/src/main/res"


class OneStepQuantitySelectionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.inventory = (SRC / "inventory_ui.cpp").read_text("utf-8")
        cls.quick = (SRC / "handle_action.cpp").read_text("utf-8")
        cls.advanced = (SRC / "advanced_inv.cpp").read_text("utf-8")
        cls.popup = (SRC / "input_popup.cpp").read_text("utf-8")
        cls.controller = (JAVA / "GameReaderController.java").read_text("utf-8")
        cls.editor = (JAVA / "ReaderTextEditor.java").read_text("utf-8")

    def test_all_multiselect_screens_share_one_step_quantity(self):
        code = self.inventory
        self.assertIn('activation == "TOGGLE_ENTRY"', code)
        self.assertIn("dynamic_cast<const inventory_multiselector *>", code)
        self.assertIn("dynamic_cast<const inventory_compare_selector *>", code)
        self.assertIn("if( available > 1 )", code)
        self.assertIn('node.actions = { "focus", "activate", "set_value" };', code)
        self.assertIn("entry.get_available_count() == 1", code)
        self.assertIn("select_count( *multi, binding->column, binding->location,", code)
        self.assertIn("entry.chosen_count == 0 ? 1 : 0 );", code)
        self.assertIn("a11y::selected_quantity( intent.value", code)
        self.assertIn("select_count( *multi, binding->column, binding->location, *amount );", code)
        self.assertIn("multi.toggle_entry( **target, count );", code)
        self.assertNotIn('intent.action == "apply"', code)
        self.assertNotIn('"cancel_draft"', code)
        # Final CONFIRM still belongs to pickup/drop and is not auto-executed.
        self.assertIn('if( input.action == "CONFIRM" )', code)

    def test_quick_pickup_follows_same_standard(self):
        code = self.quick.split("static void pickup_here_accessible()", 1)[1].split(
            "input_context game::get_player_input", 1)[0]
        self.assertIn("choice.available == 1 && intent.action == \"activate\"", code)
        self.assertIn("a11y::selected_quantity( intent.value", code)
        self.assertIn("choice.selected = static_cast<int>( *amount );", code)
        self.assertIn('node.actions = { "focus", "activate", "set_value" };', code)
        self.assertNotIn("choice.draft", code)
        self.assertNotIn('intent.action == "apply"', code)
        self.assertIn('confirm.id = "pickup-here:confirm"', code)

    def test_advanced_inventory_complete_number_closes_native_modal(self):
        self.assertIn("if( possible_max == 1 || test_mode )", self.advanced)
        self.assertIn("choice.set_accessible_quantity_max( possible_max );", self.advanced)
        self.assertIn("choice.quantity_cancelled() ? 0 : selected", self.advanced)
        self.assertIn('intent.node == "quantity" && intent.action == "set_value"', self.popup)
        self.assertIn('return "TEXT.CONFIRM";', self.popup)
        self.assertIn("a11y::selected_quantity( intent.value", self.popup)
        self.assertIn('cancel.id = "cancel"', self.popup)

    def test_reader_uses_single_activation_per_complete_number(self):
        self.assertIn('final boolean quickQuantity = "checkbox".equals(node.role)', self.controller)
        self.assertIn('!node.actions.contains("apply")', self.controller)
        self.assertIn("quickQuantity, new ReaderTextEditor.Listener()", self.controller)
        self.assertIn('group = this.quantityChoices ? "quantities" : "digits"', self.editor)
        self.assertIn('items.add(new Choice(number, () -> listener.submit(number)))',
                      self.editor)
        self.assertIn('() -> listener.submit(formatNumber(rangeMax))', self.editor)
        self.assertIn('add(R.string.a11yQuantityOther, () -> open("digits"))', self.editor)
        self.assertIn('add(R.string.a11yQuantityDeselect, () -> listener.submit("0"))',
                      self.editor)
        self.assertIn("if (editor == null || formRequest != 0 || snapshot == null",
                      self.controller)  # prevents duplicate commits while pending

    def test_container_rows_select_before_opening_read_only_pockets(self):
        # A pickup checkbox can also be a tree parent because it owns
        # informational pocket-status children. Double-tap belongs to the
        # item selection, not the non-actionable pocket child.
        code = self.controller.split("public void activate()", 1)[1].split(
            "private void openEditor(", 1)[0]
        self.assertIn('node.id.startsWith("item:")', code)
        self.assertIn('node.actions.contains("activate")', code)
        self.assertIn('if (node.hasChildren && !actionableItem)', code)
        self.assertLess(code.index("if (node.hasChildren && !actionableItem)"),
                        code.index("if (!node.fieldType.isEmpty())"))
        self.assertLess(code.index("if (!node.fieldType.isEmpty())"),
                        code.index('send("activate", "");'))
        self.assertIn('pocket_node.actions = { "focus" };', self.inventory)
        self.assertIn('intent.node.rfind( "pocket:", 0 ) == 0', self.inventory)
        self.assertIn('child.id.startsWith("pocket:")', self.controller)
        self.assertIn('node.id.equals(child.parentId)', self.controller)
        self.assertIn('information += ". " + nodeText(child, true);', self.controller)

    def test_other_numeric_forms_keep_existing_submit_behavior(self):
        self.assertIn('this(activity, initial, numeric, integer, rangeMin, rangeMax, rangeStep, false, listener)',
                      self.editor)
        self.assertIn('add(R.string.a11yEditorApply, this::submitCurrent)', self.editor)
        self.assertIn('node.actions.contains("set_value")', self.controller)
        self.assertIn('openFormEditor(node)', self.controller)
        self.assertIn('if (quantityChoices)', self.editor)

    def test_vietnamese_accessibility_copy(self):
        vi = (RES / "values-vi/a11y_editor.xml").read_text("utf-8")
        en = (RES / "values/a11y_editor.xml").read_text("utf-8")
        for key in ("a11yQuantityInstructions", "a11yQuantityDeselect",
                    "a11yQuantityAll", "a11yQuantityOther"):
            self.assertIn('name="' + key + '"', vi)
            self.assertIn('name="' + key + '"', en)


if __name__ == "__main__":
    unittest.main()
