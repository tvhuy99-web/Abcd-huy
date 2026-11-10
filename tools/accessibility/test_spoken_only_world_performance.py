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
        self.assertIn("a11y::android::alignment_distance( nearest );", draw)
        self.assertIn("draw_ter();", draw)
        self.assertIn("draw_panels( true );", draw)
        self.assertIn("u.view_offset = real_offset;", draw)

        engine = native("handle_action.cpp")
        reader = engine.split("const auto refresh_reader = [&]() {", 1)[1]
        reader = reader.split("    user_turn current_turn;", 1)[0]
        self.assertIn("vision_map.build_map_cache( z );", reader)
        self.assertIn("vision_map.update_visibility_cache( z );", reader)
        self.assertLess(reader.index("vision_map.update_visibility_cache( z );"),
                        reader.index("make_reader_snapshot()"))

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

    def test_runtime_switch_default_and_jni_lifecycle(self):
        prefs = java("ReaderPreferences.java")
        reader = java("GameReaderController.java")
        bridge = native("a11y_android.cpp")
        game = java("CataclysmDDA.java")
        self.assertIn('getBoolean("spoken_only_world", '
                      'BuildConfig.FLAVOR.equals("accessibility"))', prefs)
        self.assertIn('getBoolean("adaptive_performance_profiling",', prefs)
        self.assertIn("nativeConfigureReaderPerformance(spokenOnlyWorld, profile)", reader)
        self.assertIn("nativeConfigureReaderPerformance(boolean spokenOnlyWorld,", game)
        self.assertIn("nativeConfigureReaderPerformance(", bridge)
        self.assertIn("no_world_graphics.store(", bridge)
        self.assertIn("performance_profile_active.exchange(", bridge)
        self.assertIn("enabled() && !suspended()", bridge)

    def test_auto_pause_requires_actual_fast_uninterrupted_walking(self):
        bridge = native("a11y_android.cpp")
        engine = native("handle_action.cpp")
        self.assertIn("performance_step_sample( dx != 0 || dy != 0 || dz != 0 )", engine)
        self.assertIn("performance_step_sample( false );", engine)
        self.assertIn("stable_walking_steps >= 48", bridge)
        self.assertIn("last_profile_draw_ms > 40", bridge)
        self.assertIn("last_profile_sim_ms > 120", bridge)
        self.assertIn("busy_epoch != last_profile_busy_epoch", bridge)
        self.assertIn("shifted_since_last_step", bridge)
        self.assertIn("profile_busy_epoch.fetch_add(", bridge)
        self.assertIn("performance_profile_active.store( false", bridge)
        self.assertIn("state=auto_paused stable_walks=48", bridge)
        self.assertIn("a11y::android::performance_world_shift();",
                      native("game.cpp"))

    def test_measurements_and_high_volume_logging_suspend_independently(self):
        core = native("do_turn.cpp")
        engine = native("handle_action.cpp")
        runtime = native("a11y_runtime.cpp")
        log = java("SessionDebugLog.java")
        game = java("CataclysmDDA.java")
        self.assertIn("const bool profile = a11y::android::performance_profiling_enabled()", core)
        self.assertIn("const bool world_profile = a11y::android::performance_profiling_enabled()", core)
        self.assertIn("if( profile_reader ) {", engine)
        self.assertIn("android::performance_profiling_enabled()", runtime)
        self.assertIn("isHighFrequencyTrace(category, detail)", log)
        self.assertIn('category.equals("SCREEN")', log)
        self.assertIn("Thread.setDefaultUncaughtExceptionHandler", log)
        self.assertIn("log.crashFile", log)
        self.assertIn("setVerboseWorldTrace(this, false)", game)

    def test_debug_screen_reversible_controls_are_localized(self):
        ui = java("DebugLogActivity.java")
        self.assertIn("reader.setSpokenOnlyWorldEnabled(checked)", ui)
        self.assertIn("reader.setPerformanceProfilingEnabled(checked)", ui)
        self.assertIn("SessionDebugLog.setVerboseWorldTrace(this, checked)", ui)
        for locale in ("values", "values-vi"):
            strings = (ROOT / "android/app/src/main/res" / locale / "strings.xml").read_text(
                encoding="utf-8")
            self.assertIn('name="a11ySpokenOnlyWorld"', strings)
            self.assertIn('name="a11yAdaptiveProfiler"', strings)


if __name__ == "__main__":
    unittest.main()
