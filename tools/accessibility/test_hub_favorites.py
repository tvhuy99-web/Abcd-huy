"""Behavioral structure checks for favorites and two-choice long-press actions."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
RES = ROOT / "android/app/src/main/res"


class HubFavoritesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hub = (JAVA / "GameActionHub.java").read_text("utf-8")
        cls.controller = (JAVA / "GameReaderController.java").read_text("utf-8")
        cls.favorites = (JAVA / "ReaderFavorites.java").read_text("utf-8")

    def test_root_favorites_first_search_last_and_group_ids_stable(self):
        root = self.hub.split("if (section == ROOT) {", 1)[1].split(
            "if (section == FAVORITES)", 1)[0]
        self.assertLess(root.index("a11yHubFavorites"),
                        root.index("for (int i = 1;"))
        self.assertLess(root.index("for (int i = 1;"),
                        root.index("TITLES[SEARCH]"))
        self.assertIn("static final int FAVORITES = 17;", self.hub)
        self.assertIn("static final int DIRECTIONS = 16;", self.hub)
        self.assertIn("hubRootIndex = GameActionHub.TITLES.length;", self.controller)
        self.assertIn("hubRootIndex = 0;", self.controller)
        self.assertIn('"hub:favorites"', self.hub)

    def test_long_press_has_two_choices_and_keeps_gesture_binding(self):
        self.assertIn("option.setOnLongClickListener(", self.controller)
        self.assertIn("showHubActionChoices();", self.controller)
        self.assertIn("private void applyHubActionChoice()", self.controller)
        self.assertIn("if (choice == 0)", self.controller)
        self.assertIn("showGestureChoices();", self.controller)
        self.assertIn("favorites.add(target, label)", self.controller)
        self.assertIn("favorites.remove(target)", self.controller)
        self.assertIn("int count = choosingHubAction ? 2", self.controller)
        self.assertIn("gestureBindings.assign(", self.controller)

    def test_favorites_persist_by_identity_not_menu_index(self):
        self.assertIn('getSharedPreferences("internal_reader"', self.favorites)
        self.assertIn('"hub_favorites_v1"', self.favorites)
        self.assertIn('entry.put("target", favorite.target)', self.favorites)
        self.assertIn('entry.put("label", favorite.label)', self.favorites)
        self.assertIn('store.edit().putString(KEY, array.toString()).apply()', self.favorites)
        self.assertIn('!"hub:favorites".equals(target)', self.favorites)
        self.assertIn('snapshot.find(action)', self.hub)
        self.assertIn('a11yHubFavoriteUnavailable', self.hub)

    def test_vietnamese_strings_and_limits(self):
        for dir_name in ("values", "values-vi"):
            xml = (RES / dir_name / "strings.xml").read_text("utf-8")
            for name in ("a11yHubFavorites", "a11yHubGestureOption",
                         "a11yHubAddFavorite", "a11yHubRemoveFavorite"):
                self.assertIn('name="' + name + '"', xml)
        vi = (RES / "values-vi/strings.xml").read_text("utf-8")
        self.assertRegex(vi, r'name="a11yHubFavorites">Yêu thích')
        self.assertIn("private static final int MAX_FAVORITES = 64;", self.favorites)


if __name__ == "__main__":
    unittest.main()
