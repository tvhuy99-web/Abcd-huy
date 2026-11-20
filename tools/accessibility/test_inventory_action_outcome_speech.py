"""Regression contracts for concise inventory outcomes on Android.

These are source-level guardrails, not a substitute for an on-device TTS test.
"""
import pathlib
import unittest
import xml.etree.ElementTree as ET

ROOT = pathlib.Path(__file__).resolve().parents[2]
CPP = (ROOT / "src/inventory_ui.cpp").read_text(encoding="utf-8")
JAVA = (ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/"
        "GameReaderController.java").read_text(encoding="utf-8")


class InventoryActionOutcomeTests(unittest.TestCase):

    def test_selection_only_shortens_a_stable_inventory_checkbox(self):
        helper = JAVA.split("private String inventoryMarkFeedback(", 1)[1].split(
            "private String nodeText(", 1)[0]
        self.assertIn('!"inventory".equals(after.category)', helper)
        self.assertIn("before.contextKey().equals(after.contextKey())", helper)
        self.assertIn('!"checkbox".equals(row.role)', helper)
        self.assertIn("oldRow.label.equals(row.label)", helper)
        self.assertIn("oldRow.checked != row.checked", helper)
        self.assertIn("oldRow.appliedValue.equals(row.appliedValue)", helper)
        self.assertIn("row.appliedValue.matches", helper)
        self.assertIn("R.string.a11yInventoryMarkedQuantity", helper)
        self.assertIn("R.string.a11yInventoryUnmarked", helper)
        self.assertIn("R.string.a11yInventoryMarked", helper)

    def test_snapshot_speaks_short_mark_without_replaying_focus(self):
        self.assertIn("previous.equals(focus)", JAVA)
        self.assertIn("inventoryMarkFeedback(previousSnapshot, next, focus)", JAVA)
        self.assertIn("if (!shortInventoryMark.isEmpty())", JAVA)
        self.assertIn("feedback(shortInventoryMark);", JAVA)
        self.assertIn("} else {\n                        readCurrent();", JAVA)
        # No changes to the two-axis pocket browsing mechanism.
        self.assertIn("public boolean handleInventoryAxisSwipe(int horizontal, int vertical)", JAVA)
        self.assertIn("flattenedPocketContents(root.id)", JAVA)

    def test_native_filter_and_view_change_only_announce_real_results(self):
        native = CPP.split("void inventory_selector::on_input(", 1)[1].split(
            "void inventory_selector::on_change(", 1)[0]
        self.assertIn("const std::string old_filter = filter;", native)
        self.assertIn("if( filter != old_filter )", native)
        self.assertIn("const bool was_filtered = !filter.empty();", native)
        self.assertIn("if( was_filtered )", native)
        self.assertIn("Inventory filter cleared.", native)
        self.assertIn("Inventory filter applied: %s.", native)
        self.assertIn("Inventory view: container hierarchy.", native)
        self.assertIn("Inventory view: categories.", native)
        self.assertIn("Category selection mode.", native)
        self.assertIn("Item selection mode.", native)
        self.assertIn("Unavailable items skipped.", native)
        self.assertIn("Unavailable items included.", native)
        self.assertNotIn("Expanded, ", native)

    def test_native_inventory_outcomes_are_translated_to_vietnamese(self):
        translated = (ROOT / "lang/po/accessibility/vi.po").read_text(
            encoding="utf-8")
        for text in (
            "Category selection mode.",
            "Item selection mode.",
            "Inventory view: container hierarchy.",
            "Inventory view: categories.",
            "Inventory filter cleared.",
            "Inventory filter applied: %s.",
            "Unavailable items skipped.",
            "Unavailable items included.",
        ):
            self.assertIn('msgid "' + text + '"', translated)
        self.assertIn('msgstr "Đã áp dụng bộ lọc kho đồ: %s."', translated)

    def test_inventory_speech_has_vietnamese_strings(self):
        en = ET.parse(ROOT / "android/app/src/main/res/values/strings.xml").getroot()
        vi = ET.parse(ROOT / "android/app/src/main/res/values-vi/strings.xml").getroot()
        required = {"a11yInventoryMarked", "a11yInventoryUnmarked",
                    "a11yInventoryMarkedQuantity"}
        for document in (en, vi):
            keys = {element.get("name") for element in document.findall("string")}
            self.assertTrue(required.issubset(keys))


if __name__ == "__main__":
    unittest.main()
