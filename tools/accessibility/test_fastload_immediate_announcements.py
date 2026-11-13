"""Regression checks for fast Android loading and never-lost game messages."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = (
    ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
)


class FastLoadAndAnnouncements(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.controller = (JAVA / "GameReaderController.java").read_text(
            encoding="utf-8"
        )
        cls.queue = (JAVA / "ReaderSpeechQueue.java").read_text(
            encoding="utf-8"
        )
        cls.options = (ROOT / "src/options.cpp").read_text("utf-8")
        cls.game = (ROOT / "src/game.cpp").read_text("utf-8")
        cls.init = (ROOT / "src/init.cpp").read_text("utf-8")
        cls.io = (ROOT / "src/game_io.cpp").read_text("utf-8")
        cls.cache = (ROOT / "src/flexbuffer_cache.cpp").read_text("utf-8")
        cls.loading = (ROOT / "src/loading_ui.cpp").read_text("utf-8")

    def test_new_messages_survive_native_snapshot_invalidation(self):
        # The supplied crash log shows SNAPSHOT inactive, then
        # GAME_SPEECH "Nothing to open here", but no TTS_REQUEST.
        self.assertIn("if (!loading && (snapshot == null", self.controller)
        self.assertIn("pendingGameMessages.addLast(", self.controller)
        self.assertIn("speech.setContext(next.contextKey());", self.controller)
        self.assertIn("replayBufferedGameMessages();", self.controller)
        self.assertNotIn("MAX_BUFFERED_MESSAGES = 8", self.controller)
        self.assertIn("pendingGameMessages.addLast(", self.controller)
        self.assertNotIn("pendingGameMessages.removeFirst();\n                    SessionDebugLog.event", self.controller)
        self.assertIn("pendingGameMessages.clear();", self.controller)
        self.assertIn('"GAME_MESSAGE_BUFFERED"', self.controller)
        self.assertIn("orderedGameEvents.addLast(", self.queue)
        self.assertIn("orderedGameEvents.removeFirst()", self.queue)

    def test_new_game_messages_wait_for_each_other_in_arrival_order(self):
        self.assertIn("GAME_EVENT = FOCUS", self.queue)
        self.assertIn("ReaderSpeechQueue.GAME_EVENT", self.controller)
        self.assertIn("messageLevel, 15000", self.controller)
        self.assertIn("ReaderSpeechQueue.URGENT", self.controller)
        self.assertIn("speech.say(\"loading-progress\"", self.controller)
        self.assertIn("!isOrderedGameEvent(active)", self.queue)
        self.assertIn("isOrderedGameEvent(active) || active.context.equals(contextKey)", self.queue)
        self.assertIn("orderedGameEvents.clear();", self.queue)
        self.assertLess(self.queue.index("orderedGameEvents.removeFirst()"),
                        self.queue.index("Request best = null;"))

    def test_skip_verification_is_on_for_android_and_existing_users(self):
        debug_option = self.options.split(
            'add( "SKIP_VERIFICATION"', 1
        )[1].split(");", 1)[0]
        self.assertIn(
            "#if defined(EMSCRIPTEN) || defined(__ANDROID__)",
            debug_option,
        )
        self.assertIn('get_option<bool>( "SKIP_VERIFICATION" )', self.game)
        self.assertIn('.setValue( std::string( "true" ) )', self.game)
        self.assertIn("get_options().save();", self.game)
        self.assertIn(
            'if( !get_option<bool>( "SKIP_VERIFICATION" ) )',
            self.game,
        )

    def test_finalization_and_mods_have_actionable_timings(self):
        self.assertIn('"A11Y_LOAD_GROUP phase=finalize name="', self.init)
        self.assertIn('"A11Y_LOAD_GROUP phase=verify name="', self.init)
        self.assertIn('"A11Y_LOAD_PACK phase=content id="', self.io)
        self.assertIn('"A11Y_LOAD_PACK phase=interaction id="', self.io)
        self.assertIn('"A11Y_LOAD_PHASE name=finalize_and_verify', self.io)
        self.assertIn("mod->version", self.io)
        self.assertIn('"A11Y_JSON_CACHE hits="', self.game)

    def test_mobile_loading_screen_redraw_is_bounded_but_pumps_events(self):
        self.assertIn("last_display_refresh_at", self.loading)
        self.assertIn("std::chrono::milliseconds( 120 )", self.loading)
        self.assertIn("inp_mngr.pump_events();", self.loading)
        self.assertIn("a11y::android::announce(", self.loading)

    def test_parsed_json_cache_checks_version_size_and_source_time(self):
        self.assertIn('version_str != ".v2"', self.cache)
        self.assertIn('".v2.fb"', self.cache)
        self.assertIn("source_mtime != disk_entry->second.mtime", self.cache)
        self.assertIn(
            "source_size != disk_entry->second.source_size", self.cache
        )
        self.assertIn("lexically_relative(", self.cache)
        self.assertIn("cached_flexbuffers_[key]", self.cache)
        self.assertIn("cache_hits.fetch_add", self.cache)
        self.assertIn("cache_misses.fetch_add", self.cache)
        # Only JSON parse products are reused. DynamicDataLoader must still
        # produce fresh gameplay objects for the current world's mod order.
        self.assertIn("load_world_modfiles();", self.game)


if __name__ == "__main__":
    unittest.main()
