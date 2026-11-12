"""Static regression checks for full two-finger detail, without an APK build.

These tests guard the native/Android read-only intent contract. They do NOT
replace an instrumented screen-reader test with a real inventory or mod.
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
        cls.runtime = (SRC / "a11y_runtime.cpp").read_text("utf-8")
        cls.java = JAVA.read_text("utf-8")

    def test_native_item_info_is_complete_and_demand_driven(self):
        self.assertIn("remove_color_tags( it.info( true ) )", self.item_text)
        self.assertIn('android::announce( "A11Y_DETAIL:"', self.item_text)
        self.assertIn("constexpr std::size_t max_chunk = 11000", self.item_text)
        self.assertIn("std::string item::info( bool showtext ) const", self.item_info)
        self.assertIn("return info( showtext, dummy );", self.item_info)
        self.assertIn("return info( iteminfo, showtext ? &iteminfo_query::all", self.item_info)
        self.assertIn("return format_item_info( info, {} );", self.item_info)

    def test_inventory_every_selector_exposes_details_without_eager_building(self):
        self.assertIn('{ "focus", "activate", "set_value", "read_detail" }', self.inventory)
        self.assertIn('{ "focus", "activate", "read_detail" }', self.inventory)
        build, handler = self.inventory.split("class inventory_selector::reader_session", 1)[1].split(
            "std::optional<std::string> handle( const a11y::semantic_intent &intent )", 1
        )
        self.assertNotIn("announce_full_item_info", build)
        self.assertIn('if( intent.action == "read_detail" )', handler)
        self.assertIn("a11y::announce_full_item_info( *live );", handler)
        self.assertIn('return "TIMEOUT";', handler)

    def test_pocket_contents_are_resolved_without_selection(self):
        self.assertIn('contained.actions = { "focus", "read_detail" };', self.inventory)
        self.assertIn('"focus", "activate", "read_detail"', self.inventory)
        self.assertIn("pocket.container->get_container_pockets()", self.inventory)
        self.assertIn("!pockets[pocket.pocket_index]->sealed()", self.inventory)
        self.assertIn("a11y::announce_full_item_info( *choice->location );", self.inventory)
        self.assertIn("contained_item_bindings.emplace_back( item_node.id, contained )", self.inventory)

    def test_advanced_inventory_and_armor_use_validated_live_objects(self):
        self.assertIn('node.actions = { "focus", "activate", "read_detail" };', self.advanced)
        self.assertIn("a11y::announce_full_item_info( *live );", self.advanced)
        self.assertIn("std::find_if( spane.items.begin(), spane.items.end()", self.advanced)
        self.assertIn('node.actions = { "focus", "activate", "read_detail" };', self.armor)
        self.assertIn("std::find( shown.begin(), shown.end(), binding->worn_it )", self.armor)
        self.assertIn("a11y::announce_full_item_info( **found );", self.armor)

    def test_nearby_items_read_the_real_visible_stack(self):
        self.assertIn('#include "a11y_item_text.h"', self.nearby)
        self.assertIn('node.actions.push_back( "read_detail" );', self.nearby)
        self.assertIn('intent.action == "read_detail" && target.kind == nearby_kind::item', self.nearby)
        self.assertIn('a11y::scan_unrestricted_tile( target.target )', self.nearby)
        self.assertIn('std::find( visible.items.begin(), visible.items.end()', self.nearby)
        self.assertIn('a11y::announce_full_item_info( obj );', self.nearby)
        self.assertIn('return std::string( "TIMEOUT" );', self.nearby)

    def test_read_only_detail_does_not_need_item_to_be_selectable(self):
        self.assertIn('intent->action != "read_detail" && !node->enabled', self.runtime)
        self.assertIn('&& !"read_detail".equals(action)', self.java)
        self.assertIn('if (node.actions.contains("read_detail")) {', self.java)
        self.assertIn('send("read_detail", "");', self.java)
        self.assertIn('if (intent.node.rfind( "pocket-entry:", 0 ) == 0)', self.inventory)

    def test_long_speech_and_linked_native_documents(self):
        self.assertIn('message.startsWith("A11Y_DETAIL:")', self.java)
        self.assertIn('"item-detail:" + ANNOUNCEMENT_SEQUENCE.incrementAndGet()', self.java)
        self.assertIn("ReaderSpeechQueue.LONG_TEXT, 600000L", self.java)
        self.assertIn('ReaderSpeechQueue.LONG_TEXT', self.java)
        self.assertIn('private boolean readLinkedDetails(ReaderSnapshot.Node selected)', self.java)
        for scope in (
            '"crafting"', '"construction"', '"missions"', '"faction"',
            '"companion_missions"', '"proficiencies"',
        ):
            self.assertIn(scope, self.java)
        self.assertIn("documentReader.open(snapshot.contextKey(),", self.java)
        self.assertIn('if ("document".equals(node.role) && !node.detail.isEmpty())', self.java)


if __name__ == "__main__":
    unittest.main()
