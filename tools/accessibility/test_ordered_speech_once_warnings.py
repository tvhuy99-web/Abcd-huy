"""Source-level contracts for urgent combat speech and one-time activity warnings."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class OrderedSpeechAndWarningsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.queue = (JAVA / "ReaderSpeechQueue.java").read_text("utf-8")
        cls.controller = (JAVA / "GameReaderController.java").read_text("utf-8")
        cls.world = (ROOT / "src/a11y_world.cpp").read_text("utf-8")
        cls.game = (ROOT / "src/game.cpp").read_text("utf-8")
        cls.output = (ROOT / "src/output.cpp").read_text("utf-8")
        cls.popup = (ROOT / "src/popup.cpp").read_text("utf-8")
        cls.activity = (ROOT / "src/activity_actor.cpp").read_text("utf-8")
        cls.actor = (ROOT / "src/player_activity.cpp").read_text("utf-8")
        cls.po = (ROOT / "lang/po/accessibility/vi.po").read_text("utf-8")

    def test_urgent_game_warnings_go_first_without_preempting_active_speech(self):
        q = self.queue
        assert "GAME_EVENT_CONTEXT" in q
        assert 'key.startsWith("game-message:")' in q
        assert "orderedGameEvents.addLast(" in q
        assert "urgentGameEvents.addLast(request)" in q
        assert "urgentGameEvents.removeFirst()" in q
        assert "orderedGameEvents.removeFirst()" in q
        assert "if (active != null && !isOrderedGameEvent(active)" in q
        assert "|| active.context.equals(contextKey)" in q
        assert "orderedGameEvents.clear();" in q
        assert "urgentGameEvents.clear();" in q
        assert 'old.text.equals(text)' in q
        assert q.index("urgentGameEvents.removeFirst()") < q.index("Request best = null;")
        assert q.index('key.startsWith("game-message:")') < q.index("if (priority != FOCUS)")

    def test_screen_context_and_temporary_snapshot_never_discard_game_message(self):
        q = self.queue.split("public void setContext(", 1)[1].split("public void focus(", 1)[0]
        self.assertIn("!isOrderedGameEvent(active)", q)
        self.assertNotIn("stop();", q)
        c = self.controller
        self.assertIn("pendingGameMessages.addLast", c)
        self.assertIn("replayBufferedGameMessages", c)
        self.assertIn("announceGameMessage(buffered.text, buffered.priority, true);", c)
        self.assertIn("deliver.run();", c)
        self.assertNotIn("MAX_BUFFERED_MESSAGES = 8", c)
        self.assertNotIn("SystemClock.elapsedRealtime() - buffered.created < 15000", c)

    def test_world_enemy_report_omits_only_repeated_distance_keeps_hp(self):
        body = self.world.split('std::string enemy_report;', 1)[1].split(
            'result.nodes.push_back( document( "reader:combat:enemies"', 1)[0]
        self.assertIn('_( "%1$s, %2$s, %3$d HP remaining." )', body)
        self.assertIn("relative_name( relative.raw().xy() )", body)
        self.assertIn("std::max( 0, enemy->get_hp() )", body)
        self.assertNotIn("enemy->hp_percentage()", body)
        self.assertNotIn("distance %3$d tiles", body)
        self.assertIn('msgstr "%1$s, %2$s, còn %3$d máu."', self.po)

    def test_declined_distraction_suppressed_per_activity_and_carried_across_reload(self):
        self.assertIn('if( action == "NO" || action == "IGNORE" )', self.game)
        self.assertIn("u.activity.ignore_distraction( type );", self.game)
        self.assertIn("earlier_activity", self.activity)
        self.assertIn("who.activity.inherit_distractions( *earlier_activity );", self.activity)
        self.assertIn("dismissed_prompts.insert( other.dismissed_prompts.begin()", self.actor)

    def test_same_question_only_suppressed_for_same_running_task(self):
        self.assertIn("if( a11y::enabled() && u.activity.is_prompt_dismissed( prompt ) )", self.game)
        self.assertIn("u.activity.dismiss_prompt( prompt );", self.game)
        self.assertIn("get_avatar().activity.is_prompt_dismissed( text )", self.output)
        self.assertIn('if( action == "NO" && in_reader_activity', self.output)
        self.assertIn("query_ynq_result::no", self.output)
        self.assertIn("a11y::enabled() && g != nullptr && get_avatar().activity", self.output)
        self.assertIn('option.action == "NO"', self.popup)
        self.assertIn('"popup:" + category + ":"', self.popup)
        self.assertIn('return { false, "NO", {} };', self.popup)
        self.assertIn('get_avatar().activity.dismiss_prompt( warning_key );', self.popup)

    def test_one_spoken_reload_update_with_round_count(self):
        self.assertIn("Loaded %1$d %2$s into %3$s. %4$d rounds in gun.", self.activity)
        self.assertNotIn('add_msg( m_info, _( "Reload progress:', self.activity)
        self.assertIn('msgstr "Đã nạp %1$d viên %2$s vào %3$s. Súng có %4$d viên."', self.po)
        self.assertIn('msgstr "%s đã hết đạn!"', self.po)


if __name__ == "__main__":
    unittest.main()
