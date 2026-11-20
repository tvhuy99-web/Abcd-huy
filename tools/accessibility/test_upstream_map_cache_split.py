"""Guard upstream map-cache translation-unit split and Android profiling."""

from pathlib import Path
import unittest

SRC = Path(__file__).resolve().parents[2] / "src"
METHODS = (
    "map::alloc_cache(", "map::invalidate_map_cache(",
    "map::update_visibility_cache(", "map::build_scene_caches(",
    "map::ensure_scene_caches(", "map::build_map_cache(",
    "map::access_cache(",
)


class UpstreamMapCacheSplitTest(unittest.TestCase):
    def test_cache_methods_are_in_one_translation_unit(self):
        old = (SRC / "map.cpp").read_text(encoding="utf-8")
        cache = (SRC / "map_cache.cpp").read_text(encoding="utf-8")
        for method in METHODS:
            with self.subTest(method=method):
                self.assertNotIn(method, old)
                self.assertIn(method, cache)
        self.assertEqual(cache.count("map::access_cache("), 2)
        self.assertIn("map::free_cache_pool;", cache)
        self.assertNotIn("map::free_cache_pool;", old)

    def test_android_metrics_and_shift_trace_are_preserved(self):
        old = (SRC / "map.cpp").read_text(encoding="utf-8")
        cache = (SRC / "map_cache.cpp").read_text(encoding="utf-8")
        self.assertIn('diagnostic_event( "WORLD_CACHE_PHASE"', cache)
        self.assertIn("a11y::android::performance_profiling_enabled()", cache)
        for label in ("scene_ms=", "transparency_ms=", "avatar_fov_ms=",
                      "camera_ms=", "light_sources_ms=", "lightmap_ms="):
            self.assertIn(label, cache)
        self.assertIn('diagnostic_event( "WORLD_SUBMAP_SHIFT_PHASE"', old)


if __name__ == "__main__":
    unittest.main()
