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
GAME = (ROOT / "src/game.cpp").read_text(encoding="utf-8")


class AccessibleVehicleContracts(unittest.TestCase):
    def test_three_modes_only_appear_when_on_board(self):
        self.assertIn('next.find("reader:vehicle:seat") != null', CTRL)
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
        for gesture, node in (
            ("swipe_up", "forward"), ("swipe_down", "reverse"),
            ("swipe_up_down", "faster"), ("swipe_down_up", "slower")
        ):
            drive = CTRL.split('case "' + gesture + '":', 1)[1].split(
                "return true;", 1
            )[0]
            self.assertIn('requestVehicleAction("reader:vehicle:' + node + '")',
                          drive)
            self.assertIn('reader:vehicle:' + node + '"', WORLD)
        self.assertIn("a11y_vehicle_throttle::reverse", ENGINE)
        self.assertIn("a11y_vehicle_throttle::faster", ENGINE)
        self.assertIn("car.cruise_thrust( here, step );", ENGINE)

    def test_two_fingers_are_mode_scoped_before_map_scans(self):
        self.assertIn("handleVehicleTwoFingerSwipe(", SURF)
        self.assertLess(
            SURF.index("listener.handleVehicleTwoFingerSwipe("),
            SURF.index("listener.scanDirection("))
        self.assertIn('requestVehicleAction("reader:vehicle:brake")', CTRL)
        self.assertIn('requestVehicleAction("reader:vehicle:horn")', CTRL)
        self.assertIn('speakVehicleDocument("reader:vehicle:speed")', CTRL)
        self.assertIn('speakVehicleDocument("reader:vehicle:detail")', CTRL)

    def test_engine_horn_brake_keep_original_game_validation(self):
        self.assertIn("act = ACTION_SMASH;", ENGINE)
        self.assertIn("seated->vehicle().honk_horn( vehicle_map );", ENGINE)
        self.assertIn("player_character.controlling_vehicle", ENGINE)

    def test_remote_discovery_reports_diagnostic_limits(self):
        self.assertIn("scan_unrestricted_tile(", WORLD)
        self.assertIn("Cannot drive: working controls are missing.", WORLD)
        self.assertIn("Cannot start: no usable engine.", WORLD)
        self.assertIn("Driving is not guaranteed.", WORLD.replace(
            "driving is not guaranteed.", "Driving is not guaranteed."))

    def test_approach_and_boarding_are_distinct(self):
        self.assertIn("Find a vehicle and board the driver's seat", GAME)
        self.assertIn("Interact with nearby vehicle controls", GAME)
        self.assertIn("choices.query();", GAME)
        self.assertIn("safe_route_to(", GAME)
        self.assertIn("u.set_destination( *route );", GAME)
        self.assertIn("here.board_vehicle( selected, &u );", GAME)

    def test_obstacle_sensing_ignores_player_visibility(self):
        self.assertIn("veh.immediate_path( &here )", WORLD)
        self.assertIn("here.impassable_ter_furn( tile )", WORLD)
        self.assertIn("&other->vehicle() == &veh", WORLD)
        self.assertIn("vehicle_obstacle_ahead( *this, here )", VEH)
        self.assertIn('document( "reader:vehicle:road"', WORLD)

    def test_automatic_brake_is_restricted_to_local_player(self):
        self.assertIn("driver.in_vehicle &&", VEH)
        self.assertIn("driver.controlling_vehicle", VEH)
        self.assertIn("velocity > 0 && cruise_velocity > 0", VEH)
        self.assertIn("std::clamp( 8 + velocity / 800, 8, 24 )", VEH)
        self.assertIn("handbrake( here );", VEH)

    def test_speech_is_deferred_to_next_vehicle_snapshot(self):
        self.assertIn("pendingVehicleRoadReport", CTRL)
        self.assertIn("pendingVehicleSpeedReport", CTRL)
        self.assertIn('next.find("reader:vehicle:speed")', CTRL)
        self.assertIn('next.find("reader:vehicle:road")', CTRL)
        self.assertIn("if (vehicleMode && directWorldGesturesEnabled())", CTRL)


if __name__ == "__main__":
    unittest.main()
