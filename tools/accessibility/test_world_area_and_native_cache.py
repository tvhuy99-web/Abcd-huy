"""Contracts for spoken area transitions, step-gate safety and verified APK native reuse."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
WORKFLOW = ROOT / ".github/workflows/android-accessibility.yml"


def read(path):
    return path.read_text(encoding="utf-8")


class WorldAreaAndNativeBuildTest(unittest.TestCase):
    def test_area_speech_uses_known_overmap_terrain_only(self):
        world = read(SRC / "a11y_world.cpp")
        header = read(SRC / "a11y_world.h")
        engine = read(SRC / "handle_action.cpp")
        self.assertIn("void announce_area_transition()", header)
        area = world.split("void announce_area_transition()", 1)[1].split(
            "void announce_player_coordinates()", 1)[0]
        self.assertIn("previous_epoch", area)
        self.assertIn("previous_tile", area)
        self.assertIn("previous_area", area)
        self.assertIn("get_avatar().pos_abs_omt()", area)
        self.assertIn("overmap_buffer.seen( omt )", area)
        self.assertIn("om_vision_level::unseen", area)
        self.assertIn("overmap_buffer.ter_existing( omt )", area)
        self.assertIn("terrain->get_name( vision )", area)
        self.assertIn("area != previous_area", area)
        self.assertIn('_( "Entering area: %s." )', area)
        self.assertIn('diagnostic_event( "WORLD_AREA_CHANGE"', area)
        self.assertIn("a11y::announce_area_transition();", engine)

    def test_reader_only_visibility_refresh_is_not_duplicated(self):
        engine = read(SRC / "handle_action.cpp")
        start = engine.split("input_context game::get_player_input(", 1)[1]
        self.assertIn('if( !a11y::android::suppress_world_visuals() ) {', start)
        self.assertIn("here.update_visibility_cache( pos.z() );", start)
        self.assertIn("vision_map.build_map_cache( z );", start)
        self.assertIn("vision_map.update_visibility_cache( z );", start)

    def test_bounded_retry_checks_native_gate_after_new_world_revision(self):
        engine = read(JAVA / "GameReaderController.java")
        bridge = read(SRC / "a11y_android.cpp")
        self.assertIn("nativeWorldStepReady()", engine)
        self.assertIn("snapshot.revision <= pendingWorldRevision", engine)
        self.assertIn("pendingWorldContext.equals(snapshot.contextKey())", engine)
        self.assertIn("schedulePendingWorldRetry()", engine)
        self.assertIn("handler.postDelayed(pendingWorldRetryPoll, 30)", engine)
        self.assertIn("elapsed > 1200", engine)
        self.assertIn("handler.removeCallbacks(pendingWorldRetryPoll)", engine)
        self.assertIn("world_step_can_accept()", bridge)
        self.assertIn("world_step_ready && has_world_token", bridge)
        self.assertIn("WORLD_STEP_SUPERSEDED", engine)
        self.assertIn("WORLD_STEP_RETRIED", engine)
        self.assertIn("WORLD_STEP_EXPIRED", engine)
        self.assertIn("static native boolean nativeWorldStepReady()",
                      read(JAVA / "CataclysmDDA.java"))

    def test_cache_and_ai_slow_path_instrumented_only_when_log_on(self):
        mapping = read(SRC / "map.cpp")
        turns = read(SRC / "do_turn.cpp")
        self.assertIn('diagnostic_event( "WORLD_CACHE_PHASE"', mapping)
        for phase in ("scene_ms=", "transparency_ms=", "avatar_fov_ms=",
                      "camera_ms=", "light_sources_ms=", "lightmap_ms="):
            self.assertIn(phase, mapping)
        self.assertIn("profile_cache = a11y::android::performance_profiling_enabled()", mapping)
        self.assertIn('diagnostic_event( "WORLD_AI_HOTSPOT"', turns)
        self.assertIn('diagnostic_event( "WORLD_AI_COUNTS"', turns)
        self.assertIn("monster_ms >= 40", turns)
        self.assertIn("npc_ms >= 40", turns)

    def test_shift_keeps_submap_load_and_reconciliation(self):
        mapping = read(SRC / "map.cpp")
        self.assertIn("loaded_grids.reserve(", mapping)
        self.assertIn("loaded_grids.emplace_back(", mapping)
        self.assertIn("reconcile_loaded_items( reconcile_scope::additive )", mapping)
        self.assertIn('diagnostic_event( "WORLD_SUBMAP_SHIFT_PHASE"', mapping)
        for phase in ("unload_ms=", "grid_cache_ms=", "new_submaps_ms=",
                      "vehicles_ms=", "actualize_ms=", "reconcile_ms="):
            self.assertIn(phase, mapping)

    def test_fast_native_reuse_is_exact_fingerprint_only(self):
        yaml = read(WORKFLOW)
        self.assertIn("- name: Fingerprint native inputs", yaml)
        self.assertIn("hashFiles('src/**', 'android/app/jni/**'", yaml)
        self.assertIn("a11y-native-libs-v2-ndk28-arm64-", yaml)
        self.assertNotIn("a11y-native-libmain-v1-ndk28-arm64-", yaml)
        self.assertIn("path: android/app/src/main/jniLibs/arm64-v8a/", yaml)
        self.assertNotIn("restore-keys:", yaml.split("id: cached-native")[1].split(
            "- name: Validate native libraries", 1)[0])
        self.assertIn("steps.cached-native.outputs.cache-hit == 'true'", yaml)
        self.assertIn("-PEXCLUDE_NATIVE_LIBS=true", yaml)
        self.assertIn("-Poverride_version=$NATIVE_VERSION", yaml)
        self.assertIn("-Paccessibility_apk_version=$apk_version", yaml)
        self.assertIn("apk_version=\"$(git rev-parse --short=8 HEAD)\"", yaml)
        self.assertIn("accessibility_apk_version", read(ROOT / "android/app/build.gradle"))
        self.assertIn("-Pj=3", yaml)
        self.assertIn("CCACHE_MAXSIZE: 3G", yaml)
        self.assertIn("ccache --show-stats --verbose", yaml)
        self.assertIn("native = archive.read('lib/arm64-v8a/' + name)", yaml)
        self.assertIn("(directory / name).write_bytes(native)", yaml)
        self.assertIn("required_libs = (", yaml)
        for library in ("libmain.so", "libSDL3.so", "libSDL3_image.so",
                        "libSDL3_ttf.so", "libSDL3_mixer.so", "libc++_shared.so"):
            self.assertIn(f"\'{library}\'", yaml)
        self.assertIn("missing runtime library", yaml)
        self.assertIn("Signer #1 certificate SHA-256 digest:", yaml)
        self.assertIn("Incorrect monotonically increasing versionCode", yaml)
        self.assertIn("native_mode=prebuilt_verified", yaml)
        self.assertIn("native_mode=compile_and_seed_cache", yaml)


if __name__ == "__main__":
    unittest.main()
