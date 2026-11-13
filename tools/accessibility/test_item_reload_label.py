"""Accessibility: item reload covers ammunition, flashlight batteries and fuel."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GAME = (ROOT / "src/game.cpp").read_text(encoding="utf-8")
VI = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")


class ItemReloadAccessibilityLabels(unittest.TestCase):
    def test_item_submenu_has_accurate_spoken_label_and_native_action(self):
        self.assertIn('_( "Reload item (ammo, battery or fuel)" )', GAME)
        self.assertIn('_( "Partly reload item (ammo, battery or fuel)" )', GAME)
        self.assertIn('pgettext( "action", "reload" ), u.rate_action_reload( oThisItem )', GAME)
        self.assertIn('pgettext( "action", "part reload" ), u.rate_action_reload( oThisItem )', GAME)
        self.assertIn("a11y::enabled() ?", GAME)
        self.assertIn("case 'r':\n                    reload( locThisItem );", GAME)
        self.assertIn("case 'p':\n                    reload( locThisItem, true );", GAME)

    def test_vietnamese_labels_explain_batteries_and_fuel(self):
        self.assertIn('msgid "Reload item (ammo, battery or fuel)"\n'
                      'msgstr "Nạp vật phẩm (đạn, pin hoặc nhiên liệu)"', VI)
        self.assertIn('msgid "Partly reload item (ammo, battery or fuel)"\n'
                      'msgstr "Nạp một phần (đạn, pin hoặc nhiên liệu)"', VI)


if __name__ == "__main__":
    unittest.main()
