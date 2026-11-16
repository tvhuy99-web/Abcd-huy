"""Contracts for fully autonomous Explore Places vehicle journeys.

The overmap destination activity is ACT_AUTODRIVE, not the walking
Character::has_destination() route. Exercise the safety and JNI ownership
boundaries so future gesture-driving changes cannot silently undo them.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
RES = ROOT / "android/app/src/main/res"


def read(path):
    return path.read_text(encoding="utf-8")


PLACES = read(SRC / "overmap_ui.cpp")
ACTOR = read(SRC / "activity_actor.cpp")
DRIVER = read(SRC / "vehicle_autodrive.cpp")
VEHICLE = read(SRC / "vehicle.cpp")
DISPATCH = read(SRC / "handle_action.cpp")
BRIDGE = read(SRC / "a11y_android.cpp")
CONTROLLER = read(JAVA / "GameReaderController.java")


class ExploreAutodriveSyncContracts(unittest.TestCase):
    def test_places_uses_autonomous_activity_not_steering_swipes(self):
        method = PLACES.split("void ui::omap::display_accessible_places()", 1)[1]
        method = method.split("void ui::omap::display()", 1)[0]
        self.assertIn('intent.node == "places:mode:drive"', method)
        self.assertIn("you.in_vehicle && you.controlling_vehicle", method)
        self.assertIn("accessible_vehicle_route( destination, nearby_goal )", method)
        self.assertIn("accessible_driver_seat_nearby( you, get_map() )", method)
        self.assertIn("pending_accessible_drive = accessible_drive_request{ destination };", method)
        self.assertIn("you.omt_path = std::move( route );", method)
        self.assertIn("you.assign_activity( autodrive_activity_actor() );", method)
        self.assertNotIn("a11y_continuous_drive = true;", method)

    def test_nearby_boarding_and_ignition_precede_autodrive(self):
        world = read(SRC / "do_turn.cpp")
        runtime = read(SRC / "a11y_runtime.cpp")
        header = read(SRC / "overmap_ui.h")
        self.assertIn("bool accessible_driver_seat_nearby(", PLACES)
        self.assertIn("here.points_in_radius( origin, 16 )", PLACES)
        self.assertIn('found->part_with_feature( "BOARDABLE", true )', PLACES)
        self.assertIn('found->part_with_feature( "CONTROLS", true )', PLACES)
        self.assertIn("pending_accessible_drive = accessible_drive_request{ destination };", PLACES)
        self.assertIn("void ui::omap::update_accessible_drive_to_place()", PLACES)
        self.assertIn("if( driver.has_destination() )", PLACES)
        self.assertIn("g->control_vehicle( std::nullopt );", PLACES)
        self.assertIn("car.start_engines( here, &driver, true );", PLACES)
        self.assertIn("if( driver.activity )", PLACES)
        self.assertIn("driver.assign_activity( autodrive_activity_actor() );", PLACES)
        self.assertIn("ui::omap::update_accessible_drive_to_place();", world)
        self.assertIn("ui::omap::update_accessible_drive_to_place();", DISPATCH)
        self.assertIn("ui::omap::cancel_accessible_drive_to_place();", runtime)
        self.assertIn("bool accessible_drive_to_place_pending();", header)

    def test_reach_nearby_drivable_area_when_destination_is_not_drivable(self):
        self.assertIn("overmap_ui::get_overmap_path_to( destination, true, true )", PLACES)
        self.assertIn("for( int radius = 1; radius <= 2; ++radius )", PLACES)
        self.assertIn("overmap_buffer.seen( candidate ) <= om_vision_level::outlines", PLACES)
        self.assertIn("nearby_goal = true;", PLACES)
        self.assertIn("Driving to a reachable area near the selected place.", PLACES)
        self.assertIn("No safe vehicle route to this place or a nearby area.", PLACES)

    def test_autonomous_drive_mode_locks_only_four_straight_one_finger_swipes(self):
        guide = read(JAVA / "ReaderGestureGuide.java")
        mode = CONTROLLER.split("if (autonomousDriveTravelActive()) {", 1)[1]
        self.assertIn('case "swipe_up":', mode)
        for gesture in ('swipe_down', 'swipe_left', 'swipe_right'):
            self.assertIn('case "' + gesture + '":', CONTROLLER)
        self.assertIn("autonomousDriveTrip = true;", CONTROLLER)
        self.assertIn("vehicleMode = true;", CONTROLLER)
        self.assertIn("private void pauseOrResumeAutonomousDrive()", CONTROLLER)
        self.assertIn("requestAutonomousRead(", CONTROLLER)
        self.assertIn("autonomousDriveCancelRequested = true;", CONTROLLER)
        self.assertIn("nativeTravelControl(3)", CONTROLLER)
        self.assertIn("if (command == 3 && autonomousDriveTravelActive())", CONTROLLER)
        self.assertIn('"activity:autodrive"', read(SRC / "a11y_world.cpp"))
        self.assertIn("R.string.a11yGuideAutoMapCancel", guide)
        self.assertIn("R.string.a11yGuideAutoPause", guide)

    def test_native_actor_owns_controls_instead_of_walking_handler(self):
        self.assertIn("void autodrive_activity_actor::start(", ACTOR)
        self.assertIn("a11y::android::set_travel_state( 1 );", ACTOR)
        self.assertIn("const int control = a11y::android::take_travel_control();", ACTOR)
        self.assertIn("const bool driving_destination_activity =", DISPATCH)
        self.assertIn('activity_id( "ACT_AUTODRIVE" )', DISPATCH)
        self.assertIn("driving_destination_activity ? 0 :", DISPATCH)
        self.assertIn("!player_character.has_destination() && !driving_destination_activity",
                      DISPATCH)
        self.assertIn("pending_travel_control.exchange( 0", BRIDGE)
        self.assertIn("a11y::android::set_travel_state( 0 );", ACTOR)

    def test_pause_cancels_throttle_not_destination(self):
        method = ACTOR.split("void autodrive_activity_actor::do_turn(", 1)[1].split(
            "void autodrive_activity_actor::canceled(", 1)[0]
        self.assertIn("if( control == 1 && state == 1 )", method)
        self.assertIn("state = 2;", method)
        self.assertIn("if( state != 1 )", method)
        self.assertIn("a11y_brake_autodrive( *player_vehicle, here );", method)
        self.assertIn("who.set_moves( 0 );", method)
        self.assertNotIn("who.omt_path.clear();", method.split("if( state != 1 )", 1)[1])
        self.assertIn("if( control == 3 )", method)
        self.assertIn("player_vehicle->a11y_release_controls_pending = true;", method)
        self.assertIn("player_vehicle->a11y_drive_paused = true;", method)
        self.assertIn("player_vehicle->a11y_resume_cruise_velocity = 0;", method)
        self.assertIn("who.omt_path.clear();", method.split("if( control == 3 )", 1)[1])
        self.assertIn("who.cancel_activity();", method)
        self.assertIn("player_vehicle->stop_autodriving( true );", ACTOR)
        self.assertIn("handbrake( here );", ACTOR)

    def test_obstacle_block_has_no_ignore_button(self):
        self.assertIn("state = 4; // obstacle", ACTOR)
        self.assertIn("current != 4", BRIDGE)
        self.assertIn("state <= 4 ? state : 0", BRIDGE)
        self.assertIn("if( obstacle )", ACTOR)
        self.assertIn("Cannot resume: an obstacle still blocks the vehicle.", ACTOR)
        self.assertIn("state == 2 || state == 4", CONTROLLER)
        self.assertIn("R.string.a11yTravelObstacleStopped", CONTROLLER)
        self.assertIn("state == 3 ? 3 : 2", CONTROLLER)
        self.assertIn("control == 4 && current != 3", BRIDGE)
        self.assertIn("g->safe_mode == SAFE_MODE_STOP", ACTOR)
        for locale in ("values", "values-vi"):
            xml = read(RES / locale / "a11y_editor.xml")
            self.assertIn('name="a11yTravelObstacleStopped"', xml)

    def test_collision_and_missing_route_pause_instead_of_coasting(self):
        method = DRIVER.split("autodrive_result vehicle::do_autodrive(", 1)[1].split(
            "void vehicle::stop_autodriving(", 1
        )[0]
        self.assertIn("const auto safety_stop = [&]()", method)
        self.assertIn("cruise_velocity = 0;", method)
        self.assertIn("handbrake( here );", method)
        self.assertIn("a11y_obstacle_stop = true;", method)
        self.assertIn("a11y_obstacle_override = false;", method)
        self.assertIn(
            "const bool reader_autodrive = a11y::enabled() && driver.is_avatar();",
            method,
        )
        self.assertIn("if( reader_autodrive )", method)
        self.assertIn("return safety_stop();", method)
        self.assertIn("case collision_check_result::close_obstacle:", method)
        self.assertIn("case collision_check_result::no_visibility:", method)
        self.assertIn("if( !next_step )", method)

    def test_autonomous_sensor_brakes_even_if_manual_setting_remained(self):
        self.assertIn("if( a11y_manual_drive && !is_autodriving )", VEHICLE)
        self.assertIn("( a11y_manual_drive || is_autodriving )", VEHICLE)
        self.assertIn("if( is_autodriving ) {", VEHICLE)
        self.assertIn("a11y_obstacle_override = false;", VEHICLE)
        self.assertIn("a11y_obstacle_stop = true;", VEHICLE)
        self.assertIn("Automatic braking: %1$s ahead", VEHICLE)

    def test_stationary_pivot_uses_footprint_and_replans(self):
        method = DRIVER.split("autodrive_result vehicle::do_autodrive(", 1)[1].split(
            "void vehicle::stop_autodriving(", 1)[0]
        self.assertIn("velocity == 0 && turn_delta != 0", method)
        self.assertIn("a11y_rotate_in_place( here, std::clamp( turn_delta, -3, 3 ) )", method)
        self.assertIn("active_autodrive_controller.reset();", method)
        self.assertIn("cruise_velocity = 0;", method)
        self.assertIn("driver.set_moves( 0 );", method)

    def test_npc_autodrive_and_stale_controls_are_isolated(self):
        self.assertIn("who.is_avatar() && a11y::enabled() && a11y::android::enabled()",
                      ACTOR)
        self.assertIn("a11y::android::reset_travel_control();", ACTOR)
        self.assertIn("if( who.is_avatar() && a11y::enabled() )", ACTOR)
        self.assertIn("player_vehicle->stop_autodriving( false );", ACTOR)
        self.assertIn("driver.is_avatar()", DRIVER)

    def test_stopped_car_can_process_cancel_without_player_moves(self):
        # The deadlock occurred because Character::process_activity()
        # refuses to call ACT_AUTODRIVE with zero moves, while the Android
        # buttons only post a JNI request. The blocking-input pump MUST
        # process the request on the game thread even when out of moves.
        world = read(SRC / "do_turn.cpp")
        character = read(SRC / "character.cpp")
        header = read(SRC / "a11y_android.h")
        self.assertIn("while( get_moves() > 0 && activity )", character)
        self.assertIn("bool travel_control_pending();", header)
        self.assertIn("bool travel_control_pending()", BRIDGE)
        self.assertIn("pending_travel_control.load(", BRIDGE)
        self.assertIn("u.get_moves() <= 0 && u.activity", world)
        self.assertIn("u.activity.id() == ACT_AUTODRIVE", world)
        self.assertIn("a11y::android::travel_control_pending()", world)
        self.assertIn("u.activity.do_turn( u );", world)
        self.assertIn("a11y::android::reset_travel_control();", world)
        self.assertIn("who.cancel_activity();", ACTOR)

    def test_blocked_route_has_independent_back_and_hub_escape(self):
        self.assertIn("if (autonomousDriveTravelActive()) {", CONTROLLER)
        self.assertIn("if (travelState() == 4)", CONTROLLER)
        self.assertIn("if (travelState() == 4 || autonomousDriveTravelActive())",
                      CONTROLLER)
        self.assertIn("showComprehensiveMenu();", CONTROLLER)
        self.assertIn("hub.setText(R.string.a11yHubTitle);", CONTROLLER)
        self.assertIn("hub.setOnClickListener(view -> showComprehensiveMenu());",
                      CONTROLLER)
        self.assertIn("(!directWorldGesturesEnabled() && !autonomousDriveTravelActive())",
                      CONTROLLER)
        self.assertIn("travelChoice = travelChoiceCount(travelState()) - 1;\n"
                      "                activateTravel();", CONTROLLER)
        self.assertIn("( suspended() && control != 3 )", BRIDGE)
        self.assertIn("return pending_travel_control.exchange( 0", BRIDGE)

    def test_quiet_journey_but_urgent_warnings_survive(self):
        self.assertIn('travelState() != 0 && "activity".equals(snapshot.category)',
                      CONTROLLER)
        self.assertIn("priority < ReaderSpeechQueue.URGENT", CONTROLLER)
        self.assertIn("!extendedScan && !requestedDetail", CONTROLLER)
        self.assertIn("if (state != announcedTravelState)", CONTROLLER)
        self.assertIn("handler.postDelayed(travelStatePoll, 200);", CONTROLLER)


if __name__ == "__main__":
    unittest.main()
