"""Regression contracts for level-by-level accessible game navigation."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
NATIVE = ROOT / "src/main_menu.cpp"


class NestedMenuNavigationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.native = NATIVE.read_text(encoding="utf-8")
        cls.controller = (JAVA / "GameReaderController.java").read_text(
            encoding="utf-8"
        )

    def test_main_menu_shows_either_root_or_active_submenu(self):
        src = self.native
        self.assertIn("bool reader_submenu_open = false;", src)
        self.assertIn("if( !reader_submenu_open ) {", src)
        self.assertIn("if( reader_submenu_open && sel1 ==", src)
        self.assertIn('snapshot.focus = reader_submenu_open ?', src)
        self.assertIn('snapshot.title = reader_submenu_open ?', src)
        self.assertIn("reader_submenu_open = true;", src)
        self.assertIn('return "TIMEOUT";', src)
        self.assertIn("reader_submenu_open = false;", src)

    def test_submenus_only_accept_own_items_and_back_one_level(self):
        src = self.native
        self.assertIn('if( !reader_submenu_open ) {\n'
                      '            return std::nullopt;', src)
        self.assertIn('if( intent.action == "cancel" ) {', src)
        self.assertIn('reader_submenu_open = false;\n'
                      '                return "TIMEOUT";', src)
        self.assertIn("sel2 = 0;", src)
        self.assertIn("vNewGameSubItems.size()", src)

    def test_all_semantic_tree_screens_share_one_level_navigation(self):
        code = self.controller
        self.assertIn("readerLayerParent.equals(node.parentId)", code)
        self.assertIn("enterReaderLayer(ReaderSnapshot.Node parent)", code)
        self.assertIn("pendingReaderLayer = node.id;", code)
        self.assertIn('send("expand", "");', code)
        self.assertIn("private boolean backReaderLayer()", code)
        self.assertIn("firstDirectChild(snapshot, parent.id)", code)
        self.assertIn("readerLayerParent = parent.parentId;", code)

    def test_talkback_and_spoken_navigation_share_same_focus(self):
        code = self.controller
        self.assertIn("List<ReaderSnapshot.Node> filtered = visibleNodes();",
                      code)
        self.assertIn("AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS",
                      code)
        self.assertIn("feedback(next.title + \". \" + nodeText(", code)
        self.assertIn("boolean nativeLayerChanged", code)
        self.assertIn("focus = next.find(next.focus)", code)

    def test_return_from_modal_restores_previous_item(self):
        code = self.controller
        self.assertIn("rememberScreenFocus(previousSnapshot, previous)", code)
        self.assertIn("savedScreenFocus.get(nativeScreenKey(next))", code)
        self.assertIn("savedScreenFocus.size() > 64", code)
        self.assertIn("snapshot.category", code)
        self.assertIn("backReaderLayer()", code)


if __name__ == "__main__":
    unittest.main()
