"""Keep menu content in front of actions and full details on explicit request."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CONTROLLER = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/GameReaderController.java"
CRAFTING = ROOT / "src/crafting_gui.cpp"
DISASSEMBLE = ROOT / "src/crafting.cpp"
TRANSLATIONS = ROOT / "lang/po/accessibility/vi.po"


class CraftingReaderFirstContentTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.controller = CONTROLLER.read_text(encoding="utf-8")
        cls.crafting = CRAFTING.read_text(encoding="utf-8")
        cls.disassemble = DISASSEMBLE.read_text(encoding="utf-8")
        cls.po = TRANSLATIONS.read_text(encoding="utf-8")

    def test_first_reader_snapshot_builds_recipe_list_before_publishing(self):
        body = self.crafting.split(
            "a11y::semantic_snapshot crafting_ui_impl::reader_snapshot(", 1
        )[1].split("a11y::semantic_snapshot documents;", 1)[0]
        self.assertIn("if( recalc && !crafting_cats.empty() )", body)
        self.assertLess(body.index("recalculate_recipes();"),
                        body.index('a11y::command_snapshot( ctxt, _( "Crafting" ) )'))
        self.assertIn('snapshot.focus = "craft:no-recipes";', body)
        self.assertIn('node.id = "recipe:"', body)

    def test_no_visual_panel_required_for_recipe_description(self):
        self.assertIn("description = rec.get_description( *crafter );", self.crafting)
        self.assertIn("description = result.variant_description();", self.crafting)
        self.assertIn('a11y::append_document( documents, "craft:description"', self.crafting)

    def test_commands_only_on_long_press_for_primary_content_screens(self):
        code = self.controller
        self.assertIn("if (commandPaletteOpen) { return contextualCommands(); }", code)
        self.assertIn("if (containsContent) {", code)
        for category in ("crafting", "construction", "inventory", "zones", "surroundings"):
            self.assertIn('"' + category + '".equals(snapshot.category)', code)
        self.assertIn("result.clear();", code)
        self.assertIn("listener.openContextActions();", (
            ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/ReaderGestureSurface.java"
        ).read_text(encoding="utf-8"))

    def test_two_finger_linked_details_reads_all_sections(self):
        linked = self.controller.split("private boolean readLinkedDetails(", 1)[1].split(
            "@Override\n    public void detail()", 1
        )[0]
        self.assertIn('prefix = "craft:";', linked)
        self.assertIn('prefix = "construction:";', linked)
        # Two fingers still read every native section, but no longer
        # hijack list swipes with an implicit paragraph-reader screen.
        self.assertIn("speakInlineDetail(full.toString());", linked)
        self.assertNotIn("documentReader.open(", linked)
        self.assertIn("speakInlineDetail(information);", self.controller)
        self.assertIn("ReaderSpeechQueue.LONG_TEXT, 600000L", self.controller)
        self.assertIn("private void speakDocument(String text)", self.controller)
        self.assertIn('full.insert(0, nodeText(selected, true) + "\\n\\n");', self.controller)

    def test_disassembly_light_reason_is_accurate_and_translated(self):
        self.assertIn("Too dark to disassemble this item.", self.disassemble)
        self.assertIn("lighting_craft_speed_multiplier( r ) <= 0.0f", self.disassemble)
        self.assertIn('msgstr "Quá tối để tháo rã vật phẩm này.', self.po)


if __name__ == "__main__":
    unittest.main()
