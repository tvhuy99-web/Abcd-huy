"""Back works across reader screens and scans do not list empty ground."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class UniversalBackAndScanContracts(unittest.TestCase):
    def test_native_back_fallback_is_shared_by_all_screen_scopes(self):
        runtime = (SRC / "a11y_runtime.cpp").read_text("utf-8")
        input_code = (SRC / "a11y_input.cpp").read_text("utf-8")
        self.assertIn('if( intent->action == "cancel" ) {', runtime)
        self.assertIn("registered_intent( context, *intent )", runtime)
        self.assertIn('android::status( "unsupported", &*intent );', runtime)
        self.assertIn('"QUIT", "UILIST.QUIT", "TEXT.QUIT", "CANCEL"', input_code)
        # A handler first has one chance to go up an inner level.
        self.assertLess(runtime.index("screen.handler( *intent )"),
                        runtime.index("registered_intent( context, *intent )"))

    def test_back_handles_places_inner_selection_first(self):
        places = (SRC / "overmap_ui.cpp").read_text("utf-8")
        segment = places.split('a11y::screen_scope reader( &ctxt, "places"', 1)[1]
        segment = segment.split('if( intent.action == "focus" )', 1)[0]
        self.assertIn('if( intent.action == "cancel" )', segment)
        self.assertIn("selected.reset();", segment)
        self.assertIn('return std::string( "TIMEOUT" );', segment)
        self.assertIn('return std::string( "QUIT" );', segment)

    def test_uncancellable_uilist_does_not_gain_a_backdoor(self):
        uilist = (SRC / "uilist.cpp").read_text("utf-8")
        self.assertIn('return allow_cancel ? std::optional<std::string>( "UILIST.QUIT" )',
                      uilist)
        self.assertIn('if( allow_cancel ) {', uilist)
        self.assertIn('ctxt.register_action( "UILIST.QUIT" );', uilist)
        runtime = (SRC / "a11y_runtime.cpp").read_text("utf-8")
        self.assertIn("registered_intent( context, *intent )", runtime)

    def test_menu_revision_updates_do_not_cancel_two_finger_back(self):
        controller = (JAVA / "GameReaderController.java").read_text("utf-8")
        surface = (JAVA / "ReaderGestureSurface.java").read_text("utf-8")
        gesture = controller.split("public String gestureContext() {", 1)[1]
        gesture = gesture.split("@Override", 1)[0]
        self.assertNotIn("snapshot.revision", gesture)
        self.assertIn('":screen:" + snapshot.session', gesture)
        self.assertIn('snapshot.frame + ":" + snapshot.generation', gesture)
        self.assertIn("TwoFingerBackGesture.matches(event", surface)
        self.assertIn("listener.goBack();", surface)

    def test_both_scans_use_identical_unrestricted_map_reader(self):
        world = (SRC / "a11y_world.cpp").read_text("utf-8")
        nearby = (SRC / "surroundings_menu.cpp").read_text("utf-8")
        self.assertIn("scan_unrestricted_tile( pos )", world)
        self.assertIn("a11y::scan_unrestricted_tile( position )", nearby)
        self.assertIn("spoken_item_name( obj )", world)
        self.assertIn('_( "Items:" )', world)
        self.assertIn("!tile.furniture.empty()", nearby)
        self.assertIn("noteworthy_nearby_feature( terrain, position )", nearby)
        self.assertNotIn("noteworthy_nearby_feature( furniture, position )",
                         nearby.split("void surroundings_menu::execute_accessible()", 1)[1])
        self.assertNotIn("tile.source != a11y::observation_source::visible", nearby)

    def test_single_flight_does_not_disable_direction_commands_in_menus(self):
        native = (SRC / "a11y_android.cpp").read_text("utf-8")
        runtime = (SRC / "a11y_runtime.cpp").read_text("utf-8")
        self.assertIn("matches_last_world( intent.token )", native)
        self.assertIn("if( !world_step_ready ||", native)
        self.assertIn("const bool pending_step = std::any_of(", native)
        self.assertIn("matches_last_world( intent.token )", native)
        self.assertIn("world_step_ready = false;", native)
        self.assertIn('screens.back()->category == "DEFAULTMODE"', runtime)
        self.assertIn("contexts.active_token()", runtime)


if __name__ == "__main__":
    unittest.main()
