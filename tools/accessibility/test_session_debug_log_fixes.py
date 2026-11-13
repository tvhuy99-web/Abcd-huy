"""Regression contracts for issues observed in a user's Android session log.

The log itself is NOT committed. These tests cover reusable behavior rather
than a particular character, camp name, inventory item or world coordinate.
Device/TalkBack verification and the native Android build remain separate.
"""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
ANDROID = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class SessionDebugLogFixes(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.overmap = (SRC / "overmap.cpp").read_text("utf-8")
        cls.crafting = (SRC / "crafting_gui.cpp").read_text("utf-8")
        cls.controller = (ANDROID / "GameReaderController.java").read_text("utf-8")
        cls.speech_text = (ANDROID / "ReaderSpokenText.java").read_text("utf-8")
        cls.queue = (ANDROID / "ReaderSpeechQueue.java").read_text("utf-8")
        cls.locale = (ROOT / "lang/po/accessibility/vi.po").read_text("utf-8")

    def test_migrated_camp_is_not_added_again(self):
        migration = self.overmap.split("void overmap::migrate_camps(", 1)[1].split(
            "oter_id overmap::get_or_migrate_oter(", 1
        )[0]
        # Camp registry keys are point_abs_omt, not tripoint_abs_omt.
        # The original 3-D lookup compiled in source-only tests but failed
        # the Android NDK build. Keep a type-safe 2-D lookup before inserting.
        header = (SRC / "overmap.h").read_text("utf-8")
        self.assertIn("std::map<point_abs_omt, basecamp> camps;", header)
        self.assertIn("const tripoint_abs_omt &point : points", migration)
        self.assertIn("camps.find( point.xy() ) != camps.end()", migration)
        self.assertNotIn("camps.find( point )", migration)
        self.assertLess(migration.index("camps.find( point.xy() ) != camps.end()"),
                        migration.index('get_map().add_camp( point, "faction_camp", false );'))
        # Keep genuine conflicting-camp diagnostics intact elsewhere.
        insertion = self.overmap.split("void overmap::add_camp(", 1)[1].split(
            "std::optional<basecamp *> overmap::find_camp(", 1
        )[0]
        self.assertIn('debugmsg( "Tried to add a basecamp', insertion)

    def test_two_finger_item_details_stay_inside_inventory_list(self):
        delivery = self.controller.split("} else if (requestedDetail) {", 1)[1].split(
            "} else if (extendedScan) {", 1
        )[0]
        self.assertIn("speakInlineDetail(spokenMessage);", delivery)
        self.assertNotIn("documentReader.open(", delivery)
        inline = self.controller.split("private void speakInlineDetail(", 1)[1].split(
            "private void speakDocument(", 1
        )[0]
        self.assertIn("ReaderSpeechQueue.LONG_TEXT, 600000L", inline)
        self.assertNotIn("documentReader.open(", inline)
        self.assertIn("public static final int LONG_TEXT = 0;", self.queue)
        self.assertIn("public static final int FOCUS = 2;", self.queue)
        self.assertIn("priority > active.priority", self.queue)

    def test_ascii_armor_silhouette_is_not_spoken_as_numbers(self):
        # Only hash-only visual lines are suppressed. Actual '#123',
        # narrative, percentages, coverage and item stats remain intact.
        self.assertIn("ASCII_ARMOR_DIAGRAM_LINE", self.speech_text)
        self.assertIn(r'(?m)^[ \\t]*(?:#[ \\t]*){2,}\\r?$', self.speech_text)
        self.assertIn('if (cleaned.indexOf(\'#\') >= 0)', self.speech_text)
        self.assertIn("EXCESS_DIAGRAM_GAPS", self.speech_text)
        self.assertIn('return DUPLICATE_STOP.matcher(cleaned).replaceAll(".");',
                      self.speech_text)

    def test_crafting_discriminates_empty_filter_from_no_known_recipes(self):
        self.assertIn("initial_accessible_recipe_check_done", self.crafting)
        self.assertIn('subtab.cur() == "CSC_ALL"', self.crafting)
        self.assertIn("if( current.empty() && available_recipes && available_recipes->size() > 0", self.crafting)
        self.assertIn('diagnostic_event( "CRAFT_RECIPE_SCOPE"', self.crafting)
        self.assertIn("crafter->get_learned_recipes().size()", self.crafting)
        for phrase in (
            "No recipes learned or available to this crafter yet.",
            "No recipes match this filter.",
            "No recipes in this category.",
        ):
            self.assertIn(phrase, self.crafting)
            self.assertRegex(self.locale, r'msgid (?:""\n)?"' + re.escape(phrase))
        self.assertNotIn("learn_recipe(", self.crafting)


if __name__ == "__main__":
    unittest.main()
