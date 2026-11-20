"""Character creation should prioritize point totals over mod spell chatter."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
CREATOR = ROOT / "src/newcharacter.cpp"
RUNTIME_HEADER = ROOT / "src/a11y_runtime.h"
RUNTIME = ROOT / "src/a11y_runtime.cpp"
JAVA = (ROOT / "android/app/src/main/java/com/cleverraven"
        "/cataclysmdda/GameReaderController.java")
VI = ROOT / "lang/po/accessibility/vi.po"


class CharacterCreationSpeechTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.creator = CREATOR.read_text("utf-8")
        cls.header = RUNTIME_HEADER.read_text("utf-8")
        cls.runtime = RUNTIME.read_text("utf-8")
        cls.java = JAVA.read_text("utf-8")
        cls.vi = VI.read_text("utf-8")

    def test_preview_scope_only_mutes_native_gameplay_messages(self):
        self.assertIn("const a11y::character_creation_speech_scope creator_speech;",
                      self.creator)
        self.assertIn("class character_creation_speech_scope", self.header)
        self.assertIn("++character_creation_speech_depth;", self.runtime)
        self.assertIn("--character_creation_speech_depth;", self.runtime)
        f = self.runtime.split("void announce_game_message(", 1)[1]
        self.assertLess(f.index("character_creation_speech_depth > 0"),
                        f.index("android::announce( text, urgent ? 3 : 1 );"))
        self.assertIn("a11y::android::announce(", self.creator)

    def test_buffered_loading_chatter_does_not_replay_inside_creator(self):
        section = self.java.split("private void replayBufferedGameMessages()", 1)[1]
        section = section.split("private final AccessibilityManager.", 1)[0]
        self.assertIn('"CHARACTER_CREATION".equals(snapshot.category)', section)
        self.assertIn('snapshot.category.startsWith("character-creator-step-")',
                      section)
        self.assertIn('pendingGameMessages.clear();', section)
        self.assertIn('announceGameMessage(buffered.text, buffered.priority, true);',
                      section)

    def test_selection_feedback_speaks_tab_specific_remaining_points(self):
        self.assertIn('_( "Points remaining: %d." )', self.creator)
        self.assertGreaterEqual(
            self.creator.count("creator_change_with_remaining("), 7)
        for pool in ("stat_points_left", "trait_points_left",
                     "skill_points_left"):
            self.assertIn("multi_pool( u )." + pool, self.creator)
        self.assertIn('msgstr "Điểm còn lại: %d."', self.vi)

    def test_native_character_creation_points_and_steps_stay_intact(self):
        self.assertIn('_( "%1$s. Step %2$d of 7. Points remaining: %3$d." )',
                      self.creator)
        self.assertIn('if( cc_uistate.selected_tab == CHARCREATOR_SUMMARY )',
                      self.creator)


if __name__ == "__main__":
    unittest.main()
