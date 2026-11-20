"""Guard the entire read-only scene survey command through Android TTS."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class MasterSceneSurveyFlow(unittest.TestCase):
    def test_native_scene_is_read_only_and_accessible_from_world_menu(self):
        cpp = (SRC / "a11y_scene.cpp").read_text(encoding="utf-8")
        handler = (SRC / "handle_action.cpp").read_text(encoding="utf-8")
        world = (SRC / "a11y_world.cpp").read_text(encoding="utf-8")
        self.assertIn('announce( "A11Y_SCENE:" + build_survey(), 1 )', cpp)
        self.assertIn('intent.node == "reader:scene:survey"', handler)
        self.assertIn("a11y::announce_detailed_place_survey();", handler)
        self.assertIn('button( "reader:scene:survey"', world)
        self.assertIn("overmap_buffer.ter_existing(", cpp)

    def test_android_delivers_full_document_without_expanding_normal_message_limit(self):
        bridge = (SRC / "a11y_android.cpp").read_text(encoding="utf-8")
        reader = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        hub = (JAVA / "GameActionHub.java").read_text(encoding="utf-8")
        self.assertIn("scene_survey ? 131072 : 16384", bridge)
        self.assertIn('SCENE_PREFIX = "A11Y_SCENE:"', reader)
        self.assertIn("&& !text.startsWith(SCENE_PREFIX)", reader)
        self.assertIn("placeSurvey ? message.substring(SCENE_PREFIX.length())", reader)
        self.assertIn("} else if (placeSurvey) {", reader)
        self.assertIn("speakInlineDetail(spokenMessage);", reader)
        self.assertIn('"reader:scene:survey", "listitems", "reader:places"', hub)

    def test_vietnamese_scene_headings_exist(self):
        po = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")
        for msg in ("Detailed place survey", "Place survey: %s.", "Entrances",
                    "Dangers and traces", "Survey limit reached."):
            self.assertIn('msgid "' + msg + '"', po)


if __name__ == "__main__":
    unittest.main()
