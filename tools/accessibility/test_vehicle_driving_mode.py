"""Source contracts for accessible vehicle control and safety.

These assertions prevent regressions in the mode/gesture and driver-seat
separation. Native collisions and on-device swipes still require runtime tests.
"""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
CTRL = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
SURF = (JAVA / "ReaderGestureSurface.java").read_text(encoding="utf-8")
WORLD = (ROOT / "src/a11y_world.cpp").read_text(encoding="utf-8")
ENGINE = (ROOT / "src/handle_action.cpp").read_text(encoding="utf-8")
VEH = (ROOT / "src/vehicle.cpp").read_text(encoding="utf-8")
VEH_MOVE = (ROOT / "src/vehicle_move.cpp").read_text(encoding="utf-8")
VEH_HEADER = (ROOT / "src/vehicle.h").read_text(encoding="utf-8")
VEH_USE = (ROOT / "src/vehicle_use.cpp").read_text(encoding="utf-8")
AUTODRIVE = (ROOT / "src/vehicle_autodrive.cpp").read_text(encoding="utf-8")
GAME = (ROOT / "src/game.cpp").read_text(encoding="utf-8")


class AccessibleVehicleContracts(unittest.TestCase):
    def test_onboard_map_swipes_only_walk_after_release(self):
        gate = WORLD.split("result.swipe_movement_allowed =", 1)[1].split(
            "constexpr std::array<point, 8> combat_steps", 1
        )[0]

        for forbidden in ("!viewer.in_vehicle", "!viewer.controlling_vehicle",
                          "!viewer.is_driving()"):
            self.assertNotIn(forbidden, gate)
        self.assertIn("&& !viewer.activity", gate)
        self.assertIn("&& !vehicleReleasePending && !vehicleDriverReady()", CTRL)
        self.assertIn("return !combatMode && !vehicleMode && !autonomousDriveTrip", CTRL)
        self.assertIn('requestLiveWorldCommand("reader:vehicle:release")', CTRL)

    def test_status_uses_native_control_ownership(self):
        report = WORLD.split("std::string player_status()", 1)[1].split(
            "void reset_status_monitor()", 1
        )[0]
        self.assertIn("driven_vehicle->player_in_control( get_map(), viewer )",
                      report)
        self.assertIn('_( "On board vehicle" )', report)
        self.assertIn("At the driver's controls. Not yet driving.", report)
        self.assertIn("Not at the driver's controls.", report)
        seat = WORLD.split('seat.id = "reader:vehicle:seat"', 1)[1].split(
            'controls.id = "reader:vehicle:driver"', 1
        )[0]
        self.assertIn("at_controls ?", seat)
        self.assertIn("Driving controls available.", seat)
        self.assertIn("At the driver's controls. Not yet driving.", seat)
        self.assertIn("Not at the driver's controls.", seat)
        vi = (ROOT / "lang/po/accessibility/vi.po").read_text("utf-8")
        self.assertIn("Đang ở vị trí lái, chưa nắm quyền điều khiển.", vi)
        self.assertIn("Đang ở trên xe, chưa ở vị trí lái.", vi)

    def test_three_modes_only_appear_when_on_board(self):
        self.assertIn('snapshot.find("reader:vehicle:seat") != null', CTRL)
        self.assertIn("int nextMode = combatMode ? 2 : vehicleMode ? 0 : 1;", CTRL)
        self.assertIn('next.find("reader:vehicle:seat") == null', CTRL)
        self.assertIn("vehicleMode = false;", CTRL)

    def test_driving_requires_live_native_driver(self):
        self.assertIn('snapshot.find("reader:vehicle:driver")', CTRL)
        self.assertIn("state != null && state.enabled", CTRL)
        self.assertIn("vehicle_reader.controlling_vehicle", WORLD)
        self.assertIn("veh.player_in_control", WORLD)
        self.assertIn('intent.node == "reader:vehicle:brake"', ENGINE)

    def test_single_and_bent_swipes_have_separate_meanings(self):
        for gesture in (
            "swipe_up", "swipe_down", "swipe_left", "swipe_right",
            "swipe_up_down", "swipe_down_up", "swipe_left_right",
            "swipe_right_left",
        ):
            self.assertIn('case "' + gesture + '":', CTRL)
        self.assertIn('requestVehicleAction("reader:vehicle:heading:left")',
                      CTRL)
        self.assertIn('requestVehicleAction("reader:vehicle:heading:right")',
                      CTRL)
        self.assertIn("a11y_steering_nudge = 3;", ENGINE)

        vehicle = CTRL.split(
            "if (vehicleMode && directWorldGesturesEnabled()) {", 1
        )[1]
        for gesture, node in (
            ("swipe_up", "forward"), ("swipe_down", "reverse"),
            ("swipe_up_down", "faster"), ("swipe_down_up", "slower")
        ):
            drive = vehicle.split('case "' + gesture + '":', 1)[1].split(
                "return true;", 1
            )[0]
            self.assertIn('requestVehicleAction("reader:vehicle:' + node + '")',
                          drive)
            self.assertIn('reader:vehicle:' + node + '"', WORLD)
        self.assertIn("a11y_vehicle_throttle::reverse", ENGINE)
        self.assertIn("a11y_vehicle_throttle::faster", ENGINE)
        self.assertIn("pldrive( point_rel_ms( 0, -1 ) );", ENGINE)
        self.assertIn("pldrive( point_rel_ms( 0, 1 ) );", ENGINE)
        self.assertNotIn("car.cruise_thrust( here,", ENGINE)
        pattern = (
            r'requestVehicleAction\("reader:vehicle:horn"\);\s*return true;\s*'
            r'default:\s*(?://[^\n]*\n\s*)*return true;'
        )
        self.assertRegex(CTRL, pattern)

    def test_vehicle_onboarding_uses_touch_instructions(self):
        source = (ROOT / "src/game.cpp").read_text(encoding="utf-8")
        self.assertIn('_( "Switch to driving mode to take control." )', source)
        self.assertIn("if( a11y::enabled() )", source)
        self.assertIn("if( !a11y::enabled() )", VEH_USE)
        self.assertIn('#include "a11y_runtime.h"', VEH_USE)

    def test_engine_toggle_has_mode_scoped_gesture_and_real_start_activity(self):
        self.assertIn('case "swipe_up_right":', CTRL)
        self.assertIn('requestVehicleAction("reader:vehicle:engine");', CTRL)
        self.assertIn('button( "reader:vehicle:engine"', WORLD)
        self.assertIn("A11Y_INTERNAL_VEHICLE_ENGINE", ENGINE)
        self.assertIn("car.start_engines( vehicle_map, &player_character );", ENGINE)
        self.assertIn("car.stop_engines( vehicle_map );", ENGINE)
        self.assertIn('_( "Starting engine." )', ENGINE)

    def test_horn_only_announces_failure(self):
        self.assertIn("honk_horn( vehicle_map, true );", ENGINE)
        self.assertIn("bool quiet_success = false", VEH_HEADER)
        self.assertIn("if( !quiet_success )", VEH_USE)
        self.assertIn('_( "Horn unavailable." )', VEH_USE)

    def test_brief_vehicle_feedback_and_correct_unit_preference(self):
        self.assertIn('game_speed_units == "mph"', WORLD)
        self.assertIn('game_speed_units == "km/h"', WORLD)
        self.assertIn('_( "Speed: %1$d %3$s. Target: %2$d %3$s." )', WORLD)
        self.assertIn('requestLiveWorldCommand("reader:vehicle:begin")', CTRL)
        self.assertIn("R.string.a11yDriveBraking", CTRL)
        self.assertIn('document( "reader:vehicle:summary"', WORLD)
        self.assertIn('speakVehicleDocument("reader:vehicle:detail")', CTRL)
        self.assertIn('_( "Vehicle not moving. Check engine, fuel or wheels." )', VEH)

    def test_manual_mode_toggle_precedes_final_read_only_guide(self):
        self.assertIn("bool a11y_manual_drive = false;", VEH_HEADER)
        menu = VEH_USE.split("void vehicle::build_interact_menu(", 1)[1].split(
            "void vehicle::interact_with(", 1
        )[0]
        self.assertIn('_( "Driving: Manual. Switch to automatic" )', menu)
        self.assertIn('_( "Driving: Automatic. Switch to manual" )', menu)
        self.assertIn("a11y_manual_drive = !a11y_manual_drive;", menu)
        self.assertIn(".keep_menu_open()", menu)

        self.assertGreater(menu.index("a11y_manual_drive = !a11y_manual_drive;"),
                           menu.index("FOLD_VEHICLE"))
        self.assertIn("a11y_continuous_drive = false;", menu)
        self.assertIn("a11y_drive_paused = false;", menu)
        self.assertIn("a11y_obstacle_override = false;", menu)
        self.assertIn('menu.add( _( "Gesture guide" ) )', menu)
        self.assertGreater(menu.index('menu.add( _( "Gesture guide" ) )'),
                           menu.index("a11y_manual_drive = !a11y_manual_drive;"))
        self.assertIn("guide.addentry( index++, true, 0, line );", menu)

    def test_manual_mode_spends_one_turn_without_autopilot(self):
        self.assertIn("!seat->vehicle().a11y_manual_drive", ENGINE)
        self.assertIn("car.a11y_continuous_drive = false;", ENGINE)
        self.assertIn("!car.a11y_manual_drive && obstruction &&", ENGINE)
        self.assertIn("!car.a11y_manual_drive && !car.a11y_obstacle_override", ENGINE)
        self.assertIn("!seated->vehicle().a11y_manual_drive", ENGINE)
        self.assertIn("player_character.set_moves(", ENGINE)
        self.assertIn("!car.a11y_manual_drive && saved_speed != 0", ENGINE)

    def test_manual_mode_warns_without_applying_sensor_brakes(self):
        self.assertIn("if( a11y_manual_drive && !is_autodriving ) {", VEH)
        sensor = VEH.split("if( a11y_manual_drive && !is_autodriving ) {", 1)[1].split(
            "} else if( is_autodriving ) {", 1
        )[0]
        self.assertIn('_( "Obstacle ahead: %1$s, %2$d tiles." )', sensor)
        self.assertIn('_( "Obstacle behind: %1$s, %2$d tiles." )', sensor)
        self.assertNotIn("handbrake(", sensor)
        self.assertNotIn("cruise_velocity = 0", sensor)
        self.assertIn("!a11y_brake_alerted", sensor)
        self.assertIn("a11y_stopped_heading != turn_dir", sensor)
        self.assertIn('_( "Driving mode: Manual." )', WORLD)
        self.assertIn('_( "Driving mode: Automatic." )', WORLD)

    def test_continuous_driving_ticks_without_repeated_swipes(self):
        self.assertIn("bool a11y_continuous_drive = false;", VEH_HEADER)
        self.assertIn("a11y_continuous_vehicle_ready( avatar &driver, map &here )", ENGINE)
        self.assertIn("continuous_drive_tick_due()", ENGINE)
        self.assertIn("std::chrono::milliseconds( 350 )", ENGINE)
        self.assertIn("current_turn.has_timeout_elapsed() || continuous_drive_tick_due()", ENGINE)
        self.assertIn("car.cruise_velocity > 0;", ENGINE)
        self.assertIn("car.cruise_velocity < 0;", ENGINE)
        self.assertIn("a11y_continuous_drive = false;", VEH)
        self.assertIn("a11y_drive_stall_ticks >= 6", VEH)
        self.assertIn("seat->vehicle().cruise_velocity == 0 && seat->vehicle().velocity != 0", ENGINE)
        self.assertIn("car.a11y_continuous_drive = car.velocity != 0;", ENGINE)

    def test_swipe_forward_again_overrides_only_current_obstacle(self):
        self.assertIn("const bool force_forward =", ENGINE)
        self.assertIn("car.a11y_obstacle_stop &&", ENGINE)
        self.assertIn("car.a11y_obstacle_override = true;", ENGINE)
        self.assertIn("car.a11y_override_obstacle_name = stopped_hazard->label", ENGINE)
        self.assertIn("car.a11y_stopped_heading == car.turn_dir", ENGINE)
        self.assertIn("stopped_hazard->label == car.a11y_stopped_obstacle_name", ENGINE)
        self.assertIn("a11y_stopped_obstacle_name = hazard->label;", VEH)
        self.assertIn("a11y_stopped_heading = turn_dir;", VEH)
        self.assertIn("if( !car.a11y_manual_drive && !car.a11y_obstacle_override &&", ENGINE)
        self.assertIn("if( !a11y_obstacle_override && hazard &&", VEH)
        self.assertIn("hazard->label != a11y_override_obstacle_name", VEH)
        self.assertIn("a11y_obstacle_stop = true;", VEH)
        self.assertIn("a11y_obstacle_resume_speed", VEH)
        self.assertIn("a11y_brake_alerted = true;", VEH)
        self.assertIn("a11y_continuous_drive = car.velocity != 0;", ENGINE)
        self.assertIn("a11y_obstacle_override = false;", VEH)

    def test_braking_or_leaving_drive_mode_stops_continuous_turns(self):
        self.assertIn('action == "A11Y_INTERNAL_VEHICLE_BRAKE"', ENGINE)
        self.assertIn("seat->vehicle().a11y_continuous_drive = seat->vehicle().velocity != 0;", ENGINE)
        self.assertIn('requestVehicleAction("reader:vehicle:brake");', CTRL)
        self.assertIn('requestLiveWorldCommand("reader:vehicle:release")', CTRL)
        self.assertIn("car.a11y_drive_paused = true;", ENGINE)
        self.assertIn("car.a11y_release_controls_pending = true;", ENGINE)
        self.assertIn("car.cruise_velocity = 0;", ENGINE)
        self.assertIn("car.velocity == 0 || !u.controlling_vehicle", ENGINE)
        self.assertIn("u.controlling_vehicle = false;", ENGINE)
        self.assertIn("a11y_manual_drive && !a11y_release_controls_pending", VEH)
        self.assertIn("bool a11y_release_controls_pending = false;", VEH_HEADER)
        self.assertIn("!vehicleReleasePending && !vehicleDriverReady()", CTRL)

    def test_throttle_swipes_advance_native_physics(self):

        drive = ENGINE.split("void game::move_or_drive(", 1)[1].split(
            "    } else {\n        const int pre_walk_moves", 1
        )[0]
        self.assertIn("player_character.set_moves(", drive)
        self.assertIn("std::min( player_character.get_moves(), 0 )", drive)
        self.assertIn("throttle == a11y_vehicle_throttle::forward", drive)
        self.assertIn("throttle == a11y_vehicle_throttle::reverse", drive)
        self.assertIn("throttle == a11y_vehicle_throttle::faster", drive)
        self.assertIn("car.cruise_velocity != 0", drive)
        self.assertIn("pldrive( point_rel_ms( 0, -1 ) );", drive)

    def test_steering_obstacle_brake_is_checked_before_world_tick(self):
        drive = ENGINE.split("void game::move_or_drive(", 1)[1].split(
            "    } else {\n        const int pre_walk_moves", 1
        )[0]
        self.assertIn("Steering changes turn_dir before the world's next physics", drive)
        self.assertIn("a11y::vehicle_obstacle_ahead( car, here, reverse );", drive)
        self.assertIn("handbrake( here );", drive)
        self.assertIn("car.a11y_brake_alerted = true;", drive)
        self.assertIn("car.velocity != 0 && !car.is_rotorcraft( here )", drive)
        self.assertIn("veh.coord_translate( veh.turn_dir, pivot, part.mount,", WORLD)

    def test_heading_swipe_announces_short_actual_heading_and_blockers(self):
        self.assertIn('document( "reader:vehicle:heading"', WORLD)
        for compass in ("north", "northeast", "east", "southeast",
                        "south", "southwest", "west", "northwest"):
            self.assertIn('_( "' + compass + '" )', WORLD)
        heading = WORLD.split("const auto spoken_heading = [](", 1)[1].split(
            'document( "reader:vehicle:heading"', 1
        )[0]
        self.assertIn("spoken_heading( veh.face.dir() )", heading)
        self.assertNotIn("spoken_heading( veh.turn_dir )", heading)
        self.assertNotIn('_( "Requested heading: %s." )', heading)
        self.assertIn("veh.steering_effectiveness( get_map() )", heading)
        self.assertIn("veh.skidding", heading)
        self.assertIn("veh.velocity == 0 && veh.turn_dir != veh.face.dir()", heading)
        self.assertIn('next.find("reader:vehicle:heading")', CTRL)
        self.assertNotIn("pendingVehicleRoadReport", CTRL)
        self.assertIn('document( "reader:vehicle:road"', WORLD)

    def test_two_fingers_scan_vehicle_relative_and_bent_one_finger_reads_status(self):
        self.assertIn("handleVehicleTwoFingerSwipe(", SURF)
        self.assertLess(
            SURF.index("listener.handleVehicleTwoFingerSwipe("),
            SURF.index("listener.scanDirection("))
        self.assertIn("scanDirection(horizontal, vertical);", CTRL)
        self.assertIn('"reader:scan:vehicle:" + relative', CTRL)

        self.assertIn("octants[( facing + offset ) % 8]", WORLD)
        self.assertIn("a11y::announce_vehicle_relative_scan(", ENGINE)
        self.assertIn('intent.node.rfind( "reader:scan:vehicle:", 0 ) == 0', ENGINE)
        self.assertIn("std::max( std::abs( delta.x ), std::abs( delta.y ) ) != 1", WORLD)
        for side in ("front", "back", "left", "right"):
            self.assertIn('reader:scan:vehicle:' + side, WORLD)

        vehicle = CTRL.split(
            "if (vehicleMode && directWorldGesturesEnabled()) {", 1
        )[1]
        for gesture, action in (
            ("swipe_down_left", 'speakVehicleDocument("reader:vehicle:speed")'),
            ("swipe_down_right", 'speakVehicleDocument("reader:vehicle:detail")'),
            ("swipe_up_left", 'requestVehicleAction("reader:vehicle:brake")'),
            ("swipe_right_down", 'requestVehicleAction("reader:vehicle:horn")'),
        ):
            segment = vehicle.split('case "' + gesture + '":', 1)[1].split(
                "return true;", 1
            )[0]
            self.assertIn(action, segment)

    def test_engine_horn_brake_keep_original_game_validation(self):
        self.assertIn("act = ACTION_SMASH;", ENGINE)
        self.assertIn("seated->vehicle().honk_horn( vehicle_map, true );", ENGINE)
        self.assertIn("player_character.controlling_vehicle", ENGINE)

    def test_remote_discovery_reports_diagnostic_limits(self):
        self.assertIn("scan_unrestricted_tile(", WORLD)
        self.assertIn("Cannot drive: working controls are missing.", WORLD)
        self.assertIn("Cannot start: no usable engine.", WORLD)
        self.assertIn("Driving is not guaranteed.", WORLD.replace(
            "driving is not guaranteed.", "Driving is not guaranteed."))

    def test_approach_and_boarding_are_distinct(self):
        control = GAME.split("void game::control_vehicle(", 1)[1].split(
            "bool game::npc_menu(", 1
        )[0]
        self.assertIn("Control Vehicle always searches", control)
        self.assertIn("if( a11y::enabled() && !p && !remoteveh() )", control)
        self.assertIn("const optional_vpart_position aboard = here.veh_at( origin );",
                      control)
        self.assertIn("u.in_vehicle && aboard &&", control)
        self.assertIn("&found->vehicle() != &aboard->vehicle()", control)
        self.assertNotIn("std::nullopt;", control.split(
            "const optional_vpart_position aboard", 1
        )[1].split("std::vector<tripoint_bub_ms> seats;", 1)[0])
        self.assertIn("std::stable_sort( seats.begin(), seats.end()", control)
        self.assertIn("get_passenger( boardable->part_index() )", control)
        self.assertIn("occupant && occupant != &u", control)
        self.assertIn("safe_route_to(", control)
        self.assertIn("u.set_destination( *route );", control)
        self.assertIn("here.board_vehicle( selected, &u );", control)
        self.assertIn("if( !u.in_vehicle )", control)
        self.assertNotIn("choices.query();", control.split(
            "if( !p && !here.veh_at( u.pos_bub() ) )", 1
        )[0])
        self.assertIn("Cannot move forward: %1$s", ENGINE)
        self.assertIn("Cannot reverse: %1$s", ENGINE)

    def test_obstacle_sensing_ignores_player_visibility(self):
        self.assertIn("veh.immediate_path( &here, desired_delta,", WORLD)
        self.assertIn("horizon, reverse )", WORLD)
        self.assertIn("veh.bub_part_pos( here, i ).xy()", WORLD)
        self.assertIn("const point_rel_ms left = reverse ? mount_min : front_left;", VEH)

        self.assertIn(
            "vehicle_autodrive::classify_tile( veh, here, tile, driver, mobility )",
            WORLD)
        self.assertIn("veh.has_sufficient_rotorlift( here ), true", WORLD)
        self.assertIn("mobility.unrestricted_creatures ||", AUTODRIVE)

        self.assertIn("&other->vehicle() == &driven_veh", AUTODRIVE)
        self.assertIn("a11y::vehicle_obstacle_ahead( *this, here, reverse )", VEH)
        self.assertIn('document( "reader:vehicle:road"', WORLD)

    def test_automatic_brake_is_restricted_to_local_player(self):
        self.assertIn("driver.in_vehicle &&", VEH)
        self.assertIn("driver.controlling_vehicle", VEH)
        self.assertIn("velocity != 0 )", VEH)
        self.assertIn("const bool reverse = velocity < 0 ||", VEH)
        self.assertIn("std::clamp( 8 + std::abs( velocity ) / 500, 8, 32 )", VEH)
        self.assertIn("handbrake( here );", VEH)
        self.assertIn("velocity != 0 )", VEH)
        self.assertIn("a11y_brake_alerted", VEH)
        self.assertIn("a11y_skid_alerted", VEH)
        self.assertIn("Warning: vehicle is skidding; steering may not respond.", VEH)
        self.assertIn("std::abs( velocity ) / 500", VEH)
        self.assertIn("reverse ? 180_degrees : 0_degrees", WORLD)
        self.assertIn("requested_distance", VEH)

    def test_double_tap_pauses_only_in_live_drive(self):
        self.assertIn("if (vehicleMode && directWorldGesturesEnabled()) {", CTRL)
        self.assertIn('requestVehicleAction("reader:vehicle:pause");', CTRL)
        self.assertIn("listener.activate();", SURF)
        self.assertIn('button( "reader:vehicle:pause"', WORLD)
        self.assertIn('intent.node == "reader:vehicle:pause"', ENGINE)
        self.assertIn('action == "A11Y_INTERNAL_VEHICLE_PAUSE"', ENGINE)
        self.assertIn("car.a11y_drive_paused = true;", ENGINE)
        self.assertIn("car.a11y_drive_paused = false;", ENGINE)
        self.assertIn("car.a11y_resume_cruise_velocity", ENGINE)
        self.assertIn("car.velocity != 0", ENGINE)
        self.assertIn("car.cruise_velocity = std::clamp(", ENGINE)
        self.assertIn("hazard->distance <= distance", ENGINE)
        self.assertIn("act = ACTION_SMASH;", ENGINE)

    def test_drive_long_press_opens_real_vehicle_menu(self):
        self.assertIn("public void onLongPress(MotionEvent event)", SURF)
        self.assertIn("listener.openContextActions();", SURF)
        part = CTRL.split("public void openContextActions()", 1)[1]
        part = part.split("private void closeContextActions()", 1)[0]
        self.assertIn("if (vehicleMode && directWorldGesturesEnabled())", part)
        self.assertIn('snapshot.find("control_vehicle")', part)
        self.assertIn('requestLiveWorldCommand("control_vehicle");', part)
        self.assertNotIn("showComprehensiveMenu();", part.split(
            "if (vehicleMode && directWorldGesturesEnabled())", 1)[1].split(
                "if (combatMode && directWorldGesturesEnabled())", 1)[0])

    def test_stopped_or_paused_car_rotates_its_actual_body(self):
        self.assertIn("seated->vehicle().a11y_drive_paused ||", ENGINE)
        self.assertIn("seated->vehicle().velocity == 0", ENGINE)
        self.assertIn("direction.x() != 0 && direction.y() == 0", ENGINE)
        self.assertIn("seated->vehicle().a11y_rotate_in_place( here, steps );", ENGINE)
        self.assertIn("const int steps = direction.x() * std::max( 3, nudge );", ENGINE)
        self.assertIn("bool vehicle::a11y_rotate_in_place(", VEH_MOVE)
        self.assertIn("here.displace_vehicle( *this, tripoint_rel_ms::zero )", VEH_MOVE)
        self.assertIn("face.init( target );", VEH_MOVE)
        self.assertIn("move.init( target );", VEH_MOVE)
        self.assertIn("turn_dir = target;", VEH_MOVE)
        self.assertIn("creature && creature != &get_avatar()", VEH_MOVE)
        self.assertIn("part.info().location != vpart_location_structure", VEH_MOVE)
        self.assertIn("if( velocity != 0 )", VEH_MOVE)
        self.assertIn("is_towed() || is_towing()", VEH_MOVE)
        self.assertIn("steering_effectiveness( here )", VEH_MOVE)
        self.assertIn("Cannot turn in place: %s is in the way.", VEH_MOVE)
        self.assertIn("vehicle_autodrive::classify_tile( *this, here, tile,", VEH_MOVE)
        self.assertIn("const bool preview_only", VEH_MOVE)
        self.assertIn("if( preview_only )", VEH_MOVE)
        self.assertNotIn("here.ter( tile ).obj().is_smashable()", VEH_MOVE)
        self.assertIn('"AUTODRIVE_PIVOT_BLOCK"', VEH_MOVE)
        self.assertIn('" terrain_name=" + terrain_name', VEH_MOVE)
        self.assertIn('" blocker_name=" + obstacle_name', VEH_MOVE)
        self.assertIn("here.tr_at( tile ).name()", VEH_MOVE)
        self.assertIn("here.furnname( tile )", VEH_MOVE)
        self.assertIn("here.tername( tile )", VEH_MOVE)
        self.assertIn('" tile=" + tile.to_string()', VEH_MOVE)
        self.assertIn('" heading_deg="', VEH_MOVE)
        for category in ("map_boundary", "other_vehicle", "creature", "visible_trap",
                         "furniture", "terrain", "broken_steering_or_wheels"):
            self.assertIn('"' + category + '"', VEH_MOVE)
        self.assertIn("record_block( blocker_kind( obstacle ), tile, step, heading );", VEH_MOVE)

    def test_auto_sensor_samples_all_tiles_and_warns_about_brush(self):
        path = VEH.split("std::set<point_abs_ms> vehicle::immediate_path(", 1)[1].split(
            "static int get_turn_from_angle", 1
        )[0]
        self.assertIn("collision_vector.clear_advance();", path)
        self.assertIn("collision_vector.advance( 1 );", path)
        self.assertNotIn("collision_vector.advance( i );", path)

        self.assertIn(
            "vehicle_autodrive::classify_tile( veh, here, tile, driver, mobility )",
            WORLD)
        self.assertIn("terrain_type.is_smashable()", AUTODRIVE)

        self.assertIn("furniture.obj().movecost != 0", AUTODRIVE)
        self.assertIn("terrain_type.has_flag( ter_furn_flag::TFLAG_NOCOLLIDE )",
                      AUTODRIVE)
        self.assertIn("a11y::vehicle_obstacle_ahead( *this, here, reverse )", VEH)
        self.assertIn("a11y_obstacle_stop = true;", VEH)

    def test_manual_and_automatic_drive_are_silent_except_warnings(self):
        self.assertIn("worldView && !vehicleMode && previousTile != null", CTRL)
        self.assertIn("static boolean allowedDuringDriving(", CTRL)
        self.assertIn("&& !allowedDuringDriving(message, priority)", CTRL)
        self.assertIn("priority >= ReaderSpeechQueue.URGENT", CTRL)
        requests = CTRL.split("private void requestVehicleAction(", 1)[1].split(
            "private void speakVehicleDocument(", 1
        )[0]
        self.assertNotIn('|| "reader:vehicle:forward".equals(id)', requests)
        self.assertNotIn('|| "reader:vehicle:reverse".equals(id)', requests)
        self.assertIn("requestLiveWorldCommand(id);", requests)

        self.assertIn("if( traveler.in_vehicle && traveler.controlling_vehicle ) {",
                      WORLD)
        self.assertIn('a11y::android::announce( "A11Y_AREA:" + spoken, 1 );',
                      WORLD)

    def test_reversing_starts_at_rear_and_clearance_uses_body(self):
        self.assertIn("const point_rel_ms left = reverse ? mount_min : front_left;", VEH)
        self.assertIn("point_rel_ms( mount_min.x(), mount_max.y() )", VEH)
        self.assertIn("horizon, reverse )", WORLD)
        self.assertIn("veh.bub_part_pos( here, i ).xy()", WORLD)
        self.assertIn("const int distance = clearance( tile_xy );", WORLD)
        self.assertNotIn("rl_dist( origin, tile_xy )", WORLD)

    def test_vietnamese_vehicle_announcement_and_damage_terms(self):
        translations = (ROOT / "lang/po/accessibility/vi.po").read_text("utf-8")
        for phrase in (
            "Requested heading: %s.",
            "Calculated safe speed: %1$d %2$s.",
            "Intact wheels: %1$d of %2$d.",
            "Intact engines: %1$d of %2$d.",
            "Automatic braking: %1$s behind, about %2$d tiles.",
            "No boardable driver seats found within 16 tiles.",
        ):
            self.assertIn('msgid "' + phrase + '"', translations)
        self.assertIn("veh.steering_effectiveness( get_map() )", WORLD)
        self.assertIn("part.info().has_flag( VPFLAG_WHEEL )", WORLD)

    def test_heading_is_deferred_and_speed_read_explicitly(self):
        self.assertIn("pendingVehicleHeadingReport", CTRL)
        self.assertIn('next.find("reader:vehicle:heading")', CTRL)
        self.assertIn('speakVehicleDocument("reader:vehicle:speed")', CTRL)
        self.assertNotIn("pendingVehicleSpeedReport", CTRL)
        self.assertIn("if (vehicleMode && directWorldGesturesEnabled())", CTRL)


if __name__ == "__main__":
    unittest.main()
