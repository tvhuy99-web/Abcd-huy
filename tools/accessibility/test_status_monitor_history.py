"""Guard status monitor warnings being re-readable from message history.

announce_status_changes() (src/a11y_world.cpp) speaks hunger / thirst /
pain / bleeding / broken / bitten / infected transitions live through
a11y::announce_game_message, but that direct path never writes to the
game message log, so the two-finger double-tap recap (which only reads
Messages::recent_messages(12) via the world:messages node) could never
re-read a warning the player missed.

The fix records the same warning in the log with the new log-only
Messages::add_msg_silent(), keeping the direct live announcement
untouched. Two constraints pin the shape:

1. NO DOUBLE SPEECH: a plain Messages::add_msg() here would speak the
   warning a second time through announce_message(), so the monitor must
   never call it -- only the silent variant, whose implementation
   discards the return value that announce_message() would speak.
2. URGENT WARNINGS MUST STILL BE HEARD IN COMBAT: announce_message()
   returns early while a mute_combat_speech guard is live
   (src/messages.cpp), so routing the monitor through the normal channel
   could swallow urgent warnings. The direct announce_game_message()
   call does not consult combat mute at all and stays the live path.

Only the monitor's warnings go through the silent write; no other
message is affected.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


def monitor_body():
    """announce_status_changes() in a11y_world.cpp."""
    text = (SRC / "a11y_world.cpp").read_text("utf-8")
    body = text.split("void announce_status_changes()", 1)[1]
    return body.split("\nsemantic_snapshot world_snapshot(", 1)[0]


def announce_message_body():
    """announce_message() in messages.cpp."""
    text = (SRC / "messages.cpp").read_text("utf-8")
    body = text.split(
        "void announce_message( const std::optional<game_message> &message )", 1
    )[1]
    return body.split("\nbool message_exceeds_ttl", 1)[0]


def add_msg_silent_body():
    """Messages::add_msg_silent() in messages.cpp."""
    text = (SRC / "messages.cpp").read_text("utf-8")
    body = text.split("void Messages::add_msg_silent(", 1)[1]
    return body.split("\nvoid Messages::clear_messages()", 1)[0]


class StatusMonitorHistory(unittest.TestCase):
    def test_silent_log_write_exists(self):
        header = (SRC / "messages.h").read_text("utf-8")
        self.assertIn(
            "void add_msg_silent( const game_message_params &params, std::string msg );",
            header,
        )
        impl = (SRC / "messages.cpp").read_text("utf-8")
        self.assertIn("void Messages::add_msg_silent(", impl)

    def test_silent_write_never_speaks(self):
        body = add_msg_silent_body()

        self.assertIn("player_messages.add_msg_string( std::move( msg ), params )", body)
        self.assertNotIn("announce_message", body)

    def test_plain_add_msg_still_speaks(self):

        impl = (SRC / "messages.cpp").read_text("utf-8")
        self.assertIn(
            "announce_message( player_messages.add_msg_string( std::move( msg ) ) );",
            impl,
        )
        self.assertIn(
            "announce_message( player_messages.add_msg_string( std::move( msg ), params ) );",
            impl,
        )

    def test_normal_channel_filters_debug_and_combat_mute(self):
        body = announce_message_body()
        self.assertIn("message->type == m_debug", body)
        self.assertIn("muted_combat_speech_depth > 0", body)

        self.assertIn("message->type == m_bad", body)
        self.assertIn("message->type == m_warning", body)
        self.assertIn("message->type == m_critical", body)

    def test_monitor_keeps_direct_live_announcement(self):
        body = monitor_body()

        self.assertIn("announce_game_message( message, urgent );", body)

    def test_monitor_logs_same_warning_silently(self):
        body = monitor_body()

        self.assertIn(
            "Messages::add_msg_silent( urgent ? m_warning : m_info, message );", body
        )

        self.assertLess(
            body.index("announce_game_message( message, urgent );"),
            body.index("Messages::add_msg_silent("),
        )

        self.assertNotIn("Messages::add_msg(", body)

    def test_monitor_log_write_gated_on_reader_enabled(self):
        body = monitor_body()

        guard = body.index("if( !enabled() || suspended() )")
        silent = body.index("Messages::add_msg_silent(")
        self.assertLess(guard, silent)

    def test_silent_entries_reach_double_tap_history(self):

        impl = (SRC / "messages.cpp").read_text("utf-8")
        self.assertIn("messages.emplace_back( m );", impl)
        world = (SRC / "a11y_world.cpp").read_text("utf-8")
        self.assertIn("Messages::recent_messages( 12 )", world)
        self.assertIn('document( "world:messages"', world)


if __name__ == "__main__":
    unittest.main()
