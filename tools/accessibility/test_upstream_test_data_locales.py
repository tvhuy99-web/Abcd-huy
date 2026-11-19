"""Guard both TEST_DATA translation fixtures during upstream integration."""

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class TestUpstreamTestDataLocales(unittest.TestCase):
    def test_makefile_builds_both_locales_from_matching_catalogs(self):
        makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
        self.assertIn(
            "data/mods/TEST_DATA/lang/mo/%/LC_MESSAGES/TEST_DATA.mo: "
            "data/mods/TEST_DATA/lang/po/%.po",
            makefile,
        )
        for locale in ("en", "ru"):
            with self.subTest(locale=locale):
                self.assertIn(
                    f"data/mods/TEST_DATA/lang/mo/{locale}/LC_MESSAGES/TEST_DATA.mo",
                    makefile,
                )
                source = ROOT / f"data/mods/TEST_DATA/lang/po/{locale}.po"
                self.assertTrue(source.is_file(), f"Missing {locale} source catalog")
                self.assertIn(f"Language: {locale}", source.read_text(encoding="utf-8"))

    def test_fork_translation_dependencies_remain(self):
        makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
        self.assertIn("$(wildcard lang/po/accessibility/*.po)", makefile)
        self.assertIn("$(wildcard lang/po/*.po lang/po/*.po.gz)", makefile)


if __name__ == "__main__":
    unittest.main()
