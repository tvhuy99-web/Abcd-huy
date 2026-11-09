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

    def test_stale_revision_grace_is_cardinal_only(self):
        runtime = (SRC / "a11y_runtime.cpp").read_text(encoding="utf-8")
        self.assertIn('screen.category == "DEFAULTMODE"', runtime)
        self.assertIn("screen.snapshot.swipe_movement_allowed", runtime)
        self.assertIn('intent->action == "activate"', runtime)
        for direction in ("UP", "DOWN", "LEFT", "RIGHT"):
            self.assertIn('intent->node == "' + direction + '"', runtime)
        self.assertIn("screen.revision - intent->revision == 1", runtime)
        self.assertIn("contexts.accepts( intent->token )", runtime)
        self.assertIn("retained_world_scope.reset();", runtime)

    def test_stable_gesture_context_during_footsteps(self):
        ctrl = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        self.assertIn('":walk:" + snapshot.session', ctrl)
        self.assertIn('snapshot.frame + ":" + snapshot.generation', ctrl)
        self.assertIn("worldMovementEnabled()", ctrl)
        self.assertIn("snapshot.swipeMovementAllowed", ctrl)

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

    def test_timing_samples_are_bounded(self):
        engine = (SRC / "handle_action.cpp").read_text(encoding="utf-8")
        self.assertIn('diagnostic_event( "WORLD_READER_TIMING"', engine)
        self.assertIn("build_ms=", engine)
        self.assertIn("update_publish_ms=", engine)
        self.assertIn("reader_timing_sample % 32", engine)


if __name__ == "__main__":
    unittest.main()
