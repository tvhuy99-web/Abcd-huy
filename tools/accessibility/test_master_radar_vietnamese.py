"""All spoken radar notifications must have Vietnamese catalog entries."""

from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


class MasterVietnameseRadar(unittest.TestCase):
    def test_every_native_radar_phrase_and_button_has_vi_translation(self):
        cpp = (ROOT / "src/a11y_radar.cpp").read_text(encoding="utf-8")
        catalog = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")
        phrases = set(re.findall(r'_\\(\\s*"([^"]+)"\\s*\\)', cpp))
        phrases.update(("Automatic spoken radar",
                        "Toggle spoken announcements when objects enter eight tiles."))
        for source in phrases:
            with self.subTest(phrase=source):
                self.assertIn('msgid "' + source + '"', catalog)
        self.assertIn('msgstr "Rađa tự động"', catalog)
        self.assertIn('msgstr "Thông báo quái vật"',
                      (ROOT / "android/app/src/main/res/values-vi/strings.xml").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
