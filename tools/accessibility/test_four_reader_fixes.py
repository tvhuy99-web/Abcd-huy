"""Regression coverage for four screen-reader issues reported on device."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
VI = ROOT / "lang/po/accessibility/vi.po"


class ReaderRegressionFixes(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.controller = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        cls.tts = (JAVA / "TtsSettingsActivity.java").read_text(encoding="utf-8")
        cls.world = (SRC / "a11y_world.cpp").read_text(encoding="utf-8")
        cls.input = (SRC / "a11y_input.cpp").read_text(encoding="utf-8")
        cls.serialize = (SRC / "a11y_semantics.cpp").read_text(encoding="utf-8")
        cls.android = (SRC / "a11y_android.cpp").read_text(encoding="utf-8")
        cls.perception = (SRC / "a11y_perception.cpp").read_text(encoding="utf-8")
        cls.item = (SRC / "a11y_item_text.cpp").read_text(encoding="utf-8")
        cls.speech = (JAVA / "ReaderSpeechQueue.java").read_text(encoding="utf-8")
        cls.clean = (JAVA / "ReaderSpokenText.java").read_text(encoding="utf-8")

    def test_summary_up_opens_engine_confirm_without_focus_dependency(self):
        code = self.controller.split("public void confirm()", 1)[1].split(
            "public boolean movementPracticeEnabled()", 1)[0]
        self.assertIn('"CHARACTER_CREATION".equals(snapshot.category)', code)
        self.assertIn('snapshot.find("NEXT_TAB")', code)
        self.assertIn('send("activate", "")', code)
        creator = (SRC / "newcharacter.cpp").read_text(encoding="utf-8")
        self.assertIn('if( cc_uistate.selected_tab == CHARCREATOR_SUMMARY )', creator)
        self.assertIn('_( "Are you finished creating your character?" )', creator)

    def test_voice_preview_optional(self):
        save = self.tts.split("private void saveSettings()", 1)[1].split(
            "private boolean usesTalkBack()", 1)[0]
        self.assertNotIn('previewPassed', save)
        self.assertNotIn('a11yTtsTestFirst', save)
        self.assertIn('saveTtsSettings(enabled.isChecked()', save)
        self.assertIn('private void testVoice()', self.tts)

    def test_hub_automation_is_published_from_live_native_state(self):
        for action in ("toggle_auto_features", "toggle_auto_mining", "toggle_auto_pickup",
                       "toggle_auto_pulp_butcher", "toggle_auto_foraging", "autosafe",
                       "safemode", "toggle_thief_mode", "autoattack"):
            self.assertIn('id == "' + action + '"', self.input)
        self.assertIn('get_option<bool>( setting )', self.input)
        self.assertIn('option.getValueName()', self.input)
        self.assertIn('get_option<bool>( "AUTO_FEATURES" )', self.input)
        self.assertIn('_( "Currently on" )', self.input)
        self.assertIn('_( "Currently off" )', self.input)
        self.assertIn('This is not an on/off setting.', self.input)
        self.assertNotIn('node.id == "toggle_auto_pickup"', self.world)

    def test_spoken_markup_removed_centrally_without_mutating_edit_data(self):
        self.assertIn('quote_spoken( node.label )', self.serialize)
        self.assertIn('quote_spoken( node.detail )', self.serialize)
        self.assertIn('quote_spoken( snapshot.summary )', self.serialize)
        self.assertIn('!node.field_type.empty() ?', self.serialize)
        self.assertIn('quote( node.value ) : quote_spoken( node.value )', self.serialize)
        self.assertIn('const std::string spoken = remove_color_tags( text );', self.android)
        self.assertIn('ReaderSpokenText.clean(text)', self.controller)
        self.assertIn('ReaderSpokenText.clean(text)', self.speech)
        self.assertIn('COLOR_TAG.matcher(text).replaceAll("")', self.clean)

    def test_item_symbols_are_replaced_with_object_conditions(self):
        self.assertIn('it.tname( 1, false )', self.item)
        self.assertIn('it.damage_level()', self.item)
        self.assertIn('it.burnt > 0', self.item)
        self.assertIn('spoken_item_name( value )', self.perception)
        self.assertIn('spoken_item_name( obj )', self.world)
        po = VI.read_text(encoding="utf-8")
        self.assertIn('msgstr "Mức hư hỏng: %d"', po)
        self.assertIn('msgstr "Đang bật"', po)


if __name__ == "__main__":
    unittest.main()
