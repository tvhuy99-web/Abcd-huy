"""Guards for concise nearby actions, spoken NPC replies and quick pickup."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class ConciseInteractionDialogueQuickPickupTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.action = read("src/action.cpp")
        cls.handle = read("src/handle_action.cpp")
        cls.world = read("src/a11y_world.cpp")
        cls.dialogue = read("src/npctalk.cpp")
        cls.hub = read(
            "android/app/src/main/java/com/cleverraven/"
            "cataclysmdda/GameActionHub.java"
        )
        cls.reader = read(
            "android/app/src/main/java/com/cleverraven/"
            "cataclysmdda/GameReaderController.java"
        )
        cls.translations = read("lang/po/accessibility/vi.po")
        cls.pickup = cls.handle.split(
            "static void pickup_here_accessible()", 1
        )[1].split("input_context game::get_player_input", 1)[0]

    def test_nearby_targets_have_no_long_instruction(self):
        start = self.action.split(
            "choose_accessible_nearby_target(", 1
        )[1].split("action_id handle_interact(", 1)[0]
        self.assertIn('menu.input_category = "nearby-target";', start)
        self.assertIn("menu.text.clear();", start)
        self.assertIn("menu.addentry_desc(", start)
        self.assertIn("a11y::describe_tile( observed )", start)
        self.assertNotIn(
            'menu.text = _( "Nearby objects.', start
        )
        self.assertIn('"nearby-target".equals(next.category)', self.reader)

    def test_npc_reply_is_spoken_once_per_new_dialogue_topic(self):
        self.assertIn("const std::string spoken_reply", self.dialogue)
        self.assertIn("bool reply_announced = false;", self.dialogue)
        self.assertIn(
            "a11y::android::announce( remove_color_tags( spoken_reply ), 1 );",
            self.dialogue,
        )
        self.assertIn(
            "if( !reply_announced && a11y::enabled() )", self.dialogue
        )
        self.assertIn('snapshot.summary = remove_color_tags( challenge );',
                      self.dialogue)
        self.assertIn('"dialogue:history"', self.dialogue)

    def test_quick_pickup_only_current_tile_and_auto_single(self):
        code = self.pickup
        self.assertIn("const tripoint_bub_ms pos = you.pos_bub();", code)
        self.assertIn("map_stack ground = here.i_at( pos );", code)
        self.assertIn("choices.size() == 1", code)
        self.assertIn("choices.front().available == 1", code)
        self.assertIn("you.pick_up( only );", code)
        self.assertNotIn("choose_adjacent", code)

    def test_many_or_charged_items_get_quantity_form_and_one_confirm(self):
        code = self.pickup
        self.assertIn("found->available += quantity;", code)
        self.assertIn('node.field_type = "int";', code)
        self.assertIn('node.role = "checkbox";', code)
        self.assertIn('intent.action == "set_value"', code)
        self.assertIn('a11y::selected_quantity( intent.value', code)
        self.assertIn('choice.selected = static_cast<int>( *amount );', code)
        self.assertNotIn('intent.action == "apply"', code)
        self.assertIn('confirm.id = "pickup-here:confirm";', code)
        self.assertIn("you.pick_up( selection );", code)
        self.assertIn("remaining = choice.selected;", code)
        self.assertIn('if( intent.action == "cancel" )', code)

    def test_hub_registration_and_vietnamese_copy(self):
        self.assertIn('"reader:pickup_here"', self.hub)
        self.assertIn('"reader:pickup_here", _( "Pick up here" )',
                      self.world)
        self.assertIn('intent.node == "reader:pickup_here"', self.handle)
        self.assertIn('msgstr "Nhặt đồ tại chỗ"', self.translations)
        self.assertIn('msgstr "Xác nhận nhặt"', self.translations)
        self.assertIn('"pickup-here".equals(next.category)', self.reader)


if __name__ == "__main__":
    unittest.main()
