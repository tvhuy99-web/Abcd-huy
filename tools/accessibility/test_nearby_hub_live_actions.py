"""Section 2 shows available nearby actions, not all registered keybindings."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
SRC = ROOT / "src"


def read(path):
    return path.read_text(encoding="utf-8")


class NearbyHubLiveActionsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hub = read(JAVA / "GameActionHub.java")
        cls.reader = read(JAVA / "GameReaderController.java")
        cls.world = read(SRC / "a11y_world.cpp")
        cls.action = read(SRC / "action.cpp")
        cls.handle = read(SRC / "handle_action.cpp")
        cls.game = read(SRC / "game.cpp")
        cls.header = read(SRC / "action.h")
        cls.vi = read(ROOT / "lang/po/accessibility/vi.po")
        cls.ui_vi = read(ROOT / "android/app/src/main/res/values-vi/strings.xml")
        cls.ui_en = read(ROOT / "android/app/src/main/res/values/strings.xml")

    def test_only_second_group_filters_availability(self):
        group = self.hub.split("if (section == 1) {", 1)[1].split(
            "Set<String> seen = new HashSet<>();", 1
        )[0]
        self.assertIn('"reader:nearby:available:" + id', group)
        self.assertIn("!action.enabled", group)
        self.assertIn("available == null || !available.enabled", group)
        self.assertIn("return result;", group)
        self.assertIn("for (String id : ACTIONS[section]) { addNode(", self.hub)
        for action in ('"grab"', '"butcher"', '"smash"',
                       '"haul"', '"haul_toggle"', '"loot"'):
            self.assertIn(action, self.hub)

    def test_native_only_publishes_current_visible_usable_actions(self):
        section = self.world.split(
            "Only the second comprehensive-menu group", 1
        )[1].split("return result;", 1)[0]
        self.assertIn("seen.source != observation_source::visible", section)
        self.assertIn("can_interact_at( ACTION_EXAMINE", section)
        self.assertIn("can_interact_at( ACTION_OPEN", section)
        self.assertIn("can_interact_at( ACTION_CLOSE", section)
        self.assertIn("can_interact_at( ACTION_PICKUP", section)
        self.assertIn("can_interact_at( ACTION_SMASH", section)
        self.assertIn("nearby_map.furn( target ).obj().is_movable()", section)
        self.assertIn("viewer.is_mounted()", section)
        self.assertIn("viewer.has_active_mutation(", section)
        self.assertIn("viewer.has_effect( effect_incorporeal_nearby )", section)
        self.assertIn('"reader:nearby:available:" + id', section)
        self.assertIn('if (node.id.startsWith("reader:nearby:available:")) { continue; }',
                      self.reader)

    def test_contextual_actions_cover_native_interaction_group(self):
        group = self.hub.split("if (section == 1) {", 1)[0].split(
            'private static final String[][] ACTIONS', 1)[1]
        for action in ('"examine_and_pickup"', '"reader:pickup_here"',
                       '"haul"', '"haul_toggle"', '"loot"', '"control_vehicle"'):
            self.assertIn(action, group)
            self.assertIn(action, self.world)

    def test_native_checks_real_context_and_handbrake(self):
        for token in ('viewer.is_hauling()', '"haul_toggle"',
                      'viewer.in_vehicle', 'part_with_feature( "CONTROLS", true )',
                      'ordinary_mount && id == "close"',
                      'node.id == "smash" && viewer.controlling_vehicle',
                      '!viewer.controlling_vehicle &&',
                      'can_interact_at( ACTION_EXAMINE_AND_PICKUP, nearby_map, target )',
                      'target == player_pos', '"reader:pickup_here"'):
            self.assertIn(token, self.world)
        self.assertIn('get_actions_disabled_mounted()', self.handle)

    def test_salvage_only_and_sealed_butchery_are_detected(self):
        self.assertIn('TFLAG_SEALED, p', self.action)
        self.assertIn('valid_to_cut_up( nullptr, candidate )', self.action)
        self.assertIn('for( item *tool : player_character.items_with(', self.action)
        self.assertIn('salvage_actor', self.action)

    def test_short_accurate_no_target_feedback(self):
        self.assertIn('_( "Nothing to examine." )', self.game)
        self.assertIn('_( "No direct interaction." )', self.handle)
        self.assertIn("if( visible_object.empty() )", self.action)
        self.assertIn('_( "%1$s: %2$s" )', self.action)
        self.assertIn('msgstr "Không có gì để kiểm tra."', self.vi)
        self.assertIn('msgstr "Không có tương tác phù hợp."', self.vi)
        self.assertIn('a11yHubNearbyNoActions', self.reader)
        self.assertIn('Không có hành động phù hợp ở đây.', self.ui_vi)
        self.assertIn('No actions available here.', self.ui_en)


if __name__ == "__main__":
    unittest.main()
