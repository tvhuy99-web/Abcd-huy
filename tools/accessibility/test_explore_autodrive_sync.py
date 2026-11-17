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
SPEECH_QUEUE = read(JAVA / "ReaderSpeechQueue.java")


class ExploreAutodriveSyncContracts(unittest.TestCase):
    def test_places_uses_autonomous_activity_not_steering_swipes(self):
        method = PLACES.split("void ui::omap::display_accessible_places()", 1)[1]
        method = method.split("void ui::omap::display()", 1)[0]
        self.assertIn('intent.node == "places:mode:drive"', method)
        self.assertIn("you.in_vehicle && you.controlling_vehicle", method)
        self.assertIn("accessible_vehicle_route( destination, nearby_goal )", method)
        self.assertIn("accessible_driver_seat_nearby( you, get_map() )", method)
        self.assertIn(
            "pending_accessible_drive = accessible_drive_request{ destination, destination_name };",
            method,
        )
        self.assertIn("you.omt_path = std::move( route );", method)
        self.assertIn(
            "you.assign_activity( autodrive_activity_actor( destination, destination_name, nearby_goal ) );",
            method,
        )
        self.assertNotIn("a11y_continuous_drive = true;", method)

    def test_nearby_boarding_and_ignition_precede_autodrive(self):
        world = read(SRC / "do_turn.cpp")
        runtime = read(SRC / "a11y_runtime.cpp")
        header = read(SRC / "overmap_ui.h")
        self.assertIn("bool accessible_driver_seat_nearby(", PLACES)
        self.assertIn("here.points_in_radius( origin, 16 )", PLACES)
        self.assertIn('found->part_with_feature( "BOARDABLE", true )', PLACES)
        self.assertIn('found->part_with_feature( "CONTROLS", true )', PLACES)
        self.assertIn("pending_accessible_drive = accessible_drive_request{ destination, destination_name };", PLACES)
        self.assertIn("void ui::omap::update_accessible_drive_to_place()", PLACES)
        self.assertIn("if( driver.has_destination() )", PLACES)
        self.assertIn("g->control_vehicle( std::nullopt );", PLACES)
        self.assertIn("car.start_engines( here, &driver, true );", PLACES)
        self.assertIn("if( driver.activity )", PLACES)
        self.assertIn(
            "driver.assign_activity( autodrive_activity_actor( pending_accessible_drive->destination,",
            PLACES,
        )
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
        self.assertIn("if (command == 3 && autonomousDriveTrip)", CONTROLLER)
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

    def test_unreachable_route_pauses_without_obstacle_dialog(self):
        self.assertIn("if( player_vehicle->a11y_obstacle_stop && state == 1 )", ACTOR)
        self.assertIn("state = 2;", ACTOR)
        self.assertNotIn("state = 4; // obstacle", ACTOR)
        self.assertNotIn('Cannot resume: an obstacle still blocks the vehicle.', ACTOR)
        self.assertIn("player_vehicle->a11y_obstacle_stop = false;", ACTOR)
        self.assertIn("if( state != 1 )", ACTOR)
        self.assertIn("state == 2 || state == 4", CONTROLLER)
        self.assertIn("control == 4 && current != 3", BRIDGE)
        self.assertIn("g->safe_mode == SAFE_MODE_STOP", ACTOR)
        for locale in ("values", "values-vi"):
            xml = read(RES / locale / "a11y_editor.xml")
            self.assertIn('name="a11yTravelObstacleStopped"', xml)

    def test_collision_and_missing_route_pause_instead_of_coasting(self):
        method = DRIVER.split("autodrive_result vehicle::do_autodrive(", 1)[1].split(
            "void vehicle::stop_autodriving(", 1
        )[0]
        self.assertIn("const auto safety_stop = [&]( const char *reason )", method)
        self.assertIn("cruise_velocity = 0;", method)
        self.assertIn("handbrake( here );", method)
        self.assertIn("a11y_obstacle_stop = true;", method)
        self.assertIn("a11y_obstacle_override = false;", method)
        self.assertIn(
            "const bool reader_autodrive = a11y::enabled() && driver.is_avatar();",
            method,
        )
        self.assertIn("if( reader_autodrive )", method)
        self.assertIn('return safety_stop( "no_local_path" );', method)
        self.assertIn('return safety_stop( "close_obstacle" );', method)
        self.assertIn('return safety_stop( "no_visibility" );', method)
        self.assertIn('return safety_stop( "pivot_blocked" );', method)
        self.assertIn('diagnostic_event( "AUTODRIVE_ROUTE"', method)
        self.assertIn("case collision_check_result::close_obstacle:", method)
        self.assertIn("case collision_check_result::no_visibility:", method)
        self.assertIn("if( !next_step )", method)

    def test_pivot_preflight_checks_one_increment_and_continues_other_headings(self):
        self.assertIn("constexpr std::array<int, 7> turns", DRIVER)
        self.assertIn("std::clamp( steps, -3, 3 )", DRIVER)
        self.assertIn("if( !can_pivot_first_step( first_step ) )", DRIVER)
        self.assertIn("++blocked_first_turns;", DRIVER)
        self.assertIn("++no_path_headings;", DRIVER)
        self.assertIn("result=selected steps=", DRIVER)
        self.assertIn("result=none blocked_first=", DRIVER)

    def test_successful_astar_reports_real_state_count(self):
        self.assertIn("last_search.search_nodes = known_nodes.size();", DRIVER)
        self.assertIn("last_search.found_goal = true;", DRIVER)
        self.assertIn('"goal_found"', DRIVER)

    def test_ground_items_are_not_fabricated_solid_vehicle_obstacles(self):
        classifier = DRIVER.split(
            "vehicle_autodrive::tile_blocker vehicle_autodrive::classify_tile(", 1
        )[1].split("class vehicle::autodrive_controller", 1)[0]
        self.assertNotIn("here.has_items( pt )", classifier)
        self.assertIn("tile_blocker::other_vehicle", classifier)
        self.assertIn("tile_blocker::creature", classifier)
        self.assertIn("tile_blocker::visible_trap", classifier)
        self.assertIn("tile_blocker::furniture", classifier)
        self.assertIn("tile_blocker::terrain", classifier)
        self.assertIn("vehicle_autodrive::classify_tile( veh, here, tile,", read(SRC / "a11y_world.cpp"))
        self.assertIn("vehicle_autodrive::classify_tile( *this, here, tile,", read(SRC / "vehicle_move.cpp"))

    def test_hub_selection_cannot_pause_or_resume_a_vehicle(self):
        part = CONTROLLER.split("public void activate()", 1)[1].split("public void readCurrent()", 1)[0]
        self.assertLess(
            part.index("if (comprehensiveMenu)"),
            part.index("if (!activatingHubCommand && autonomousDriveTravelActive())")
        )
        self.assertLess(part.index("if (comprehensiveMenu)"), part.index("if (!activatingHubCommand && vehicleMode"))
        self.assertIn("activatingHubCommand = true;", CONTROLLER)
        self.assertIn("activatingHubCommand = false;", CONTROLLER)

    def test_heading_speech_only_after_real_body_heading_change(self):
        self.assertIn("vehicleHeadingBeforeCommand = vehicleCompass(", CONTROLLER)
        self.assertIn("!actualCompass.equals(vehicleHeadingBeforeCommand)", CONTROLLER)
        self.assertIn("pendingVehicleHeadingSnapshots >= 8", CONTROLLER)
        self.assertIn("pendingVehicleHeadingReport = false;", CONTROLLER)

    def test_diagnose_and_recover_stopped_car_before_stopping(self):
        for token in ("search_nodes=", "search_expanded=", "start_options=",
                      "rejected_footprint=", "search_limit=", "facing_degrees="):
            self.assertIn(token, DRIVER)
        self.assertIn("find_accessible_pivot(", DRIVER)
        self.assertIn("can_pivot_first_step( first_step )", DRIVER)
        self.assertIn('"AUTODRIVE_PIVOT"', DRIVER)
        self.assertIn("compute_path( MIN_SPEED_TPS, candidate, 2000 )", DRIVER)
        self.assertIn("a11y_rotate_in_place( here, step )", DRIVER)
        self.assertIn("a11y_pivot_steps_remaining -= step;", DRIVER)
        self.assertIn("a11y_recovery_segment_omt.reset();", ACTOR)
        self.assertIn("reverse_start_options=", DRIVER)
        self.assertIn("reverse_live_clear=", DRIVER)
        self.assertIn("reverse_live_blocked=", DRIVER)
        self.assertIn("AUTODRIVE_REVERSE_PROBE", DRIVER)
        self.assertIn("search_class=", DRIVER)
        self.assertIn("no_initial_step", DRIVER)
        self.assertIn("search_budget", DRIVER)

    def test_bounded_reverse_uses_native_physics_and_brakes_after_one_tile(self):
        method = DRIVER.split("autodrive_result vehicle::do_autodrive(", 1)[1].split(
            "void vehicle::stop_autodriving(", 1)[0]
        self.assertIn("safe_straight_reverse_step( here )", method)
        self.assertIn("const auto try_reverse_escape =", method)
        self.assertIn("if( try_reverse_escape() )", method)
        self.assertIn("a11y_reverse_recovery_attempts >= 2", method)
        self.assertIn("a11y_reverse_recovery_ticks++ >= 6", method)
        self.assertIn("a11y::vehicle_obstacle_ahead( *this, here, true )", method)
        self.assertIn("cruise_velocity = -VMIPH_PER_TPS;", method)
        self.assertIn("handbrake( here );", method)
        self.assertIn('"result=moved_one_tile', method)
        self.assertIn('safety_stop( "reverse_recovery_blocked" )', method)
        self.assertIn("driver.set_moves( 0 );", method)
        self.assertNotIn("here.displace_vehicle(", method)
        self.assertNotIn("pos_abs() =", method)
        self.assertIn("player_vehicle->a11y_reverse_recovery_active = false;", ACTOR)
        self.assertIn("a11y_reverse_recovery_attempts = 0;", DRIVER)

    def test_reverse_probe_rechecks_live_footprint_without_reversing(self):
        part = DRIVER.split("std::string vehicle::autodrive_controller::reverse_clearance_details(", 1)[1]
        part = part.split("std::size_t vehicle::autodrive_controller::forward_start_options(", 1)[0]
        self.assertIn("compute_next_nodes(", part)
        self.assertIn("MIN_SPEED_TPS", DRIVER)
        self.assertIn("data.profile( placement.second ).occupied_zone", part)
        self.assertIn("!check_drivable( here, tile )", part)
        self.assertIn("target_abs.to_string()", part)
        self.assertIn("last_search = saved;", part)
        for dangerous in ("pldrive(", "cruise_velocity =", "displace_vehicle(", "handbrake("):
            self.assertNotIn(dangerous, part)

    def test_reroute_after_blocked_stationary_pivot(self):
        self.assertIn("const auto retry_nearby_route =", DRIVER)
        self.assertIn("if( retry_nearby_route() )", DRIVER)
        self.assertIn("a11y_rotate_in_place( here, first_step, true )", DRIVER)
        self.assertIn("std::clamp( candidate_turn, -3, 3 ), true", DRIVER)
        self.assertIn('return safety_stop( "pivot_blocked" );', DRIVER)

    def test_bounded_strategic_alternate_only_after_local_failure(self):
        self.assertIn("accessible_vehicle_alternative_routes(", PLACES)
        self.assertIn("max_path_checks = 12", PLACES)
        self.assertIn("max_results = 4", PLACES)
        self.assertIn("std::chrono::milliseconds( 500 )", PLACES)
        self.assertIn("route.back() == blocked_step", PLACES)
        self.assertIn("overmap_buffer.seen( candidate )", PLACES)
        self.assertIn("!a11y_route_alternatives_tried", DRIVER)
        self.assertIn("omt_path = original_route;", DRIVER)
        self.assertIn("candidate_controller->compute_next_step( here )", DRIVER)
        self.assertIn("AUTODRIVE_REROUTE", DRIVER)
        self.assertIn("skipped=no_executable_first_step", DRIVER)
        self.assertIn("forward_start_options( here )", DRIVER)
        self.assertIn("candidate_controller->check_collision_zone( here,", DRIVER)
        self.assertNotIn("A11Y_DRIVE_FEEDBACK:", DRIVER)
        self.assertIn("AUTODRIVE_REROUTE", DRIVER)

    def test_autonomous_planner_avoids_obstacles_before_emergency_braking(self):
        self.assertIn("if( a11y_manual_drive && !is_autodriving )", VEHICLE)
        self.assertIn("( a11y_manual_drive || is_autodriving )", VEHICLE)
        self.assertIn("} else if( is_autodriving ) {", VEHICLE)
        self.assertIn("a11y_obstacle_override = false;", VEHICLE)
        self.assertIn("a11y_obstacle_stop = true;", VEHICLE)
        self.assertIn("Automatic braking: %1$s ahead", VEHICLE)
        driver = DRIVER.split("autodrive_result vehicle::do_autodrive(", 1)[1]
        self.assertIn("retry_local_route", driver)
        self.assertIn("std::make_shared<autodrive_controller>", driver)
        self.assertIn("if( !next_step && reader_autodrive )", driver)
        self.assertIn("alternate_clearance", driver)
        self.assertIn("next_step = alternate;", driver)
        self.assertIn("No safe route found. Vehicle paused.", driver)
        self.assertIn("if( !a11y_obstacle_stop )", driver)

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

    def test_autonomous_drive_keeps_existing_gestures_without_modal(self):
        self.assertIn("else if (walkingTravelPanelActive()", CONTROLLER)
        self.assertIn("if (activeTravel != 0 && !autonomousDriveTrip && !vehicleMode)", CONTROLLER)
        # A paused or transitioning destination must keep the ordinary
        # vehicle gestures and must not re-open the walking travel panel.
        # The poll now deliberately covers both autonomous and vehicle mode.
        self.assertIn("if (autonomousDriveTrip || vehicleMode) {", CONTROLLER)
        self.assertIn("private boolean walkingTravelPanelActive()", CONTROLLER)
        self.assertIn("return !autonomousDriveTrip && !vehicleMode && travelState() != 0;",
                      CONTROLLER)
        self.assertIn("public void openActionMenu()", CONTROLLER)
        self.assertIn("showComprehensiveMenu();", CONTROLLER)
        self.assertIn("if (autonomousDriveTrip && rawNativeTravelState() != 0)",
                      CONTROLLER)
        self.assertNotIn("hub.setOnClickListener(view -> showComprehensiveMenu());",
                         CONTROLLER)
        self.assertIn("travelChoice = 1;\n"
                      "            activateTravel();", CONTROLLER)
        self.assertIn("( suspended() && control != 3 )", BRIDGE)
        self.assertIn("return pending_travel_control.exchange( 0", BRIDGE)

    def test_activity_reader_allows_only_live_autodrive_queries(self):
        runtime = read(SRC / "a11y_runtime.cpp")
        self.assertIn('screen.category == "activity" || screen.category == "DEFAULTMODE"', runtime)
        self.assertIn('n.id == "activity:autodrive"', runtime)
        self.assertIn('query.rfind( "reader:scan:vehicle:", 0 ) == 0', runtime)
        self.assertIn('query == "reader:coordinates"', runtime)
        self.assertIn('query == "reader:vehicle:horn"', runtime)
        self.assertIn('query == "control_vehicle"', runtime)
        self.assertIn("intent->revision <= screen.revision", runtime)
        self.assertIn("live_read_only_focus || live_vehicle_query", runtime)
        self.assertIn("contexts.accepts( intent->token )", runtime)
        self.assertIn("intent->request > screen.last_request", runtime)
        self.assertIn('n.id == intent->node', runtime)
        self.assertIn('intent->action', runtime)

    def test_autodrive_never_intercepts_ordinary_reader_focus(self):
        self.assertIn("private boolean walkingTravelPanelActive()", CONTROLLER)
        self.assertIn("return !autonomousDriveTrip && !vehicleMode", CONTROLLER)
        self.assertIn("rawNativeTravelState() == 0", CONTROLLER)
        self.assertIn("walkingTravelPanelActive()", CONTROLLER)
        self.assertIn("if (walkingTravelPanelActive()) { activateTravel(); return; }",
                      CONTROLLER)
        self.assertIn("if (walkingTravelPanelActive()) {", CONTROLLER)
        self.assertIn("feedback(travelSpoken(travelState()));", CONTROLLER)
        self.assertIn("feedback(travelDetailSpeech(travelState()));", CONTROLLER)
        self.assertIn("public void openActionMenu()", CONTROLLER)
        self.assertIn("showComprehensiveMenu();", CONTROLLER)
        self.assertIn("&& rawNativeTravelState() != 0", CONTROLLER)
        self.assertIn("!combatMode && !vehicleMode && !autonomousDriveTrip",
                      CONTROLLER)
        self.assertNotIn("hub.setOnClickListener(view -> showComprehensiveMenu());",
                         CONTROLLER)

    def test_autodrive_uses_world_reader_without_activity_menu(self):
        world = read(SRC / "do_turn.cpp")
        activity = read(SRC / "a11y_world.cpp")
        self.assertIn('drive_reader ? "DEFAULTMODE" : "activity"', world)
        self.assertIn('drive.title = _( "World" )', world)
        self.assertIn('drive.focus = "world:here";', world)
        self.assertIn('node.id != "activity:autodrive"', world)
        self.assertIn('node.id != "world:here"', world)
        self.assertIn('node.id.rfind( "reader:vehicle:", 0 )', world)
        self.assertIn('reader_is_autodrive = drive_reader;', world)
        self.assertIn('"world:here", "reader:vehicle:seat"', activity)
        self.assertIn('next.find("activity:autodrive") != null;', CONTROLLER)
        self.assertIn('if (autonomousDriveTrip || vehicleMode) {', CONTROLLER)
        self.assertIn('"activity:autodrive".equals(node.id)', CONTROLLER)
        self.assertIn('if (walkingTravelPanelActive()) {', CONTROLLER)

    def test_compact_diagnostics_preserve_route_failures(self):
        bridge = read(SRC / "a11y_android.cpp")
        self.assertIn('std::array<performance_sample, 4> samples', bridge)
        for category in ("WORLD_CACHE_PHASE", "WORLD_AI_COUNTS",
                         "WORLD_SIM_TIMING", "WORLD_RENDER_BREAKDOWN"):
            self.assertIn('"' + category + '"', bridge)
        self.assertIn('std::chrono::seconds( 1 )', bridge)
        self.assertIn("suppressed_samples=", bridge)
        self.assertIn('diagnostic_event( "AUTODRIVE_ROUTE"', DRIVER)
        self.assertIn("safe_speed_tps=", DRIVER)
        self.assertIn("goal_states=", DRIVER)
        self.assertIn("land_ok=", DRIVER)
        self.assertIn("if( !a11y_obstacle_stop )", DRIVER)

    def test_arrival_is_announced_and_clears_old_progress(self):
        self.assertIn('a11y::android::announce( "A11Y_AUTODRIVE_END:"', ACTOR)
        self.assertIn('reader_announce( who, "ended", "arrived"', ACTOR)
        self.assertIn("autodriveEnd || autodriveStopped || autodriveArrived", CONTROLLER)
        self.assertIn("You have reached your destination.", ACTOR)
        self.assertIn("lastAutodriveTerminalActionEpoch", CONTROLLER)
        self.assertIn("speech.discardStaleAutodriveProgress();", CONTROLLER)

    def test_stopped_autodrive_discards_old_progress_not_real_warnings(self):
        self.assertIn("discardStaleAutodriveProgress()", SPEECH_QUEUE)
        self.assertIn("isAutodriveProgressText", SPEECH_QUEUE)
        self.assertIn("orderedGameEvents.iterator()", SPEECH_QUEUE)
        self.assertIn("urgentGameEvents.iterator()", SPEECH_QUEUE)
        self.assertIn("speech.discardStaleAutodriveProgress();", CONTROLLER)
        self.assertIn("lastAutodriveTerminalActionEpoch == receivedActionEpoch", CONTROLLER)
        self.assertIn("pendingGameMessages.removeIf", CONTROLLER)
        self.assertIn("No safe route found. Vehicle paused.", CONTROLLER)

    def test_quiet_journey_but_urgent_warnings_survive(self):
        self.assertIn('travelState() != 0 && "activity".equals(snapshot.category)',
                      CONTROLLER)
        # d9c2b55 rewrote the driving speech policy in the positive form;
        # urgent warnings still pass the gate.
        self.assertIn("priority >= ReaderSpeechQueue.URGENT", CONTROLLER)
        # d9c2b55 routes the scan/detail exceptions through the shared
        # allowedDuringDriving policy instead of an inline condition.
        self.assertIn('|| message.startsWith("A11Y_SCAN:")', CONTROLLER)
        self.assertIn('|| message.startsWith("A11Y_DETAIL:")', CONTROLLER)
        self.assertIn("if (state != announcedTravelState)", CONTROLLER)
        self.assertIn("handler.postDelayed(travelStatePoll, 200);", CONTROLLER)


if __name__ == "__main__":
    unittest.main()
