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

    def test_double_tapped_no_dispatches_no_not_generic_confirm(self):
        popup = (SRC / "popup.cpp").read_text(encoding="utf-8")
        option_handler = popup.split('a11y::screen_scope reader( &ctxt, category,', 1)[1]
        option_handler = option_handler.split('a11y::semantic_snapshot reader_snapshot;', 1)[0]
        self.assertIn('intent.node == "option:" + std::to_string( i )', option_handler)
        self.assertIn('category == "YESNO" || category == "YESNOQUIT"', option_handler)
        self.assertIn('return options[i].action;', option_handler)
        self.assertLess(option_handler.index('return options[i].action;'),
                        option_handler.index('return intent.action == "activate" ? "CONFIRM"'))
        # The popup, not the Android two-finger confirmation path, resolves
        # a direct one-finger activation into the exact native YES/NO result.
        self.assertIn('if( res.action == options[ind].action )', popup)
        self.assertIn('res.wait_input = false;', popup)
        output = (SRC / "output.cpp").read_text(encoding="utf-8")
        self.assertIn('return action == "YES";', output)

    def test_native_no_keeps_character_creator_on_summary_step(self):
        creator = (SRC / "newcharacter.cpp").read_text(encoding="utf-8")
        summary = creator.split('if( cc_uistate.selected_tab == CHARCREATOR_SUMMARY ) {', 1)[1]
        summary = summary.split('} else if( action == "SCROLL_INFOBOX_UP" )', 1)[0]
        self.assertIn('if( query_yn( query ) ) {', summary)
        self.assertIn('cc_uistate.finished_character_creator = true;', summary)
        surface = (JAVA / "ReaderGestureSurface.java").read_text(encoding="utf-8")
        self.assertIn('listener.activate();', surface)
        self.assertIn('listener.confirm();', surface)  # Two-finger up is unchanged.
        activation = self.controller.split('public void activate()', 1)[1]
        activation = activation.split('private void openEditor(', 1)[0]
        self.assertIn('send("activate", "");', activation)

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
        self.assertIn('node.value = _( "One attack, not an on/off setting" );', self.input)
        self.assertIn("even when unarmed. With no target, you may wait one turn", self.input)
        self.assertIn('This is not a toggle.', self.input)
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
