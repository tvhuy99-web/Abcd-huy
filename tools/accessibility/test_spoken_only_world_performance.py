"""Source contracts for spoken-only world rendering and adaptive performance tracing.

These tests validate wiring and safety boundaries. Runtime speed and visual
correctness must still be checked on the ARM64 device.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


def native(name):
    return (SRC / name).read_text(encoding="utf-8")


def java(name):
    return (JAVA / name).read_text(encoding="utf-8")


class SpokenOnlyWorldContracts(unittest.TestCase):
    def test_render_path_skips_paint_not_gameplay_or_visibility(self):
        game = native("game.cpp")
        draw = game.split("void game::draw( ui_adaptor &ui )", 1)[1]
        draw = draw.split("void game::draw_panels(", 1)[0]
        self.assertLess(draw.index("here.build_map_cache("),
                        draw.index("suppress_world_visuals()"))
        self.assertLess(draw.index("here.update_visibility_cache("),
                        draw.index("suppress_world_visuals()"))
        self.assertIn("a11y::android::directional_sonar_targets( sonar.creatures, sonar.items );", draw)
        self.assertIn("a11y::scan_directional_sonar( u.pos_bub( here ) )", draw)
        self.assertIn("a11y::android::directional_sonar_enabled()", draw)
        self.assertIn("draw_ter();", draw)
        self.assertIn("draw_panels( true );", draw)
        self.assertIn('diagnostic_event( "WORLD_RENDER_BREAKDOWN"', draw)
        self.assertIn("visibility_ms=", draw)
        self.assertIn("terrain_ms=", draw)
        self.assertIn("panels_ms=", draw)
        self.assertIn("u.view_offset = real_offset;", draw)

        engine = native("handle_action.cpp")
        reader = engine.split("const auto refresh_reader = [&]() {", 1)[1]
        reader = reader.split("    user_turn current_turn;", 1)[0]
        self.assertIn("vision_map.build_map_cache( z );", reader)
        self.assertIn("vision_map.update_visibility_cache( z );", reader)
        self.assertLess(reader.index("vision_map.update_visibility_cache( z );"),
                        reader.index("make_reader_snapshot()"))
        self.assertIn("a11y::android::directional_sonar_targets( sonar.creatures, sonar.items );", reader)
        self.assertIn("a11y::scan_directional_sonar(", reader)
        observer = native("a11y_perception.cpp")
        self.assertIn("seen.source != observation_source::visible", observer)
        self.assertIn("const tile_observation seen = observe_tile( target );", observer)

    def test_no_headless_redraw_or_animation_loop_before_a_footstep(self):
        turns = native("do_turn.cpp")
        engine = native("handle_action.cpp")
        self.assertIn("if( !headless ) {", turns)
        self.assertIn("ui_manager::redraw_invalidated();", turns)
        self.assertIn("ui_manager::redraw();", turns)
        self.assertIn('diagnostic_event( "WORLD_DRAW_TIMING"', turns)
        self.assertIn("mode=headless_skip", turns)
        self.assertIn('get_option<bool>( "ANIMATIONS" ) && '
                      '!a11y::android::suppress_world_visuals()', engine)
        self.assertIn("refresh_reader();", engine)
        self.assertIn("handle_mouseview( ctxt, action )", engine)

    def test_runtime_switch_default_and_manual_logger(self):
        prefs = java("ReaderPreferences.java")
        reader = java("GameReaderController.java")
        bridge = native("a11y_android.cpp")
        game = java("CataclysmDDA.java")
        log = java("SessionDebugLog.java")
        self.assertIn('getBoolean("spoken_only_world", '
                      'BuildConfig.FLAVOR.equals("accessibility"))', prefs)
        self.assertNotIn("isPerformanceProfilingEnabled", prefs)
        self.assertIn("boolean profile = SessionDebugLog.isRecording()", reader)
        self.assertIn('getBoolean("enabled", false)', log)
        self.assertIn("nativeConfigureReaderPerformance(spokenOnlyWorld, profile)", reader)
        self.assertIn("nativeConfigureReaderPerformance(boolean spokenOnlyWorld,", game)
        self.assertIn("nativeConfigureReaderPerformance(", bridge)
        self.assertIn("no_world_graphics.store(", bridge)
        self.assertIn("performance_profile_active.exchange(", bridge)
        self.assertIn("enabled() && !suspended()", bridge)

    def test_detailed_performance_traces_never_auto_pause(self):
        bridge = native("a11y_android.cpp")
        engine = native("handle_action.cpp")
        turns = native("do_turn.cpp")
        self.assertIn("performance_step_sample( dx != 0 || dy != 0 || dz != 0 )", engine)
        self.assertIn("performance_step_sample( false );", engine)
        self.assertIn('diagnostic_event( "WORLD_STEP_PIPELINE"', bridge)
        self.assertIn("performance_reader_sample( const long long ms )", bridge)
        self.assertIn("pending_step_completed_at", bridge)
        self.assertIn("last_profile_sim_ms = std::max(", bridge)
        self.assertIn("performance_vision_sample( vision_ms )", engine)
        self.assertIn("performance_reader_sample( build_ms + publish_ms )", engine)
        self.assertIn("a11y::android::performance_world_shift();",
                      native("game.cpp"))
        self.assertIn("if( profile_reader ) {", engine)
        self.assertIn("if( world_profile ) {", turns)
        self.assertNotIn("stable_walking_steps", bridge)
        self.assertNotIn("auto_paused", bridge)
        self.assertNotIn("performance_profile_active.store( false", bridge)
        self.assertNotIn("reader_timing_sample % 8", engine)

    def test_all_measurements_and_writes_stop_on_manual_off(self):
        core = native("do_turn.cpp")
        engine = native("handle_action.cpp")
        runtime = native("a11y_runtime.cpp")
        bridge = native("a11y_android.cpp")
        log = java("SessionDebugLog.java")
        game = java("CataclysmDDA.java")
        reader = java("GameReaderController.java")
        self.assertIn("const bool profile = a11y::android::performance_profiling_enabled()", core)
        self.assertIn("const bool world_profile = a11y::android::performance_profiling_enabled()", core)
        self.assertIn("if( profile_reader ) {", engine)
        self.assertIn("android::performance_profiling_enabled()", runtime)
        self.assertIn("log.recording = enabled;", log)
        self.assertIn("if (!recording || closed) { return; }", log)
        self.assertIn("SessionDebugLog.isRecording()", reader)
        self.assertIn("SessionDebugLog.event(category, detail);", game)
        self.assertNotIn("setVerboseWorldTrace", log)
        self.assertNotIn("auto_paused", game)
        self.assertIn('if( !performance_profile_active.load( std::memory_order_acquire ) )',
                      bridge)
        self.assertNotIn('diagnostic_event( "WORLD_MOVE_BLOCKED"', engine)

    def test_single_manual_log_switch_and_independent_spoken_only_setting(self):
        ui = java("DebugLogActivity.java")
        graphics = java("AccessibilitySettingsActivity.java")
        self.assertIn("SessionDebugLog.setEnabled(this, checked)", ui)
        self.assertNotIn("spokenOnlyWorld = new CheckBox(this)", ui)
        self.assertIn("prefs.setSpokenOnlyWorldEnabled(spokenOnly)", graphics)
        self.assertIn("CataclysmDDA.nativeConfigureReaderPerformance(", graphics)
        self.assertNotIn("CheckBox profiling", ui)
        for locale in ("values", "values-vi"):
            strings = (ROOT / "android/app/src/main/res" / locale / "strings.xml").read_text(
                encoding="utf-8")
            self.assertIn('name="a11ySpokenOnlyWorld"', strings)
            self.assertIn('name="a11yDebugLogEnabled"', strings)


if __name__ == "__main__":
    unittest.main()
