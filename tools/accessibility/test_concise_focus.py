"""Source-level regressions for concise Android reader focus announcements."""

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android" / "app" / "src" / "main" / "java"
JAVA = JAVA / "com" / "cleverraven" / "cataclysmdda"
READER = JAVA / "GameReaderController.java"
EDITOR = JAVA / "ReaderTextEditor.java"
SPEECH = JAVA / "ReaderSpeechQueue.java"


class ConciseFocusTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.reader = READER.read_text(encoding="utf-8")
        cls.editor = EDITOR.read_text(encoding="utf-8")
        cls.speech = SPEECH.read_text(encoding="utf-8")

    def test_every_automatic_focus_uses_compact_node_text(self):
        body = self.reader.split("public void readCurrent() {", 1)[1].split(
            "public void detail() {", 1
        )[0]
        self.assertIn("String spoken = nodeText(node, false);", body)
        self.assertIn("speech.focus(node.id, spoken);", body)
        self.assertIn("announceAccessibility(spoken);", body)
        self.assertNotIn("snapshot.title", body)
        self.assertNotIn("snapshot.summary", body)

    def test_selected_only_on_details_but_position_on_every_focus(self):
        body = self.reader.split("private String nodeText(", 1)[1].split(
            "private static String formatRawNumber(", 1
        )[0]
        self.assertIn("if (includeDetail && node.selected)", body)
        self.assertRegex(body, r"activity\.getString\(\s*R\.string\.a11yPosition")
        self.assertIn("if (spokenPosition > 0 && spokenTotal > 0)", body)
        self.assertNotIn("if (includeDetail && node.position", body)
        self.assertIn("activity.getString(R.string.a11yChecked)", body)
        self.assertIn("node.disabledReason", body)

    def test_screen_title_on_entry_and_full_details_remain_available(self):
        self.assertIn('feedback(introduction.isEmpty()', self.reader)
        self.assertIn('introduction + ". " + nodeText(next.find(focus), false)',
                      self.reader)
        self.assertIn('String introduction = yesNo ? next.summary.trim() : next.title;',
                      self.reader)
        self.assertIn("String information = nodeText(node, true);", self.reader)
        self.assertIn("feedback(information);", self.reader)

    def test_editor_and_hub_restore_position_counters(self):
        body = self.editor.split("public void readCurrent() {", 1)[1].split(
            "private BreakIterator boundaries(", 1
        )[0]
        self.assertIn("R.string.a11yPosition, selected + 1, items.size()", body)
        self.assertIn("hubSpoken(List<GameActionHub.Entry> entries, int selected)", self.reader)
        self.assertIn("R.string.a11yPosition, selected + 1, entries.size()", self.reader)

    def test_fast_swipes_have_no_time_based_speech_suppression(self):
        self.assertNotIn("DUPLICATE_WINDOW_MS", self.speech)
        self.assertNotIn("focusDebounce", self.speech)
        self.assertNotIn("recentStarts", self.speech)
        self.assertIn("say(key, text, FOCUS);", self.speech)
        self.assertIn("if (priority != FOCUS) {", self.speech)
        self.assertIn("(priority == FOCUS && active.priority == FOCUS)", self.speech)


if __name__ == "__main__":
    unittest.main()
