"""Regression guards for the six reader-friendly custom character creation steps."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "src" / "newcharacter.cpp"
UILIST = ROOT / "src" / "uilist.cpp"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/GameReaderController.java"
VI = ROOT / "lang/po/accessibility/vi.po"


class CharacterCreatorSixSteps(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = SOURCE.read_text(encoding="utf-8")
        cls.uilist = UILIST.read_text(encoding="utf-8")
        cls.java = JAVA.read_text(encoding="utf-8")
        cls.vi = VI.read_text(encoding="utf-8")

    def test_step_titles_and_available_points(self):
        self.assertIn('"character-creator-step-"', self.source)
        self.assertIn('_( "%1$s. Step %2$d of 7. Points remaining: %3$d." )', self.source)
        self.assertIn('multi_pool( u ).skill_points_left', self.source)
        for phrase in ("Select scenario", "Select profession", "Select background",
                       "Set attributes", "Select traits", "Set skills"):
            self.assertIn('_( "' + phrase + '" )', self.source)

    def test_first_two_single_choice_steps_advance_only_on_confirm(self):
        callback = self.source.split("void character_creator_callback::confirm( uilist *menu )", 1)[1]
        for step in ("CHARCREATOR_SCENARIO", "CHARCREATOR_PROFESSION"):
            body = callback.split("case " + step + ":", 1)[1].split("break;", 1)[0]
            self.assertIn('if( a11y::enabled() && cc_uistate.generation_type == character_type::CUSTOM )', body)
            self.assertIn('ui_parent->handle_action( "NEXT_TAB" );', body)
            self.assertIn('return;', body)  # Invalid selections leave the step unchanged.
        self.assertIn('if( a11y::enabled() )', callback)
        self.assertIn('_( "Selected %s." )', callback)
        self.assertIn('_( "Unselected %s." )', callback)

    def test_two_finger_navigation_is_scoped_to_creator_steps(self):
        self.assertIn('"character-creator-step-3".equals(snapshot.category)', self.java)
        self.assertIn('"character-creator-step-4".equals(snapshot.category)', self.java)
        self.assertIn('"character-creator-step-5".equals(snapshot.category)', self.java)
        self.assertIn('"character-creator-step-6".equals(snapshot.category)', self.java)
        self.assertIn('isCreatorMultiChoiceStep() && creatorStepAction("NEXT_TAB")', self.java)
        self.assertIn('if (creatorStepAction("PREV_TAB")) { return; }', self.java)
        self.assertIn('next.category.startsWith("character-creator-step-")', self.java)

    def test_attributes_and_skills_have_retryable_value_picker(self):
        self.assertIn('choices.input_category = "character-creator-value";', self.source)
        self.assertIn('value <= 10', self.source)
        self.assertIn('choices.addentry( 11, true, -1, _( "Enter manually" ) );', self.source)
        self.assertIn('if( candidate < min_value || candidate > max_value )', self.source)
        self.assertIn('if( const std::optional<std::string> error = validate( candidate ) )', self.source)
        self.assertIn('popup( *error );\n            continue;', self.source)
        self.assertIn('Not enough points for this attribute.', self.source)
        self.assertIn('Not enough points for this skill.', self.source)
        self.assertIn(
            'skill_queried->name(), previous_skill_level,\n'
            '                                                      MIN_SKILL, MAX_SKILL, true,',
            self.source
        )

    def test_single_trait_variant_returns_to_trait_step(self):
        self.assertIn('menu.input_category = "character-creator-variant";', self.source)
        self.assertIn('if( a11y::enabled() && menu.ret >= 0 && menu.ret < idx )', self.source)
        self.assertIn('if( menu.ret == UILIST_CANCEL || menu.ret == UILIST_ERROR )', self.source)
        self.assertIn(
            'const bool creator_step = input_category.rfind( "character-creator-step-", 0 ) == 0',
            self.uilist
        )
        self.assertIn(
            'node.enabled = allow_confirm && ( entry.enabled || ( allow_disabled && !creator_step ) );',
            self.uilist
        )

    def test_summary_logic_is_unmodified_by_new_gestures(self):
        self.assertIn('if( cc_uistate.selected_tab == CHARCREATOR_SUMMARY )', self.source)
        self.assertNotIn('"character-creator-step-7"', self.source)
        self.assertNotIn('"character-creator-step-7"', self.java)

    def test_vietnamese_step_prompts(self):
        self.assertIn('msgstr "%1$s. Bước %2$d trên 7. Điểm còn lại: %3$d."', self.vi)
        self.assertIn('msgstr "Nhập thủ công"', self.vi)
        self.assertIn('msgstr "Không đủ điểm để tăng thuộc tính này."', self.vi)


if __name__ == "__main__":
    unittest.main()
