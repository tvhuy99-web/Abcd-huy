"""Guard upstream release dependency updates without enabling automatic publishing."""

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class UpstreamReleaseAndIwyu(unittest.TestCase):
    def test_release_remains_manual_and_resource_bounded(self):
        content = (ROOT / ".github/workflows/release.yml").read_text(encoding="utf-8")
        self.assertIn("workflow_dispatch:", content)
        self.assertNotIn("\n  push:", content)
        self.assertIn("max-parallel: 4", content)
        self.assertEqual(content.count(
            "actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1"
        ), 3)
        self.assertIn("make -C lang -j$NUMBER_OF_PROCESSORS", content)
        self.assertNotIn("lang/compile_mo.sh all", content)

    def test_iwyu_skips_both_legacy_and_new_cache_units(self):
        ignored = (ROOT / "tools/iwyu/bad_files.txt").read_text(encoding="utf-8").splitlines()
        self.assertIn("src/map.cpp", ignored)
        self.assertIn("src/map_cache.cpp", ignored)
        self.assertEqual(ignored.count("src/map_cache.cpp"), 1)


if __name__ == "__main__":
    unittest.main()
