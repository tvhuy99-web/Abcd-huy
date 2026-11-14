"""Quantity selection is independent from two-axis inventory navigation."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
CTRL = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
SURFACE = (JAVA / "ReaderGestureSurface.java").read_text(encoding="utf-8")
NATIVE = (ROOT / "src/inventory_ui.cpp").read_text(encoding="utf-8")


class QuantityAndTwoAxisContracts(unittest.TestCase):
    def test_inventory_swipes_precede_generic_numeric_adjustment(self):
        self.assertLess(
            SURFACE.index("listener.handleInventoryAxisSwipe(horizontal, vertical)"),
            SURFACE.index("listener.adjustFocusedSetting(dy < 0 ? 1 : -1)"))
        self.assertIn("if (isItemQuantityRow(node)) { return false; }", CTRL)

    def test_double_tap_keeps_existing_quantity_editor(self):
        self.assertIn("final boolean quickQuantity", CTRL)
        self.assertIn("openFormEditor(node);", CTRL)
        self.assertIn('node.actions = { "focus", "activate", "set_value", "read_detail" };',
                      NATIVE)
        self.assertIn('node.actions = { "focus", "activate", "read_detail" };',
                      NATIVE)

    def test_settings_keep_their_numeric_adjustment(self):
        self.assertIn("public boolean adjustFocusedSetting(int delta)", CTRL)
        self.assertIn('node.actions.contains("set_value")', CTRL)
        self.assertIn('node.actions.contains("select_choice")', CTRL)


if __name__ == "__main__":
    unittest.main()
