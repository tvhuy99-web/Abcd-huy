"""Guard newest-first spoken messages and split world-mod load timing."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = (ROOT / "android/app/src/main/java"
        / "com/cleverraven/cataclysmdda")


class MessageRecencyAndLoadTiming(unittest.TestCase):
    def test_two_finger_double_tap_reads_recent_messages_newest_first(self):
        controller = (JAVA / "GameReaderController.java").read_text("utf-8")
        world = (SRC / "a11y_world.cpp").read_text("utf-8")
        messages = (SRC / "messages.cpp").read_text("utf-8")
        self.assertIn("public void twoFingerTap(int count)", controller)
        self.assertIn('"world:status" : "world:messages"', controller)
        self.assertIn("Messages::recent_messages( 12 )", world)
        self.assertIn("messages.rbegin(); it != messages.rend(); ++it", world)
        self.assertIn("it->first + \": \" + it->second", world)
        # The source message storage remains chronological; reverse only
        # the spoken presentation rather than changing other game clients.
        self.assertIn("begin( messages ) + offset, end( messages )", messages)

    def test_two_finger_double_tap_reads_recent_messages_on_any_screen(self):
        controller = (JAVA / "GameReaderController.java").read_text("utf-8")
        surface = (JAVA / "ReaderGestureSurface.java").read_text("utf-8")
        # The surface forwards the double-tap outside the live world too;
        # only the triple tap (action hub) stays world-gated.
        self.assertIn(
            "if (count == 1 || count == 2 || listener.directWorldGesturesEnabled())",
            surface)
        # The controller re-reads recent messages without the world gate
        # and falls back to the last world snapshot that published them,
        # because menu/activity snapshots do not carry a world:messages
        # node (only world_snapshot() in src/a11y_world.cpp does).
        self.assertIn("private String lastRecentMessages", controller)
        self.assertIn("lastRecentMessages = recentMessages.detail;", controller)
        body = controller.split("public void twoFingerTap(", 1)[1]
        body = body.split("public void threeFingerTap()", 1)[0]
        self.assertIn("if (count == 2) {", body)
        self.assertIn("spoken = lastRecentMessages;", body)
        # The world gate still stands between the double-tap branch and
        # the triple-tap action hub, and a single tap still reads detail.
        self.assertIn("if (count == 1) { detail(); return; }", body)
        self.assertIn("if (!directWorldGesturesEnabled()) { return; }", body)
        self.assertIn("if (count == 3) { openActionMenu(); return; }", body)

    def test_android_load_metrics_identify_check_vs_load_cost(self):
        game = (SRC / "game.cpp").read_text("utf-8")
        self.assertIn("check_mods_list( world_generator->active_world )", game)
        self.assertIn("const auto mods_checked_at =", game)
        self.assertIn("load_world_modfiles();", game)
        self.assertIn('" mods_check_ms="', game)
        self.assertIn('" mods_load_ms="', game)
        self.assertIn('" mods_ms="', game)
        self.assertIn("mods_loaded_at - mods_checked_at", game)


if __name__ == "__main__":
    unittest.main()
