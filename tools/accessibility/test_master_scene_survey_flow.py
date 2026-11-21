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

    def test_android_opens_focusable_survey_without_expanding_normal_message_limit(self):
        bridge = (SRC / "a11y_android.cpp").read_text(encoding="utf-8")
        reader = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        hub = (JAVA / "GameActionHub.java").read_text(encoding="utf-8")
        surface = (JAVA / "ReaderGestureSurface.java").read_text(encoding="utf-8")
        self.assertIn("scene_survey ? 131072 : 16384", bridge)
        self.assertIn('SCENE_PREFIX = "A11Y_SCENE:"', reader)
        self.assertIn("&& !text.startsWith(SCENE_PREFIX)", reader)
        self.assertIn("placeSurvey ? message.substring(SCENE_PREFIX.length())", reader)
        self.assertIn("openPlaceSurvey(message.substring(SCENE_PREFIX.length()))", reader)
        self.assertNotIn("} else if (placeSurvey) {", reader)
        self.assertIn("private void renderPlaceSurvey()", reader)
        self.assertIn("private void closePlaceSurvey()", reader)
        self.assertIn('report.split("\\r?\\n")', reader)
        self.assertIn("final int total = placeSurveyRows.size() + 1;", reader)
        self.assertIn("placeSurveyIndex + delta", reader)
        self.assertIn("if (placeSurveyOpen) { closePlaceSurvey(); return; }", reader)
        self.assertIn("if (placeSurveyOpen) { return presentationEpoch +", reader)
        self.assertIn("!comprehensiveMenu && !placeSurveyOpen", reader)
        self.assertIn("listener.goBack();", surface)
        self.assertIn('"reader:scene:survey", "listitems", "reader:places"', hub)

    def test_survey_native_output_has_individual_fact_boundaries(self):
        scene = (SRC / "a11y_scene.cpp").read_text(encoding="utf-8")
        self.assertIn('report += "\\n" + title + ":";', scene)
        self.assertIn('report += "\\n" + entry.label + ", " +', scene)
        self.assertIn('report += "\\n" + ordered[i].first + ", " +', scene)
        self.assertIn('report += "\\n" + straight_route( here, center, dir );', scene)
        self.assertIn('report += "\\n" + std::string( _( "Surroundings:" ) );', scene)

    def test_survey_screen_has_localized_continue_button(self):
        for folder, expected in (("values", "Continue"),
                                 ("values-vi", "Tiếp tục")):
            text = (ROOT / "android/app/src/main/res" / folder / "strings.xml").read_text(
                encoding="utf-8")
            self.assertIn('name="a11ySurveyContinue">' + expected + "</string>", text)
        reader = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        self.assertIn("R.string.a11ySurveyContinue", reader)
        self.assertIn("if (continueButton) { closePlaceSurvey(); }", reader)

    def test_vietnamese_scene_headings_exist(self):
        po = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")
        for msg in ("Detailed place survey", "Place survey: %s.", "Entrances",
                    "Dangers and traces", "Survey limit reached."):
            self.assertIn('msgid "' + msg + '"', po)


if __name__ == "__main__":
    unittest.main()
