"""Regression contracts for Vietnamese Android crafting navigation.

Uses source-level checks to protect UI semantics even when an Android
emulator is unavailable. Native/phone runtime still needs separate CI.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CRAFT = (ROOT / "src/crafting_gui.cpp").read_text(encoding="utf-8")
READER = (ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/GameReaderController.java").read_text(encoding="utf-8")
GESTURES = (ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/ReaderGestureSurface.java").read_text(encoding="utf-8")
VI = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")


class AccessibleCraftingCategoryGesturesTest(unittest.TestCase):
    def test_global_all_uses_every_group_recipe_not_subcategory_all(self):
        self.assertIn("reader_all_categories = true;", CRAFT)
        self.assertIn("available_recipes->begin(), available_recipes->end()", CRAFT)
        self.assertIn('"All categories"', CRAFT)
        self.assertIn("reader_all_categories ? std::string", CRAFT)
        self.assertIn('!reader_all_categories && ( subtab.cur() == "CSC_*_RECENT" )', CRAFT)

    def test_reader_list_is_bounded_for_large_global_all(self):
        self.assertIn("constexpr std::size_t reader_page_size = 80;", CRAFT)
        self.assertIn('"craft:page:next"', CRAFT)
        self.assertIn('"craft:page:previous"', CRAFT)
        self.assertIn('rows.insert( rows.end(), std::make_move_iterator( snapshot.nodes.begin() )', CRAFT)
        self.assertIn('craftingCommand("craft:page:next")', READER)
        self.assertIn('craftingCommand("craft:page:previous")', READER)

    def test_transient_empty_list_and_focus_are_recovered(self):
        self.assertIn("Recover every transient empty subtab", CRAFT)
        self.assertIn('snapshot.focus = "craft:no-recipes";', CRAFT)
        self.assertIn("craftingNavigationCompleted", READER)
        self.assertIn("craftingNavigationPending = false;", READER)

    def test_two_finger_main_one_finger_sub_and_horizontal_recipe(self):
        self.assertIn("handleCraftingMainCategorySwipe(up ? -1 : 1)", GESTURES)
        self.assertIn("handleCraftingSubcategorySwipe(vertical)", GESTURES)
        self.assertIn("handleCraftingRecipeSwipe(horizontal)", GESTURES)
        self.assertIn("navigate(horizontal > 0 ? 1 : -1);", READER)
        self.assertIn('"crafting".equals(snapshot.category)', READER)
        self.assertIn('vertical < 0 ? "PREV_TAB" : "NEXT_TAB"', READER)
        self.assertIn('vertical < 0 ? "LEFT" : "RIGHT"', READER)
        self.assertIn("navigate(int delta)", READER)
        # Existing non-crafting two-finger confirm/back is still present.
        self.assertIn("listener.confirm();", GESTURES)
        self.assertIn("listener.goBack();", GESTURES)

    def test_time_translation_and_similar_recipe_names(self):
        self.assertIn('msgid "Estimated time: %s"\nmsgstr "Thời gian dự kiến: %s"', VI)
        self.assertIn('msgid "All categories"\nmsgstr "Tất cả"', VI)
        self.assertIn('"Recipe group: %s"', CRAFT)
        self.assertIn('"(recipe %1$d of %2$d)"', CRAFT)


if __name__ == "__main__":
    unittest.main()
