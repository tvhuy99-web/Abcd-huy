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
TWO_FINGER = (ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/TwoFingerBackGesture.java").read_text(encoding="utf-8")
VI = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")


class AccessibleCraftingCategoryGesturesTest(unittest.TestCase):
    def test_global_all_only_uses_visible_real_categories_not_internal_nested_groups(self):
        self.assertIn("reader_all_categories = true;", CRAFT)
        self.assertIn('"All categories"', CRAFT)
        self.assertIn("reader_all_categories ? std::string", CRAFT)
        self.assertIn('!reader_all_categories && ( subtab.cur() == "CSC_*_RECENT" )', CRAFT)
        # CC_* is the internal wildcard recipe category with nested group
        # placeholders; it must not inflate All or trap main-category swipes.
        self.assertIn("( !a11y::enabled() || !cat.is_wildcard )", CRAFT)
        self.assertIn("entry->category.str() ) != crafting_cats.end()", CRAFT)
        self.assertNotIn("picking.insert( picking.end(), available_recipes->begin(),", CRAFT)
        # Preserve native visual-mode special Favorites/Recent/Hidden tabs.
        self.assertIn("!a11y::enabled() || !cat.is_wildcard", CRAFT)

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

    def test_two_finger_horizontal_main_one_finger_vertical_sub_and_horizontal_recipe(self):
        self.assertIn("handleCraftingMainCategorySwipe(left ? -1 : 1)", GESTURES)
        self.assertIn("handleCraftingSubcategorySwipe(vertical)", GESTURES)
        self.assertIn("handleCraftingRecipeSwipe(horizontal)", GESTURES)
        self.assertIn("navigate(horizontal > 0 ? 1 : -1);", READER)
        self.assertIn('"crafting".equals(snapshot.category)', READER)
        self.assertIn('horizontal < 0 ? "PREV_TAB" : "NEXT_TAB"', READER)
        self.assertIn("TwoFingerBackGesture.matchesLeft", GESTURES)
        self.assertIn("TwoFingerBackGesture.matchesRight", GESTURES)
        self.assertIn("static boolean isLeftSwipe", TWO_FINGER)
        self.assertIn("static boolean isRightSwipe", TWO_FINGER)
        # Main categories must no longer override the two-finger vertical actions.
        self.assertNotIn("handleCraftingMainCategorySwipe(up ? -1 : 1)", GESTURES)
        self.assertIn('vertical < 0 ? "LEFT" : "RIGHT"', READER)
        self.assertIn("navigate(int delta)", READER)
        # Existing non-crafting two-finger confirm/back is still present.
        self.assertIn("listener.confirm();", GESTURES)
        self.assertIn("listener.goBack();", GESTURES)

    def test_category_commands_apply_in_native_game_thread(self):
        self.assertIn("process_action( intent.node, ctxt );", CRAFT)
        self.assertIn('"CRAFT_CATEGORY_NAV"', CRAFT)
        self.assertIn("All categories has no subcategories.", CRAFT)
        self.assertIn("ignored=global_all_subcategory", CRAFT)

    def test_horizontal_recipes_skip_detail_tail_and_page_backward_correctly(self):
        self.assertIn("List<ReaderSnapshot.Node> recipeNodes = new ArrayList<>();", READER)
        self.assertIn('node.id.startsWith("recipe:")', READER)
        self.assertIn('craftingCommand("craft:page:next")', READER)
        self.assertIn('craftingCommand("craft:page:previous")', READER)
        self.assertIn("if (nextRecipe < 0)", READER)
        self.assertIn("std::min( first + page_size - 1", CRAFT)

    def test_swipe_left_from_weapon_skips_empty_internal_favorites_tab(self):
        from json import loads
        categories = loads((ROOT / "data/json/recipes/recipes.json").read_text(encoding="utf-8"))
        wildcard = next(c for c in categories if c.get("id") == "CC_*")
        self.assertTrue(wildcard["is_wildcard"])
        self.assertIn("CSC_*_FAVORITE", wildcard["recipe_subcategories"])
        groups = loads((ROOT / "data/json/recipes/nested.json").read_text(encoding="utf-8"))
        self.assertGreater(
            sum(g.get("category") == "CC_*" and g.get("type") == "nested_category"
                for g in groups), 300
        )
        self.assertIn("!cat.is_wildcard", CRAFT)
        self.assertIn("a11y::enabled() && tab.cur_index() == 0", CRAFT)
        self.assertIn("reader_all_categories = true;", CRAFT)

    def test_native_category_selection_cannot_be_reverted_by_imgui(self):
        # In the 539b0655 phone log PREV_TAB produced a 20-recipe native
        # category and ImGui immediately restored All with 1121 recipes.
        self.assertIn("reader_all_categories = a11y::enabled() && !goto_recipe.is_valid();", CRAFT)
        self.assertIn("ImGui::IsItemClicked() && !reader_all_categories", CRAFT)
        self.assertIn("( !a11y::enabled() || ImGui::IsItemClicked() )", CRAFT)
        self.assertIn("subcategory=", CRAFT)
        self.assertIn("category=", CRAFT)

    def test_swipe_left_at_first_recipe_wraps_to_last_without_detail_rows(self):
        # Page controls are present even on first/last page so the native
        # engine, not local Java focus, selects the last real recipe.
        self.assertIn("if( current.size() > 1 )", CRAFT)
        self.assertIn("line = first < 0 ? static_cast<int>( current.size() ) - 1 :", CRAFT)
        self.assertIn("line = first >= static_cast<int>( current.size() ) ? 0 : first;", CRAFT)
        self.assertIn('craftingCommand("craft:page:previous")', READER)
        self.assertIn('craftingCommand("craft:page:next")', READER)
        self.assertIn('node.id.startsWith("recipe:")', READER)

    def test_time_translation_and_similar_recipe_names(self):
        self.assertIn('msgid "Estimated time: %s"\nmsgstr "Thời gian dự kiến: %s"', VI)
        self.assertIn('msgid "All categories"\nmsgstr "Tất cả"', VI)
        self.assertIn('"Recipe group: %s"', CRAFT)
        self.assertIn('"(recipe %1$d of %2$d)"', CRAFT)


if __name__ == "__main__":
    unittest.main()
