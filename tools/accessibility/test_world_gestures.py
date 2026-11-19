"""Source contracts for the direct four-way Android reader gesture design."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
GESTURES = JAVA / "ReaderGestureSurface.java"
CONTROLLER = JAVA / "GameReaderController.java"
WORLD = ROOT / "src/a11y_world.cpp"
HANDLER = ROOT / "src/handle_action.cpp"


class WorldGestureContracts(unittest.TestCase):
    def test_cardinal_swipes_need_no_fast_mode_opt_in(self):
        gestures = GESTURES.read_text(encoding="utf-8")
        controller = CONTROLLER.read_text(encoding="utf-8")
        self.assertIn("listener.moveDirection(dx > 0 ? 1 : -1, 0)", gestures)
        self.assertIn("listener.moveDirection(0, dy > 0 ? 1 : -1)", gestures)
        guard = controller.split("private boolean worldStepAllowed()", 1)[1]
        guard = guard.split(
            "private static boolean hasCardinalDirections", 1
        )[0]
        self.assertIn("snapshot.swipeMovementAllowed", guard)
        self.assertIn("hasCardinalDirections(snapshot)", guard)


        self.assertIn("return !combatMode && !vehicleMode && !autonomousDriveTrip", guard)
        self.assertIn("&& worldStepAllowed();", guard)
        self.assertNotIn("preferences.isFastMovementEnabled()", guard)
        self.assertNotIn("hasCompletedMovementPractice()", guard)

    def test_multitap_sequences_are_mutually_exclusive(self):
        gestures = GESTURES.read_text(encoding="utf-8")
        self.assertIn("queueMultiTap(owner, maxPointers)", gestures)
        self.assertIn("dispatchMultiTap(fingers, 3)", gestures)
        self.assertIn("listener.twoFingerTap(count)", gestures)
        self.assertIn("listener.threeFingerTap()", gestures)
        self.assertIn("ViewConfiguration.getDoubleTapTimeout()", gestures)
        self.assertIn('if (maxPointers == 2)', gestures)
        self.assertIn('listener.scanDirection(', gestures)
        self.assertIn('maxPointers, direction', gestures)
        self.assertIn("maxPointers <= 4", gestures)
        self.assertIn("listener.handleCustomGesture(gesture)", gestures)
        self.assertNotIn("listener.stopSpeech();\\n", gestures)

    def test_unrestricted_scan_is_ordered_and_read_only(self):
        world = WORLD.read_text(encoding="utf-8")
        scan = world.split("void announce_linear_scan(", 1)[1]
        scan = scan.split("void announce_player_coordinates()", 1)[0]
        self.assertIn("distance = 1; distance <= scan_range; ++distance", scan)
        self.assertIn("constexpr int scan_range = 30;", scan)
        self.assertIn("scan_unrestricted_tile( pos )", scan)
        helper = world.split("unrestricted_tile scan_unrestricted_tile(", 1)[1]
        self.assertIn("here.tername( position )", helper)
        self.assertIn("get_creature_tracker().creature_at( position, true )", helper)
        self.assertIn("here.i_at( position )", helper)
        self.assertIn("spoken_item_name( obj )", helper)
        self.assertIn("for( const std::string &name : tile.items )", scan)
        self.assertIn("if( details.empty() )", scan)
        self.assertIn('_( "No noteworthy information within 30 tiles." )', scan)
        self.assertNotIn("detail = tile.terrain;", scan)
        self.assertIn("here.has_flag( ter_furn_flag::TFLAG_GOES_UP, pos )", scan)
        self.assertIn("tile.impassable", scan)
        self.assertIn("std::set<const vehicle *> announced_vehicles;", scan)
        self.assertIn("announced_vehicles.insert( &found_vehicle->vehicle() ).second", scan)
        self.assertIn("tile.traps", scan)
        self.assertNotIn("observe_tile(", scan)
        self.assertIn("A11Y_SCAN:", scan)
        handler = HANDLER.read_text(encoding="utf-8")
        self.assertIn("intent.node == \"reader:scan:north\"", handler)
        self.assertIn("a11y::announce_player_coordinates()", handler)

    def test_map_and_vehicle_scans_keep_their_existing_gestures(self):
        controller = CONTROLLER.read_text(encoding="utf-8")
        world = WORLD.read_text(encoding="utf-8")
        handler = HANDLER.read_text(encoding="utf-8")
        for direction in ("north", "south", "west", "east"):
            self.assertIn('"reader:scan:' + direction + '"', controller)
            self.assertIn('"Scan 30 tiles ' + direction + '"', world)
            self.assertIn('intent.node == "reader:scan:' + direction + '"', handler)



        self.assertIn('intent.node.rfind( "reader:scan:vehicle:", 0 ) == 0', handler)
        self.assertIn("a11y::announce_vehicle_relative_scan(", handler)
        for direction in ("front", "back", "left", "right"):
            self.assertIn('"reader:scan:vehicle:' + direction + '"', world)
        self.assertIn("scanDirection(horizontal, vertical);", controller)
        self.assertIn("octants[( facing + offset ) % 8]", world)

    def test_sparse_scan_has_vietnamese_empty_message(self):
        vi = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")
        self.assertIn('msgid "No noteworthy information within 30 tiles."', vi)
        self.assertIn(
            'msgstr "Không phát hiện gì đáng chú ý trong phạm vi 30 ô."', vi)
        self.assertIn('msgstr "Ô %1$d: %2$s."', vi)

    def test_coordinate_readout_uses_known_overmap_area_with_safe_fallback(self):
        world = WORLD.read_text(encoding="utf-8")
        reader = world.split("void announce_player_coordinates()", 1)[1].split(
            "semantic_snapshot activity_snapshot(", 1
        )[0]


        self.assertIn('_( "Coordinates.  X %1$d. Y %2$d. Z %3$d." )', reader)
        self.assertIn("viewer.pos_abs_omt()", reader)
        self.assertIn("overmap_buffer.seen( omt )", reader)
        self.assertIn("vision != om_vision_level::unseen", reader)
        self.assertIn("overmap_buffer.ter_existing( omt )", reader)
        self.assertNotIn("overmap_buffer.ter( omt )", reader)
        self.assertIn("terrain.is_valid()", reader)
        self.assertIn("terrain->get_name( vision )", reader)
        self.assertIn('_( "Area: %1$s. %2$s" )', reader)


        self.assertIn('a11y::android::announce( "A11Y_QUERY:" + spoken, 1 );', reader)
        po = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")
        self.assertIn('msgid "Area: %1$s. %2$s"', po)
        self.assertIn('msgstr "Khu vực: %1$s. %2$s"', po)

    def test_reader_hub_is_on_demand_without_the_old_dashboard(self):
        controller = CONTROLLER.read_text(encoding="utf-8")
        self.assertTrue((JAVA / "GameActionHub.java").exists())
        self.assertIn("android.graphics.Color.TRANSPARENT", controller)
        self.assertIn("public void showComprehensiveMenu()", controller)
        self.assertIn("private void renderHub()", controller)
        self.assertIn("else if (comprehensiveMenu)", controller)
        self.assertIn("root.setVisibility(View.GONE);", controller)
        self.assertNotIn("button(pages,", controller)
        self.assertNotIn("button(tools,", controller)

    def test_direct_shortcuts_include_the_restored_hub(self):
        controller = CONTROLLER.read_text(encoding="utf-8")
        self.assertIn("if (target == null) { return false; }", controller)
        self.assertIn("executeShortcutTarget(target);", controller)
        self.assertIn(
            'String id = target.substring("action:".length());',
            controller,
        )
        self.assertIn('target.startsWith("hub:group:")', controller)
        self.assertIn('send("activate", "");', controller)


if __name__ == "__main__":
    unittest.main()
