"""Ensure live command status is spoken consistently throughout the action hub."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class GlobalCommandStateSpeech(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.builder = (SRC / "a11y_input.cpp").read_text(encoding="utf-8")
        cls.world = (SRC / "a11y_world.cpp").read_text(encoding="utf-8")
        cls.hub = (JAVA / "GameActionHub.java").read_text(encoding="utf-8")
        cls.controller = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        cls.vi = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")

    def test_action_state_is_not_trapped_in_world_menu(self):
        self.assertIn('void annotate_live_command_states( semantic_snapshot &snapshot )',
                      self.builder)
        self.assertIn('annotate_live_command_states( cached );', self.builder)
        self.assertIn('annotate_live_command_states( snapshot );', self.builder)
        self.assertLess(self.builder.index('cached_world_commands = snapshot;'),
                        self.builder.index('annotate_live_command_states( snapshot );'))
        self.assertNotIn('node.label = auto_on ?', self.world)
        self.assertNotIn('node.id == "toggle_auto_features"', self.world)

    def test_combat_and_automation_share_live_setting_state(self):
        for command in ("autoattack", "safemode", "autosafe", "toggle_auto_pickup",
                        "toggle_auto_features", "toggle_auto_mining",
                        "toggle_auto_pulp_butcher", "toggle_auto_foraging",
                        "toggle_thief_mode", "auto_travel_mode", "haul"):
            self.assertIn('id == "' + command + '"', self.builder)
        self.assertIn('node.value = enabled ? _( "Currently on" ) : _( "Currently off" );',
                      self.builder)
        self.assertIn('node.value = option.getValue() == "off" ?', self.builder)
        self.assertIn('node.value = _( "One attack, not an on/off setting" );', self.builder)
        self.assertIn('get_avatar().is_hauling()', self.builder)
        self.assertIn('node.value = g->auto_travel_mode ?', self.builder)

    def test_hub_search_favorites_all_use_semantic_status(self):
        self.assertIn('private static String spokenNodeLabel(', self.hub)
        self.assertIn('label += ". " + node.value;', self.hub)
        self.assertIn('!"document".equals(node.role)', self.hub)
        self.assertIn('!"status".equals(node.role)', self.hub)
        self.assertIn('new Entry(spokenNodeLabel(context, node), id, EXECUTE, 0)',
                      self.hub)
        self.assertIn('new Entry(spokenNodeLabel(context, node), node.id, EXECUTE, 0)',
                      self.hub)
        self.assertIn('favorite.label : spokenNodeLabel(context, node)', self.hub)
        self.assertIn('addCommandMatches(result, context, snapshot, query);', self.hub)
        self.assertIn('text.append(". ").append(node.value);', self.controller)

    def test_vietnamese_descriptions_are_present(self):
        self.assertIn('msgstr "Tấn công một lần, không phải chế độ bật tắt"', self.vi)
        self.assertIn('msgstr "Chế độ di chuyển: %s"', self.vi)
        self.assertIn('msgstr "Đang bật"', self.vi)
        self.assertIn('msgstr "Đang tắt"', self.vi)


if __name__ == "__main__":
    unittest.main()
