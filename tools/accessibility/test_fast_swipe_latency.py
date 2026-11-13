"""Regression contracts for fluent walking swipes in DEFAULTMODE."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class FastSwipeLatencyContracts(unittest.TestCase):
    def test_world_frame_survives_game_turns(self):
        runtime = (SRC / "a11y_runtime.cpp").read_text(encoding="utf-8")
        engine = (SRC / "handle_action.cpp").read_text(encoding="utf-8")
        self.assertIn("std::unique_ptr<screen_scope> retained_world_scope;", runtime)
        self.assertIn("retained_world_scope->rebind( nullptr, {} );", runtime)
        self.assertIn("a11y::bind_world_screen( &ctxt,", engine)
        self.assertIn("a11y::unbind_world_screen();", engine)
        self.assertNotIn(
            'std::make_unique<a11y::screen_scope>( &ctxt, "DEFAULTMODE",',
            engine,
        )

    def test_world_movement_uses_native_permission_not_map_revision(self):
        runtime = (SRC / "a11y_runtime.cpp").read_text(encoding="utf-8")
        gate = (SRC / "a11y_world_input_gate.h").read_text(encoding="utf-8")
        self.assertIn("live_world_walk_command( screen.snapshot, *intent )", runtime)
        self.assertIn("contexts.accepts( intent->token )", runtime)
        self.assertIn("intent->request > screen.last_request", runtime)
        self.assertIn("intent.revision <= snapshot.revision", gate)
        self.assertIn('snapshot.category == "DEFAULTMODE"', gate)
        self.assertIn("snapshot.swipe_movement_allowed", gate)
        self.assertIn("intent.value.empty()", gate)
        self.assertIn("node->enabled", gate)
        self.assertNotIn("screen.revision - intent->revision == 1", runtime)
        self.assertIn("retained_world_scope.reset();", runtime)

    def test_hub_cancels_outstanding_footsteps_in_native_mailbox(self):
        bridge = (SRC / "a11y_android.cpp").read_text(encoding="utf-8")
        java = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        self.assertIn("cancel_pending_world_steps()", bridge)
        self.assertIn("std::remove_if( inbox.begin(), inbox.end()", bridge)
        self.assertIn("nativeCancelPendingWorldSteps()", java)
        self.assertIn("comprehensiveMenu = true;", java)

    def test_native_and_ui_wait_are_sampled(self):
        runtime = (SRC / "a11y_runtime.cpp").read_text(encoding="utf-8")
        java = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        self.assertIn('"WORLD_INPUT_TIMING"', runtime)
        self.assertIn("jni_to_poll_ms=", runtime)
        self.assertIn('"WORLD_JAVA_TIMING"', java)
        self.assertIn("jni_to_ui_ms=", java)
        self.assertIn("parse_ms=", java)

    def test_stable_gesture_context_during_footsteps(self):
        ctrl = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        self.assertIn('":screen:" + snapshot.session', ctrl)
        self.assertIn('snapshot.frame + ":" + snapshot.generation', ctrl)
        self.assertIn("worldMovementEnabled()", ctrl)
        self.assertIn("snapshot.swipeMovementAllowed", ctrl)

    def test_single_flight_accepts_no_steps_during_simulation(self):
        bridge = (SRC / "a11y_android.cpp").read_text(encoding="utf-8")
        context = (SRC / "input_context.cpp").read_text(encoding="utf-8")
        runtime = (SRC / "a11y_runtime.cpp").read_text(encoding="utf-8")
        self.assertIn("if( !world_step_ready ||", bridge)
        self.assertIn("const bool pending_step = std::any_of(", bridge)
        self.assertIn("world_step_ready = false;", bridge)
        self.assertIn("matches_last_world( intent.token )", bridge)
        self.assertIn("a11y::set_world_input_ready( this, true );", context)
        self.assertIn("a11y::set_world_input_ready( this, false );", context)
        self.assertIn('screens.back()->category == "DEFAULTMODE"', runtime)

    def test_no_new_movement_backlog_or_synthetic_steps(self):
        runtime = (SRC / "a11y_runtime.cpp").read_text(encoding="utf-8")
        engine = (SRC / "handle_action.cpp").read_text(encoding="utf-8")
        self.assertNotIn("pendingWalkingSteps", runtime)
        self.assertNotIn("queuedSwipeDirections", engine)
        self.assertIn("return action;", runtime)

    def test_command_description_cache_is_invalidated(self):
        inp = (SRC / "a11y_input.cpp").read_text(encoding="utf-8")
        runtime = (SRC / "a11y_runtime.cpp").read_text(encoding="utf-8")
        self.assertIn("cached_world_action_ids == actions", inp)
        self.assertIn('context.category_id() == "DEFAULTMODE"', inp)
        self.assertIn('if( screen_category != "DEFAULTMODE" )', runtime)
        self.assertIn("invalidate_command_snapshot_cache();", runtime)

    def test_detailed_reader_timing_is_manual_only(self):
        engine = (SRC / "handle_action.cpp").read_text(encoding="utf-8")
        self.assertIn('diagnostic_event( "WORLD_READER_TIMING"', engine)
        self.assertIn("build_ms=", engine)
        self.assertIn("update_publish_ms=", engine)
        self.assertIn("if( profile_reader ) {", engine)
        self.assertNotIn("reader_timing_sample % 8", engine)

    def test_ordinary_footsteps_do_not_allocate_three_translated_restriction_maps(self):
        engine = (SRC / "handle_action.cpp").read_text(encoding="utf-8")
        action = engine.split("bool game::do_regular_action(", 1)[1]
        movement = action.split("switch( act ) {", 1)[0]
        self.assertIn("if( in_shell ) {", movement)
        self.assertIn("if( player_character.is_mounted() ) {", movement)
        self.assertIn('if( u.has_flag( json_flag_TEMPORARY_SHAPESHIFT_NO_HANDS ) ) {', movement)
        self.assertIn("is_move_mode_action( act )", movement)
        self.assertNotIn("const std::vector<action_id> actions_move_mode", movement)
        self.assertNotIn("const std::map<action_id, std::string> actions_disabled_mounted =", movement)

    def test_map_shift_skips_empty_uniform_submap_item_scans(self):
        code = (SRC / "map.cpp").read_text(encoding="utf-8")
        reconcile = code.split("void map::reconcile_loaded_items(", 1)[1].split(
            "void map::shift_traps(", 1)[0]
        self.assertIn("sm == nullptr || sm->is_uniform()", reconcile)
        self.assertIn("for( int sx = 0; sx < SEEX; ++sx )", reconcile)
        # Vehicles, the character inventory and NPC inventories remain in
        # reconciliation even if ground submaps are uniform.
        self.assertIn("get_vehicles()", reconcile)
        self.assertIn("walk_character( get_avatar() );", reconcile)

    def test_uniform_inert_submaps_preserve_actualization_bookkeeping(self):
        code = (SRC / "map.cpp").read_text(encoding="utf-8")
        actualize = code.split("void map::actualize(", 1)[1].split(
            "void map::add_tree_tops(", 1)[0]
        self.assertIn("tmpsub->is_uniform()", actualize)
        self.assertIn("terrain == ter_t_rock_floor || terrain == ter_t_open_air", actualize)
        self.assertIn("tmpsub->last_touched = calendar::turn;", actualize)
        self.assertLess(actualize.index("process_items_in_submap("),
                        actualize.index("if( tmpsub->is_uniform() )"))
        self.assertLess(actualize.index("explosion_handler::process_explosions();"),
                        actualize.index("if( tmpsub->is_uniform() )"))
        self.assertIn("for( int x = 0; x < SEEX; x++ )", actualize)
        # The normal scene rebuild must remain; this is not a stale-vision
        # shortcut and does not remove cache invalidation on a map shift.
        self.assertIn("void map::build_map_cache(", code)
        self.assertIn("void map::invalidate_map_cache(", code)

    def test_slow_walk_profiler_is_gated_and_reports_map_shift_phases(self):
        game = (SRC / "game.cpp").read_text(encoding="utf-8")
        self.assertIn('diagnostic_event( "WORLD_WALK_PROFILE"', game)
        self.assertIn('diagnostic_event( "WORLD_PLACE_PROFILE"', game)
        self.assertIn('diagnostic_event( "WORLD_SHIFT_PROFILE"', game)
        for stage in ("before_place_ms=", "place_player_ms=", "after_place_ms=",
                      "update_map_ms=", "search_ms=", "map_shift_ms=",
                      "npcs_ms=", "cache_ms=", "spawn_ms=", "reveal_ms="):
            self.assertIn(stage, game)
        self.assertIn("if( a11y::android::performance_profiling_enabled() ) {", game)
        self.assertNotIn("profile_walk_samples % 8", game)
        self.assertNotIn("profile_place_samples % 8", game)
        self.assertNotIn("profile_shift_samples % 8", game)

    def test_native_step_result_uses_real_position(self):
        engine = (SRC / "handle_action.cpp").read_text(encoding="utf-8")
        self.assertIn('diagnostic_event( "WORLD_MOVE_RESULT"', engine)
        self.assertIn('std::optional<tripoint_abs_ms> diagnostic_before_position', engine)
        self.assertIn("if( diagnostic_walk ) {", engine)
        self.assertIn('const tripoint_abs_ms after = player_character.pos_abs();', engine)
        self.assertIn('" result=action_rejected changed=false"', engine)
        self.assertIn('" result=" + ( dx != 0 || dy != 0 || dz != 0', engine)
        for field in ("from_x=", "from_y=", "from_z=", "to_x=", "to_y=",
                      "to_z=", "moves_spent=", "action_ms="):
            self.assertIn(field, engine)

    def test_reader_step_redraws_only_invalidated_widgets(self):
        code = (SRC / "do_turn.cpp").read_text(encoding="utf-8")
        self.assertIn('if( a11y::enabled() ) {', code)
        self.assertIn('ui_manager::redraw_invalidated();', code)
        self.assertIn('diagnostic_event( "WORLD_DRAW_TIMING"', code)
        self.assertIn('diagnostic_event( "WORLD_SIM_TIMING"', code)
        for phase in ("before_cache_ms=", "cache_ms=", "monsters_ms=",
                      "player_turn_ms="):
            self.assertIn(phase, code)


if __name__ == "__main__":
    unittest.main()
