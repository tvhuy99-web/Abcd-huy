"""Regression contracts for the crafting + construction detail screen.

Tapping a recipe/construction opens a detail screen instead of crafting
or building at once. Nothing is crafted/built until the detail screen's
confirm button is pressed. Uses source-level checks to protect UI
semantics even when an Android emulator is unavailable. Native/phone
runtime still needs separate CI.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CRAFT = (ROOT / "src/crafting_gui.cpp").read_text(encoding="utf-8")
CONSTRUCTION = (ROOT / "src/construction.cpp").read_text(encoding="utf-8")
VI = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")


def _region(text, start, end):
    return text.split(start, 1)[1].split(end, 1)[0]


class CraftingDetailScreenTest(unittest.TestCase):
    def test_recipe_activate_opens_detail_instead_of_crafting(self):
        loop = _region(CRAFT, "for( std::size_t i = 0; i < current.size(); ++i ) {",
                       "    return a11y::registered_intent( ctxt, intent );")
        self.assertIn('if( intent.action == "activate" && !current[line]->is_nested() ) {', loop)
        self.assertIn("reader_detail_open = true;", loop)
        self.assertIn("reader_detail_recipe = current[line]->ident().str();", loop)
        self.assertIn("reader_detail_quantity = manual_batch;", loop)
        # Nested recipe groups still expand in place on activate.
        self.assertIn('return intent.action == "activate" ? "CONFIRM" : "TIMEOUT";', loop)

    def test_detail_screen_focus_order(self):
        detail = _region(CRAFT, "    const recipe &rec = *current[line];",
                         "    if( !rec.is_nested() ) {\n        a11y::semantic_node batch_field;")
        order = ['"craft:detail:name"', '"craft:detail:intro"',
                 'append_reader_requirement_nodes( detail, "craft:detail"',
                 '"craft:detail:quantity"', '"craft:detail:cancel"',
                 '"craft:detail:confirm"']
        positions = [detail.index(item) for item in order]
        self.assertEqual(positions, sorted(positions))
        self.assertIn('snapshot.focus = "craft:detail:name";', detail)

    def test_second_row_is_actual_detail_text_not_an_action_label(self):
        detail = _region(CRAFT, 'intro.id = "craft:detail:intro";',
                         "detail.push_back( std::move( intro ) );")
        self.assertIn("intro.label = full_recipe_detail_text( rec, reader_detail_quantity );",
                      detail)
        self.assertIn("intro.label = accessible_recipe_description( rec, *crafter );",
                      detail)
        self.assertIn("intro.detail = intro.label;", detail)
        self.assertIn('intro.actions = { "focus", "read_detail" };', detail)
        self.assertNotIn('intro.label = _( "Description" );', detail)
        self.assertNotIn('intro.label = _( "Read details" );', detail)
        self.assertNotIn("activate", detail)
        self.assertNotIn('msgid "Read details"', VI)

    def test_detail_content_is_spoken_on_entry_and_on_focus(self):
        java = (ROOT / "android/app/src/main/java/com/cleverraven/"
                "cataclysmdda/GameReaderController.java").read_text(encoding="utf-8")
        self.assertIn('boolean enteredCraftRecipeDetail = "crafting".equals(next.category)',
                      java)
        self.assertIn('next.find("craft:detail:intro") != null', java)
        self.assertIn('previousSnapshot.find("craft:detail:qty:current") == null',
                      java)
        self.assertIn('} else if (enteredCraftRecipeDetail', java)
        self.assertIn('String details = intro.detail.isEmpty() ? intro.label : intro.detail;',
                      java)
        self.assertIn('speakInlineDetail(introduction.toString());', java)
        self.assertIn('"craft:detail:intro".equals(node.id)', java)
        self.assertIn('speakInlineDetail(node.detail.isEmpty() ? node.label : node.detail);',
                      java)

    def test_two_finger_detail_still_reads_complete_recipe(self):
        intent = _region(CRAFT, "std::optional<std::string> crafting_ui_impl::reader_intent(",
                         "    // Apply spoken tab/subtab controls")
        self.assertIn('if( intent.action == "read_detail" ||', intent)
        self.assertIn('full_recipe_detail_text( *current[line], reader_detail_quantity )',
                      intent)

    def test_two_fingers_up_confirms_crafting_detail_with_selected_quantity(self):
        java = (ROOT / "android/app/src/main/java/com/cleverraven/"
                "cataclysmdda/GameReaderController.java").read_text(encoding="utf-8")
        confirm = _region(java, "public void confirm() {",
                          "public boolean movementPracticeEnabled()")
        self.assertIn('if ("crafting".equals(snapshot.category)) {', confirm)
        self.assertIn('snapshot.find("craft:detail:qty:current") != null', confirm)
        self.assertIn('snapshot.find("craft:detail:confirm")', confirm)
        self.assertIn('focus = craft.id;', confirm)
        self.assertIn('send("activate", "");', confirm)
        self.assertLess(confirm.index('snapshot.find("craft:detail:confirm")'),
                        confirm.index('for (String id : new String[] { "CONFIRM"'))
        native = _region(CRAFT, 'if( reader_detail_open ) {\n        // Back closes',
                         '// Apply spoken tab/subtab controls')
        self.assertIn('intent.node == "craft:detail:confirm"', native)
        self.assertIn('manual_batch = reader_detail_quantity;', native)
        self.assertIn('return "CONFIRM";', native)

    def test_each_material_alternative_group_has_its_own_numbered_heading(self):
        helper = _region(CRAFT, "static void append_reader_requirement_nodes(",
                         '// Resolve a "…:req:<kind>:<group>:<alt>"')
        self.assertIn('heading.label = string_format( _( "Requirement %d" ),', helper)
        self.assertIn('heading.value = _( "Choose one of the following." );',
                      helper)
        self.assertIn('heading.actions = { "focus" };', helper)
        self.assertIn('heading.id = prefix + ":requirement:"', helper)
        self.assertEqual(helper.count("add_requirement_header( group.size() );"), 3)
        self.assertIn('if( alternatives > 1 ) {', helper)
        self.assertIn('++requirement_number', helper)
        self.assertIn('std::to_string( group_index ) + ":" +', helper)
        self.assertIn('msgid "Requirement %d"\nmsgstr "Yêu cầu %d"', VI)

    def test_requirement_focuses_announce_have_and_need(self):
        helper = _region(CRAFT, "static int reader_requirement_have(",
                         "std::string crafting_ui_impl::full_recipe_detail_text")
        self.assertIn("const int have = reader_requirement_have( alternative.type, inventory, filter );",
                      helper)
        self.assertIn("alternative.to_string( batch, have )", helper)
        self.assertIn('_( "Available." )', helper)
        self.assertIn('_( "Missing." )', helper)
        self.assertIn('node.actions = { "focus", "read_detail" };', helper)
        for kind in ("quality", "tool", "component"):
            self.assertIn('":req:' + kind + ':"', helper)

    def test_quantity_picker_offers_1_to_10_and_manual_entry(self):
        detail = _region(CRAFT, "    const recipe &rec = *current[line];",
                         "    if( !rec.is_nested() ) {\n        a11y::semantic_node batch_field;")
        self.assertIn("for( int choice = 1; choice <= 10; ++choice ) {", detail)
        self.assertIn('"craft:detail:qty:manual"', detail)
        self.assertIn('_( "Enter a quantity manually" )', detail)
        intent = _region(CRAFT, "std::optional<std::string> crafting_ui_impl::reader_intent(",
                         "    // Apply spoken tab/subtab controls")
        self.assertIn('intent.node == "craft:detail:qty:manual"', intent)
        self.assertIn("quantity < 1 || quantity > 50", intent)
        self.assertIn("reader_quantity_receipt = intent;", intent)

    def test_confirm_crafts_the_selected_quantity(self):
        intent = _region(CRAFT, "std::optional<std::string> crafting_ui_impl::reader_intent(",
                         "    // Apply spoken tab/subtab controls")
        confirm = _region(intent, 'intent.node == "craft:detail:confirm"',
                          'intent.node == "craft:detail:quantity"')
        self.assertIn("manual_batch = reader_detail_quantity;", confirm)
        self.assertIn('return "CONFIRM";', confirm)
        # No new result announcement on confirm: the game's own messages
        # already cover the outcome; the player re-checks them with the
        # game-wide two-finger double-tap.
        self.assertNotIn("announce", confirm)

    def test_back_closes_detail_instead_of_the_menu(self):
        intent = _region(CRAFT, "std::optional<std::string> crafting_ui_impl::reader_intent(",
                         "    // Apply spoken tab/subtab controls")
        back = _region(intent, 'if( reader_detail_open ) {',
                       'if( intent.action == "activate" && intent.node == "craft:detail:confirm" )')
        self.assertIn('intent.action == "cancel"', back)
        self.assertIn('intent.node == "QUIT"', back)
        self.assertIn("reader_detail_open = false;", back)
        self.assertNotIn('"CONFIRM"', back)

    def test_two_finger_detail_describes_the_focused_item(self):
        intent = _region(CRAFT, "std::optional<std::string> crafting_ui_impl::reader_intent(",
                         "    // Apply spoken tab/subtab controls")
        self.assertIn("full_recipe_detail_text( *current[line], reader_detail_quantity )", intent)
        self.assertIn("a11y::announce_full_item_info( example, label );", intent)
        self.assertIn("a11y::announce_full_detail_text( label );", intent)


class ConstructionDetailScreenTest(unittest.TestCase):
    def test_construction_activate_opens_detail_instead_of_building(self):
        handler = _region(CONSTRUCTION, 'a11y::screen_scope reader( &ctxt, "construction",',
                          "    std::string reader_detail_key;")
        self.assertIn("reader_detail_open = true;", handler)
        self.assertIn("reader_detail_group = constructs[i].str();", handler)
        self.assertNotIn('return intent.action == "activate" ? "CONFIRM" : "TIMEOUT";', handler)

    def test_detail_screen_focus_order_and_fixed_quantity(self):
        detail = _region(CONSTRUCTION, "} else {\n                    // The detail screen replaces the list",
                         '                    snapshot.focus = "construction:detail:name";')
        order = ['"construction:detail:name"', '"construction:detail:intro"',
                 'append_reader_requirement_nodes( detail, "construction:detail"',
                 '"construction:detail:quantity"', '"construction:detail:cancel"',
                 '"construction:detail:confirm"']
        positions = [detail.index(item) for item in order]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("quantity.enabled = false;", detail)
        self.assertIn('_( "Construction builds one at a time; quantity is fixed at 1." )', detail)
        self.assertIn('_( "Construct" )', detail)
        self.assertIn('snapshot.focus = "construction:detail:name";',
                      _region(CONSTRUCTION, "} else {\n                    // The detail screen replaces the list",
                              "            reader.update( std::move( snapshot ) );"))

    def test_requirements_come_from_the_first_visible_stage(self):
        detail = _region(CONSTRUCTION, "} else {\n                    // The detail screen replaces the list",
                         '                    snapshot.focus = "construction:detail:name";')
        self.assertIn("( filter_mode == 1 && !( resources && !location ) )", detail)
        self.assertIn("( filter_mode == 2 && !( resources && location ) )", detail)
        self.assertIn("stage_requirements = &*con->requirements;", detail)

    def test_back_closes_detail_and_confirm_builds(self):
        handler = _region(CONSTRUCTION, 'a11y::screen_scope reader( &ctxt, "construction",',
                          "    std::string reader_detail_key;")
        back = _region(handler, 'if( reader_detail_open ) {',
                       'intent.node == "construction:detail:confirm"')
        self.assertIn('intent.action == "cancel"', back)
        self.assertIn('intent.node == "QUIT"', back)
        self.assertIn("reader_detail_open = false;", back)
        self.assertNotIn('"CONFIRM"', back)
        confirm = _region(handler, 'intent.node == "construction:detail:confirm"',
                          'intent.action == "read_detail"')
        self.assertIn('return "CONFIRM";', confirm)
        self.assertNotIn("announce", confirm)

    def test_two_finger_detail_describes_the_focused_item(self):
        handler = _region(CONSTRUCTION, 'a11y::screen_scope reader( &ctxt, "construction",',
                          "    std::string reader_detail_key;")
        self.assertIn("a11y::announce_full_item_info( example, label );", handler)
        self.assertIn("a11y::announce_full_detail_text( label );", handler)


class DetailScreenSharedTest(unittest.TestCase):
    def test_requirement_helpers_stay_in_sync(self):
        # The helper twin lives in both files because the shared home
        # (a11y_requirements.cpp) is outside the sparse checkout.
        craft = _region(CRAFT, "static int reader_requirement_have(",
                        "std::string crafting_ui_impl::full_recipe_detail_text")
        construction = _region(CONSTRUCTION, "static int reader_requirement_have(",
                               "construction_id construction_menu")
        self.assertEqual(craft, construction)

    def test_new_strings_are_translated(self):
        self.assertIn('msgid "Enter a quantity manually"\nmsgstr "Nhập số lượng thủ công"', VI)
        self.assertIn('msgid "Construction builds one at a time; quantity is fixed at 1."\n'
                      'msgstr "Xây dựng làm từng cái một; số lượng cố định là 1."', VI)
        self.assertIn('msgid "Construct"\nmsgstr "Xây dựng"', VI)


if __name__ == "__main__":
    unittest.main()
