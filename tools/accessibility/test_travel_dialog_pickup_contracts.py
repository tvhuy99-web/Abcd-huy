"""Cross-screen contracts for user-controlled automatic travel and pickup results.

The native game thread owns safety decisions. Android displays controls and
queues requests but never manipulates character state, item state or routes.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
def read(file):
    return file.read_text("utf-8")


class TravelDialogPickupContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bridge = read(SRC / "a11y_android.cpp")
        cls.bridge_header = read(SRC / "a11y_android.h")
        cls.handler = read(SRC / "handle_action.cpp")
        cls.dialog = read(SRC / "popup.cpp")
        cls.output = read(SRC / "output.cpp")
        cls.game = read(SRC / "game.cpp")
        cls.controller = read(JAVA / "GameReaderController.java")
        cls.activity = read(JAVA / "CataclysmDDA.java")
        cls.pickup = read(SRC / "pickup.cpp")
        cls.pickup_header = read(SRC / "pickup.h")
        cls.pickup_actor = read(SRC / "activity_actor.cpp")

    def test_travel_requests_are_thread_safe_and_game_thread_authoritative(self):
        self.assertIn("std::atomic<int> active_travel_state{ 0 }", self.bridge)
        self.assertIn("std::atomic<int> pending_travel_control{ 0 }", self.bridge)
        self.assertIn("pending_travel_control.exchange( 0,", self.bridge)
        self.assertIn("Java_com_cleverraven_cataclysmdda_CataclysmDDA_nativeTravelControl", self.bridge)
        self.assertIn("static native int nativeTravelState();", self.activity)
        self.assertIn("static native boolean nativeTravelControl(int action);", self.activity)
        self.assertIn("const int travel_control = a11y::android::take_travel_control();",
                      self.handler)
        # No game/Character mutation belongs in the JNI callback.
        jni = self.bridge.split(
            "Java_com_cleverraven_cataclysmdda_CataclysmDDA_nativeTravelControl(", 1
        )[1].split("extern \"C\" JNIEXPORT", 1)[0]
        self.assertNotIn("abort_automove", jni)
        self.assertNotIn("set_destination(", jni)
        self.assertNotIn("set_safe_mode", jni)

    def test_pause_does_not_consume_waypoints_or_game_turns(self):
        src = self.handler
        self.assertIn("a11y::android::set_travel_state( 2 );", src)
        self.assertIn("get_player_input( ignored_action );", src)
        self.assertIn("return false;", src)
        first = src.index("const int travel_control = a11y::android::take_travel_control();")
        waypoint = src.index("act = player_character.get_next_auto_move_direction();", first)
        self.assertLess(first, waypoint)
        pause_branch = src.split(
            "if( player_character.has_destination() &&", 1
        )[1].split("if( player_character.has_destination() && a11y::enabled() )", 1)[0]
        self.assertNotIn("clear_destination();", pause_branch)
        self.assertNotIn("get_next_auto_move_direction();", pause_branch)
        self.assertIn("player_character.abort_automove();", pause_branch)

    def test_all_interruptions_keep_yes_cancel_no_keep_native_semantics(self):
        self.assertIn('if( query_yn( _( "%s Cancel auto move?" ), text ) )', self.game)
        self.assertIn("you.abort_automove();", self.game)
        self.assertIn("u.set_destination( u.get_auto_move_route()", self.game)
        self.assertIn('const int travel_control = a11y::android::take_travel_control();',
                      self.handler)
        self.assertIn("safe_mode == SAFE_MODE_STOP", self.handler)
        self.assertIn("a11y::android::set_travel_state( 3 );", self.handler)
        self.assertIn("travel_control == 4", self.handler)
        self.assertIn("set_safe_mode( SAFE_MODE_ON );", self.handler)
        self.assertNotIn("set_safe_mode( SAFE_MODE_OFF );\n        a11y::android::set_travel_state",
                         self.handler)
        # A step can be rejected after get_next_auto_move_direction()
        # has already removed its waypoint. It must be restored on a
        # safety stop; simply keeping the remaining route would skip it.
        self.assertIn("const auto keep_safety_stopped_route =", self.handler)
        self.assertIn("get_auto_move_route().insert(", self.handler)
        self.assertIn("get_auto_move_route().begin(), dest_tile", self.handler)
        self.assertIn("if( !keep_safety_stopped_route() )", self.handler)
        self.assertIn('_( "Travel stopped by safe mode. Route kept." )', self.handler)
        # Losing Android foreground focus must never silently resume.
        self.assertIn("!a11y::android::enabled()", self.handler)

    def test_travel_panel_is_accessible_and_modal_warnings_take_priority(self):
        src = self.controller
        self.assertIn('&& !"DEFAULTMODE".equals(snapshot.category)', src)
        self.assertIn('&& !"overmap".equals(snapshot.category)', src)
        self.assertIn('&& !"look-around".equals(snapshot.category)', src)
        self.assertIn("handler.postDelayed(travelStatePoll, 200);", src)
        self.assertIn("if (state != announcedTravelState)", src)
        self.assertIn('|| (!"activity".equals(snapshot.category)', src)
        self.assertIn("private void renderTravelPanel(int state)", src)
        self.assertIn("else if (travelState() != 0)", src)
        self.assertIn("travelChoice = Math.floorMod(travelChoice + delta,", src)
        self.assertIn("if (travelState() != 0) { activateTravel(); return; }", src)
        self.assertIn("CataclysmDDA.nativeTravelControl(command)", src)
        self.assertIn('state == 3 ? 3 : 2;', src)
        self.assertIn("if (travelState() != 0) {", src)
        self.assertIn("travelStatus(activeTravel)", src)
        for language in ["values", "values-vi"]:
            source = read(ROOT / f"android/app/src/main/res/{language}/a11y_editor.xml")
            for name in ("a11yTravelMoving", "a11yTravelPaused",
                         "a11yTravelSafetyStopped", "a11yTravelCancel",
                         "a11yTravelIgnoreWarning", "a11yTravelResume"):
                self.assertIn(f'name="{name}"', source)

    def test_yes_no_has_original_question_and_only_its_true_choices(self):
        self.assertIn('const bool yes_no_prompt = category == "YESNO"', self.dialog)
        self.assertIn("!reader_snapshot.summary.empty() && !yes_no_prompt", self.dialog)
        self.assertIn("node.detail = reader_snapshot.summary;", self.dialog)
        self.assertNotIn('question.label = _( "Dialog message" );', self.dialog)
        self.assertIn('String introduction = yesNo ? next.summary.trim() : next.title;',
                      self.controller)
        self.assertIn("!a11y::enabled()", self.output)
        self.assertIn("!a11y::enabled()", self.game)

    def test_pickup_reports_exact_failed_items_from_native_reasons(self):
        self.assertIn("a11y_success_count", self.pickup_header)
        self.assertIn("a11y_failure_count", self.pickup_header)
        self.assertIn("a11y_failures", self.pickup_header)
        self.assertIn("a11y_successes", self.pickup_header)
        self.assertIn('jsout.member( "a11y_failures"', self.pickup)
        self.assertIn('jsobj.read( "a11y_failures"', self.pickup)
        self.assertIn("selected_name = target->display_name(", self.pickup)
        self.assertIn("!failure_reason.empty()", self.pickup)
        self.assertIn('_( "The item is too heavy to carry." )', self.pickup)
        self.assertIn('_( "There is not enough usable inventory space." )', self.pickup)
        self.assertIn('_( "The pickup volume limit was reached." )', self.pickup)
        self.assertIn('_( "No available pocket accepted this item." )', self.pickup)
        self.assertIn("if( !keep_going || target_items.empty() )", self.pickup_actor)
        self.assertIn('_( "Not collected:" )', self.pickup_actor)
        self.assertIn("info.a11y_failures", self.pickup_actor)
        self.assertIn("info.a11y_successes", self.pickup_actor)
        self.assertIn("!autopickup", self.pickup_actor)

    def test_look_around_always_has_one_swipe_path_back_to_map(self):
        self.assertIn('exit.id = "look:return_to_map";', self.game)
        self.assertIn('return "QUIT";', self.game)
        self.assertIn('if ("look-around".equals(snapshot.category)) { return result; }',
                      self.controller)


if __name__ == "__main__":
    unittest.main()
