"""Regression guards for character-creator summary and Android TalkBack input."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
RES = ROOT / "android/app/src/main/res"


class CharacterCreatorSummaryInput(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.creator = (SRC / "newcharacter.cpp").read_text(encoding="utf-8")
        cls.calendar = (SRC / "calendar_ui.cpp").read_text(encoding="utf-8")
        cls.popup = (SRC / "input_popup.cpp").read_text(encoding="utf-8")
        cls.controller = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        cls.gestures = (JAVA / "ReaderGestureSurface.java").read_text(encoding="utf-8")
        cls.native = (JAVA / "NativeUI.java").read_text(encoding="utf-8")

    def test_summary_direct_edits_and_other_commands_on_hold(self):
        self.assertIn('const std::set<std::string> direct_controls', self.creator)
        for action in ("CHANGE_NAME", "CHANGE_AGE", "CHANGE_HEIGHT",
                       "CHANGE_GENDER", "CHANGE_OUTFIT", "CHANGE_BLOOD_TYPE",
                       "CHOOSE_LOCATION", "CHOOSE_CITY",
                       "CHANGE_START_OF_CATACLYSM", "CHANGE_START_OF_GAME"):
            self.assertIn('"' + action + '"', self.creator)
        self.assertIn('node.role = "button";', self.creator)
        self.assertIn('node.role == "button" || node.role == "document"', self.creator)
        self.assertIn('_( "Summary. Step 7 of 7. Points remaining: %d." )', self.creator)
        self.assertIn('"CHARACTER_CREATION".equals(next.category)', self.controller)

    def test_gender_outfit_and_blood_use_single_choice_lists(self):
        self.assertIn('choices.title = _( "Gender" );', self.creator)
        self.assertIn('choices.title = _( "Outfit" );', self.creator)
        self.assertIn('choices.title = _( "Blood type" );', self.creator)
        self.assertIn('choices.query();', self.creator)
        self.assertIn('if( choices.ret != UILIST_CANCEL && choices.ret != UILIST_ERROR )', self.creator)

    def test_day_hour_gestures_scoped_to_creator_calendar(self):
        self.assertIn('smenu.input_category = simple_reader ? "character-creator-calendar"', self.calendar)
        self.assertIn('smenu.additional_actions.emplace_back( "CALENDAR_DONE"', self.calendar)
        self.assertIn('const int selected_field = concise ?', self.calendar)
        self.assertIn('return initial_value;', self.calendar)
        self.assertIn('handleCreatorCalendarSwipe(int horizontal, int vertical)', self.controller)
        self.assertIn('creatorCalendarAction("CALENDAR_DONE")', self.controller)
        self.assertIn('"CHARACTER_CREATION".equals(snapshot.category)', self.controller)
        self.assertIn('ReaderSnapshot.Node previousTab = snapshot.find("PREV_TAB");', self.controller)
        self.assertIn('listener.handleCreatorCalendarSwipe(horizontal, vertical)', self.gestures)

    def test_reusable_text_and_number_android_editor(self):
        self.assertIn('public String inputText(', self.native)
        self.assertIn('final EditText field = new EditText(activity);', self.native)
        self.assertIn('setNegativeButton(R.string.cancel', self.native)
        self.assertIn('setPositiveButton(R.string.ok', self.native)
        self.assertIn('field.setError(', self.native)
        self.assertIn('if (completed.compareAndSet(false, true))', self.native)
        self.assertIn('android_accessible_input( title, text, 0, max_input_length', self.popup)
        self.assertIn('std::is_integral_v<T> ? 1 : 2', self.popup)
        self.assertIn('void number_input_popup<T>::set_value_range', self.popup)
        self.assertIn('manual.set_value_range( min_value, maximum_affordable );', self.creator)
        self.assertIn('age_query.set_value_range( CHARACTER_AGE_MIN, CHARACTER_AGE_MAX );', self.creator)
        self.assertIn('height_query.set_value_range( min_allowed_height, max_allowed_height );', self.creator)
        editor = (JAVA / "ReaderTextEditor.java").read_text(encoding="utf-8")
        self.assertIn('this.nativeInputOnly = !this.quantityChoices;', editor)
        self.assertIn('add(R.string.ok, this::submitCurrent);', editor)
        self.assertIn('add(R.string.cancel, listener::cancel);', editor)
        self.assertIn('R.string.a11yNativeInputInstructions', editor)

    def test_no_accessibility_service_toggle(self):
        self.assertNotIn('WRITE_SECURE_SETTINGS', self.native)
        self.assertNotIn('setAccessibilityServiceEnabled', self.native)
        for locale in ("values", "values-vi"):
            xml = (RES / locale / "strings.xml").read_text(encoding="utf-8")
            self.assertIn('name="a11yNativeInputRangeError"', xml)


if __name__ == "__main__":
    unittest.main()
