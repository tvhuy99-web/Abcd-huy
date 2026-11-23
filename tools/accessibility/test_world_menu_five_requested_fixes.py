"""Regression contracts for the five requested world-menu reader fixes."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INPUT = (ROOT / "src/a11y_input.cpp").read_text(encoding="utf-8")
WORLD = (ROOT / "src/worldfactory.cpp").read_text(encoding="utf-8")
OPTIONS = (ROOT / "src/options.cpp").read_text(encoding="utf-8")
SPEECH = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")
ANDROID = (ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/GameReaderController.java").read_text(
    encoding="utf-8"
)
SURFACE = (ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda/ReaderGestureSurface.java").read_text(
    encoding="utf-8"
)


class WorldMenuFiveRequestedFixes(unittest.TestCase):
    def test_two_finger_back_uses_existing_confirmed_worldgen_exit(self):
        self.assertIn('listener.goBack();', SURFACE)
        self.assertIn('if( context.category_id() == "WORLDGEN_CONFIRM_DIALOG" &&', INPUT)
        self.assertIn('return "WORLDGEN_CONFIRM.QUIT";', INPUT)
        self.assertIn('ctxt.register_action( "WORLDGEN_CONFIRM.QUIT" );', WORLD)
        self.assertIn('query_yn( _( "Do you want to abort World Generation?" ) )', WORLD)

    def test_vertical_swipes_adjust_native_worldgen_slider_without_extra_rows(self):
        self.assertIn('listener.adjustFocusedSetting(dy < 0 ? 1 : -1)', SURFACE)
        self.assertIn('send("set_value", candidate.stripTrailingZeros().toPlainString())', ANDROID)
        self.assertIn('option.field_type = "int";', WORLD)
        self.assertIn('option.applied_value = std::to_string( wg_slevels[i] );', WORLD)
        self.assertIn('option.has_field_range = true;', WORLD)
        self.assertIn('option.field_step = 1;', WORLD)
        self.assertIn('return requested > current ? "RIGHT" : "LEFT";', WORLD)
        self.assertIn('wg_sliders[sel_opt - 1]->apply_opts(', WORLD)
        self.assertNotIn('const std::string action_id = "worldgen:slider:"', WORLD)
        self.assertNotIn('accessible_actions.emplace( action_id, increase ?', WORLD)

    def test_untranslated_speech_labels_are_now_vietnamese(self):
        for en, vi in [
            ("Create New World", "Tạo thế giới mới"),
            ("Active mod load order", "Thứ tự nạp mod đang bật"),
            ("Mod selection commands", "Các lệnh quản lý mod"),
            ("Options group: %s", "Nhóm tùy chọn: %s")
        ]:
            self.assertIn('msgid "' + en + '"\nmsgstr "' + vi + '"', SPEECH)

    def test_available_mod_double_tap_is_an_actual_enable_disable_toggle(self):
        # The available category still lists enabled mods. A second activate
        # must remove that very mod instead of repeating try_add (a no-op).
        self.assertIn('ctxt.register_action( "A11Y_INTERNAL_MOD_TOGGLE" );', WORLD)
        dispatch = WORLD.split('const auto target = reader_targets.find( intent.node );', 1)[1]
        dispatch = dispatch.split('const auto update_reader = [&]()', 1)[0]
        self.assertIn('if( intent.action == "activate" && !target->second.active_list )', dispatch)
        self.assertIn('return "A11Y_INTERNAL_MOD_TOGGLE";', dispatch)

        toggle = WORLD.split('} else if( action == "A11Y_INTERNAL_MOD_TOGGLE" ) {', 1)[1]
        toggle = toggle.split('} else if( action == "A11Y_INTERNAL_MANUAL_MOD_IMPORT" )', 1)[0]
        self.assertIn('std::find( active_mod_order.begin(), active_mod_order.end(), selected_id )', toggle)
        self.assertIn('mman_ui->try_rem(', toggle)
        self.assertIn('mman_ui->try_add( selected_id, active_mod_order );', toggle)
        self.assertIn('_( "Mod disabled: %s." )', toggle)
        self.assertIn('_( "Mod enabled: %s." )', toggle)

    def test_mod_toggle_is_checkable_and_even_unavailable_active_mod_can_be_removed(self):
        rows = WORLD.split('node.id = "mod:available:" + id.str();', 1)[1]
        rows = rows.split('if( active_mod_order.empty() ) {', 1)[0]
        self.assertIn('node.role = "checkbox";', rows)
        self.assertIn('node.checked = already_active;', rows)
        self.assertIn('!already_active && !mman_ui->mm_tree.is_available( id )', rows)
        self.assertIn('msgid "Mod enabled: %s."\nmsgstr "Đã bật mod: %s."', SPEECH)
        self.assertIn('msgid "Mod disabled: %s."\nmsgstr "Đã tắt mod: %s."', SPEECH)
        # Normal keyboard and mouse confirmation semantics remain untouched.
        self.assertIn('} else if( action == "CONFIRM" ) {', WORLD)
        self.assertIn('mman_ui->try_rem( cursel[1], active_mod_order );', WORLD)

    def test_world_option_group_is_not_checkbox_or_duplicate_expansion(self):
        segment = OPTIONS.split('if( item.type == ItemType::GroupHeader ) {', 1)[1].split(
            'row.parent_id = tree_root_published', 1)[0]
        self.assertIn('if( world_options_only ) {', segment)
        self.assertIn('row.role = "button";', segment)
        self.assertIn('_( "Options group: %s" )', segment)
        self.assertIn('} else {\n                    row.role = "checkbox";', segment)
        self.assertIn('row.expanded = groups_state[item.data];', OPTIONS)

    def test_spoken_counts_are_scoped_to_content_or_command_list(self):
        self.assertIn('const size_t content_count = snapshot.nodes.size();', WORLD)
        self.assertIn('is_content ? content_count : snapshot.nodes.size() - content_count', WORLD)
        self.assertIn('const size_t content_count = snapshot.nodes.size();', OPTIONS)
        self.assertIn('for( size_t i = content_count; i < snapshot.nodes.size(); ++i ) {', OPTIONS)
        self.assertIn('snapshot.nodes[i].total = static_cast<int>( snapshot.nodes.size() - content_count );', OPTIONS)


if __name__ == "__main__":
    unittest.main()
