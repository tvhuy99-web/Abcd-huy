"""Source-level regressions for concise Android reader focus announcements."""

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android" / "app" / "src" / "main" / "java"
JAVA = JAVA / "com" / "cleverraven" / "cataclysmdda"
READER = JAVA / "GameReaderController.java"
EDITOR = JAVA / "ReaderTextEditor.java"


class ConciseFocusTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.reader = READER.read_text(encoding="utf-8")
        cls.editor = EDITOR.read_text(encoding="utf-8")

    def test_every_automatic_focus_uses_compact_node_text(self):
        body = self.reader.split("public void readCurrent() {", 1)[1].split(
            "public void detail() {", 1
        )[0]
        self.assertIn("String spoken = nodeText(node, false);", body)
        self.assertIn("speech.focus(node.id, spoken);", body)
        self.assertIn("root.announceForAccessibility(spoken);", body)
        self.assertNotIn("snapshot.title", body)
        self.assertNotIn("snapshot.summary", body)

    def test_selected_is_only_spoken_on_explicit_details(self):
        body = self.reader.split("private String nodeText(", 1)[1].split(
            "private static String formatRawNumber(", 1
        )[0]
        self.assertIn("if (includeDetail && node.selected)", body)
        self.assertRegex(body, r"activity\.getString\(\s*R\.string\.a11yPosition")
        self.assertIn("activity.getString(R.string.a11yChecked)", body)
        self.assertIn("node.disabledReason", body)

    def test_screen_title_on_entry_and_full_details_remain_available(self):
        self.assertIn(
            'feedback(next.title + ". " + nodeText(next.find(focus), false));',
            self.reader,
        )
        self.assertIn("feedback(nodeText(node, true));", self.reader)

    def test_editor_speaks_choice_and_position_only(self):
        body = self.editor.split("public void readCurrent() {", 1)[1].split(
            "private BreakIterator boundaries(", 1
        )[0]
        self.assertIn('items.get(selected).label + ", "', body)
        self.assertIn("R.string.a11yPosition", body)


if __name__ == "__main__":
    unittest.main()
