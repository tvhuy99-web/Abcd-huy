"""Android tutorial text coverage: every lesson uses touchscreen controls."""
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
TRAPS = ROOT / "data/mods/dda_tutorial/traps.json"
SPOKEN = ROOT / "data/mods/dda_tutorial/snippets_android_reader.json"
DESKTOP = ROOT / "data/mods/dda_tutorial/snippets.json"
TUTORIAL = ROOT / "src/gamemode_tutorial.cpp"
EFFECTS = ROOT / "src/npctalk.cpp"


class AndroidTutorialLessonTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.traps = json.loads(TRAPS.read_text("utf-8"))
        cls.mobile = json.loads(SPOKEN.read_text("utf-8"))[0]["text"]
        cls.desktop = json.loads(DESKTOP.read_text("utf-8"))[0]["text"]
        cls.tutorial = TUTORIAL.read_text("utf-8")
        cls.effects = EFFECTS.read_text("utf-8")
        cls.messages = {entry["id"]: entry["text"] for entry in cls.mobile}

    def test_every_engine_lesson_has_one_android_reader_snippet(self):
        lessons = re.findall(
            r'case tut_lesson::(LESSON_\w+): return "LESSON_\w+"',
            self.tutorial)
        self.assertEqual(len(lessons), 49)
        self.assertEqual(len(self.mobile), 49)
        self.assertEqual(len(self.messages), len(self.mobile))
        for name in lessons:
            with self.subTest(name=name):
                self.assertIn("ANDROID_READER_" + name, self.messages)

    def test_all_eoc_tutorial_cells_use_android_alternatives(self):
        eocs = [entry for entry in self.traps
                if entry.get("type") == "effect_on_condition"
                and entry["id"].startswith("EOC_LESSON_")]
        self.assertEqual(len(eocs), 14)
        for entry in eocs:
            name = entry["id"].removeprefix("EOC_")
            message = next(effect for effect in entry["effect"]
                           if "u_message" in effect)
            with self.subTest(name=name):
                self.assertEqual(message["android_reader_lesson"],
                                 "ANDROID_READER_" + name)
                self.assertIn(message["android_reader_lesson"], self.messages)
                self.assertIn("u_message", message)
                self.assertTrue(message["popup"])

    def test_android_override_leaves_desktop_text_and_other_eocs(self):
        self.assertIn("Mouse view", str(self.traps))
        self.assertIn("<keybind:", str(self.traps))
        self.assertIn("LESSON_INTRO", {x["id"] for x in self.desktop})
        self.assertIn('jo.get_string( "android_reader_lesson", "" )',
                      self.effects)
        self.assertIn("#if defined(__ANDROID__)", self.effects)
        self.assertIn("SNIPPET.get_snippet_by_id( snippet_id( android_reader_lesson ) )",
                      self.effects)
        self.assertIn("if( android_reader_popup )", self.effects)

    def test_mobile_instructions_do_not_use_pc_keys_or_mouse(self):
        forbidden = ("<keybind:", "dùng chuột", "bấm phím",
                     "nhấn phím", "numpad", "keyboard")
        for name, value in self.messages.items():
            with self.subTest(name=name):
                self.assertGreater(len(value), 45)
                self.assertLessEqual(len(value), 680)
                for phrase in forbidden:
                    self.assertNotIn(phrase, value.lower())

    def test_new_trap_lessons_start_with_goal_and_end_with_task(self):
        for entry in self.traps:
            if entry.get("type") != "effect_on_condition":
                continue
            for effect in entry["effect"]:
                if "android_reader_lesson" not in effect:
                    continue
                text = self.messages[effect["android_reader_lesson"]]
                with self.subTest(lesson=effect["android_reader_lesson"]):
                    self.assertIn("Nhiệm vụ", text)

    def test_quick_popup_keeps_complete_lesson_for_reading_on_demand(self):
        helper = (ROOT / "src/a11y_tutorial_text.h").read_text("utf-8")
        world = (ROOT / "src/a11y_world.cpp").read_text("utf-8")
        game = (ROOT / "src/gamemode_tutorial.cpp").read_text("utf-8")
        hub = (ROOT / "android/app/src/main/java/com/cleverraven"
               "/cataclysmdda/GameActionHub.java").read_text("utf-8")
        self.assertIn('"Nhiệm vụ:"', helper)
        self.assertIn('a11y::tutorial::brief( translated_message )',
                      self.effects)
        self.assertIn('last_lesson_key, translated_message', self.effects)
        self.assertIn('last_lesson_key, translated_lesson', game)
        self.assertIn("lesson == tut_lesson::LESSON_INTRO", game)
        self.assertIn('"reader:tutorial:last"', world)
        self.assertIn('"reader:tutorial:last"', hub)
        self.assertIn('msgid "Read latest tutorial lesson in full"',
                      (ROOT / "lang/po/accessibility/vi.po").read_text("utf-8"))


if __name__ == "__main__":
    unittest.main()
