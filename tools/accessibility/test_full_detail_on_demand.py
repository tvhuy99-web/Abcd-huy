"""Source-contract regression tests for optional complete two-finger details.

These tests do not claim that a real APK was compiled or that TalkBack has
been tested on a device. They protect native read-only intents and Java
presentation against common accessibility regressions.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/GameReaderController.java"


class FullDetailOnDemandTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.item_text = (SRC / "a11y_item_text.h").read_text("utf-8")
        cls.item_info = (SRC / "item_info.cpp").read_text("utf-8")
        cls.inventory = (SRC / "inventory_ui.cpp").read_text("utf-8")
        cls.advanced = (SRC / "advanced_inv.cpp").read_text("utf-8")
        cls.armor = (SRC / "armor_layers.cpp").read_text("utf-8")
        cls.nearby = (SRC / "surroundings_menu.cpp").read_text("utf-8")
        cls.pickup_here = (SRC / "handle_action.cpp").read_text("utf-8")
        cls.crafting = (SRC / "crafting_gui.cpp").read_text("utf-8")
        cls.help = (SRC / "help.cpp").read_text("utf-8")
        cls.diary = (SRC / "diary_ui.cpp").read_text("utf-8")
        cls.runtime = (SRC / "a11y_runtime.cpp").read_text("utf-8")
        cls.java = JAVA.read_text("utf-8")

    def test_item_details_reuse_all_native_info_categories(self):
        for value in (
            "remove_color_tags( it.info( true ) )",
            "std::string item::info( bool showtext ) const",
            "return info( showtext, dummy );",
            "return info( iteminfo, showtext ? &iteminfo_query::all",
            "return format_item_info( info, {} );",
        ):
            self.assertIn(value, self.item_text + self.item_info)

    def test_full_details_are_computed_only_on_explicit_request(self):
        self.assertIn("announce_full_detail_text", self.item_text)
        self.assertIn("constexpr std::size_t max_chunk = 11000", self.item_text)
        self.assertIn("inline void announce_full_item_info(", self.item_text)
        self.assertIn('{ "focus", "activate", "set_value", "read_detail" }', self.inventory)
        self.assertIn('{ "focus", "activate", "read_detail" }', self.inventory)
        build, handler = self.inventory.split("class inventory_selector::reader_session", 1)[1].split(
            "std::optional<std::string> handle( const a11y::semantic_intent &intent )", 1
        )
        self.assertNotIn("announce_full_item_info(", build)
        self.assertIn('if( intent.action == "read_detail" )', handler)
        self.assertIn("a11y::announce_full_item_info( *live, live.describe( &owner.u ) );", handler)
        self.assertIn('return "TIMEOUT";', handler)

    def test_pocket_item_detail_is_read_only_and_rechecked(self):
        for phrase in (
            'contained.actions = { "focus", "read_detail" };',
            '"focus", "activate", "read_detail"',
            "pocket.container->get_container_pockets()",
            "!pockets[pocket.pocket_index]->sealed()",
            "a11y::announce_full_item_info( *choice->location,",
            "contained_item_bindings.emplace_back( item_node.id, contained )",
        ):
            self.assertIn(phrase, self.inventory)
        self.assertIn("POCKET_CONFIRM_NODE.equals(focus)", self.java)
        self.assertIn("feedback(pocketChoiceSpeech());", self.java)

    def test_advanced_inventory_and_armor_resolve_live_items(self):
        self.assertIn('node.actions = { "focus", "activate", "read_detail" };', self.advanced)
        self.assertIn("a11y::announce_full_item_info( *live,", self.advanced)
        self.assertIn("std::find_if( spane.items.begin(), spane.items.end()", self.advanced)
        self.assertIn('node.actions = { "focus", "activate", "read_detail" };', self.armor)
        self.assertIn("std::find( shown.begin(), shown.end(), binding->worn_it )", self.armor)
        self.assertIn("a11y::announce_full_item_info( **found,", self.armor)

    def test_nearby_item_details_do_not_trigger_travel(self):
        for phrase in (
            'node.actions.push_back( "read_detail" );',
            'intent.action == "read_detail" && target.kind == nearby_kind::item',
            "a11y::scan_unrestricted_tile( target.target )",
            "std::find( visible.items.begin(), visible.items.end()",
            "a11y::announce_full_detail_text( full );",
        ):
            self.assertIn(phrase, self.nearby)
        first = self.nearby.index('if( intent.action == "read_detail" && target.kind')
        read_end = self.nearby.index('return std::string( "TIMEOUT" );', first)
        self.assertLess(read_end, self.nearby.index('g->safe_route_to(', first))

    def test_dedicated_pickup_here_uses_the_original_item_info(self):
        self.assertIn('a11y::screen_scope reader( &ctxt, "pickup-here"', self.pickup_here)
        self.assertIn('node.actions = { "focus", "activate", "read_detail" };', self.pickup_here)
        self.assertIn('node.actions = { "focus", "activate", "set_value", "read_detail" };',
                      self.pickup_here)
        self.assertIn("const std::string information =", self.pickup_here)
        self.assertIn("source.first->info( true )", self.pickup_here)
        self.assertIn("a11y::announce_full_detail_text( complete );", self.pickup_here)

    def test_recipe_detail_includes_complete_output_item_pane(self):
        self.assertIn('node.actions = { "focus", "activate", "read_detail" };', self.crafting)
        self.assertIn('if( intent.action == "read_detail" ) {', self.crafting)
        self.assertIn("const a11y::semantic_snapshot details = reader_snapshot( ctxt );",
                      self.crafting)
        self.assertIn("result_info->get_result_data(", self.crafting)
        self.assertIn("format_item_info( item_data.get_item_display()", self.crafting)
        self.assertIn("a11y::announce_full_detail_text( complete );", self.crafting)

    def test_help_and_diary_original_text_on_two_finger_request(self):
        self.assertIn('node.actions = { "focus", "activate", "read_detail" };', self.help)
        self.assertIn("category.paragraphs", self.help)
        self.assertIn("parse_tags( words, viewer, viewer );", self.help)
        self.assertIn("a11y::announce_full_detail_text( document );", self.help)
        self.assertIn('prefix = "diary:entry";', self.java)
        self.assertIn('a11y::append_document( snapshot, "diary:entry"', self.diary)

    def test_read_detail_allowed_even_if_selection_is_disabled(self):
        self.assertIn('intent->action != "read_detail" && !node->enabled', self.runtime)
        self.assertIn('&& !"read_detail".equals(action)', self.java)
        self.assertIn('if (node.actions.contains("read_detail")) {', self.java)
        self.assertIn('send("read_detail", "") == 0', self.java)
        self.assertIn('if( intent.node.rfind( "pocket-entry:", 0 ) == 0 )', self.inventory)

    def test_utf8_chunks_are_reassembled_before_tts(self):
        for marker in ("A11Y_DETAIL:", "A11Y_DETAIL_BEGIN:",
                       "A11Y_DETAIL_PART:", "A11Y_DETAIL_END:"):
            self.assertIn(marker, self.item_text)
            self.assertIn(marker, self.java)
        self.assertIn("static_cast<unsigned char>( full[end] )", self.item_text)
        self.assertIn("pendingItemDetailParts.append(part)", self.java)
        self.assertIn('announceGameMessage("A11Y_DETAIL:" + assembled, priority, true)', self.java)
        self.assertIn("documentReader.open(snapshot.contextKey(),", self.java)
        self.assertIn("speakDocument(spokenMessage);", self.java)
        self.assertIn('pendingItemDetailNode.equals(focus)', self.java)

    def test_linked_detail_waits_for_native_selected_row(self):
        self.assertIn("private boolean readLinkedDetails(ReaderSnapshot.Node selected)", self.java)
        self.assertIn("!selected.selected || !id.equals(snapshot.focus)", self.java)
        self.assertIn('pendingLinkedDetail.equals(focus)', self.java)
        self.assertIn("ready != null && ready.selected", self.java)
        for scope in ('"crafting"', '"construction"', '"missions"', '"faction"',
                      '"companion_missions"', '"proficiencies"'):
            self.assertIn(scope, self.java)
        self.assertIn('if ("document".equals(node.role) && !node.detail.isEmpty())', self.java)


if __name__ == "__main__":
    unittest.main()
