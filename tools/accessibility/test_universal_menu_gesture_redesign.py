"""Keep spoken menus, world alerts, and gestures consistent."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class UniversalMenuGestureRedesign(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.controller = (
            JAVA / "GameReaderController.java"
        ).read_text(encoding="utf-8")
        cls.surface = (
            JAVA / "ReaderGestureSurface.java"
        ).read_text(encoding="utf-8")
        cls.editor = (
            JAVA / "ReaderTextEditor.java"
        ).read_text(encoding="utf-8")
        cls.recognizer = (
            JAVA / "TwoFingerBackGesture.java"
        ).read_text(encoding="utf-8")

    def test_drawer_hides_auxiliary_commands_on_mixed_screens(self):
        code = self.controller
        self.assertIn('private static boolean isAuxiliaryCommand(', code)
        self.assertIn('if (containsContent) {', code)
        self.assertIn(
            'result.removeIf(GameReaderController::isAuxiliaryCommand);',
            code)
        self.assertIn(
            'if (commandPaletteOpen) { return contextualCommands(); }',
            code)
        self.assertIn('commandPaletteReturnFocus = focus;', code)
        self.assertIn('focus = commandPaletteReturnFocus;', code)
        self.assertIn(
            'if (commandPaletteOpen) { closeContextActions(); return; }',
            code)

    def test_long_press_opens_commands_and_two_finger_tap_reads_detail(self):
        self.assertIn('listener.openContextActions();', self.surface)
        self.assertIn('listener.twoFingerTap(count);', self.surface)
        self.assertIn('if (fingers == 2 && count == 1)', self.surface)
        self.assertIn('if (count == 1) { detail(); return; }', self.controller)
        self.assertIn('public void openContextActions()', self.controller)

    def test_menu_accept_cancel_are_not_world_scans(self):
        self.assertIn('TwoFingerBackGesture.matchesUp(event', self.surface)
        self.assertIn('TwoFingerBackGesture.matches(event', self.surface)
        self.assertIn('listener.confirm();', self.surface)
        self.assertIn('listener.goBack();', self.surface)
        self.assertIn('!listener.directWorldGesturesEnabled()', self.surface)
        self.assertIn('listener.scanDirection(', self.surface)
        self.assertIn('static boolean isUpSwipe(', self.recognizer)
        self.assertIn('static boolean matchesUp(', self.recognizer)

    def test_confirm_only_uses_native_validated_operations(self):
        code = self.controller
        self.assertIn('public void confirm() {', code)
        self.assertIn('chosen.actions.contains("apply")', code)
        self.assertIn('send("apply", "");', code)
        self.assertIn('accept.actions.contains("activate")', code)
        self.assertIn('directWorldGesturesEnabled()) { return; }', code)
        self.assertIn('R.string.a11yContextConfirmUnavailable', code)
        self.assertIn(
            'if (commandPaletteOpen) { closeContextActions(); return; }',
            code)

    def test_numeric_editor_digits_and_commit_cancel(self):
        code = self.editor
        self.assertIn('if (numeric) {', code)
        self.assertIn('group = this.quantityChoices ? "quantities" : "digits";', code)
        self.assertIn('if (quantityChoices && "quantities".equals(group))', code)
        self.assertIn('selected = 1;', code)
        self.assertIn('public void submitCurrent() {', code)
        self.assertIn('public void cancelInput() { listener.cancel(); }', code)
        self.assertIn('replaceInitialNumberOnFirstDigit', code)
        self.assertIn('editor.submitCurrent();', self.controller)
        self.assertIn('editor.cancelInput();', self.controller)

    def test_talkback_has_button_fallback_when_it_owns_gestures(self):
        code = self.controller
        self.assertIn('if (usesTouchExploration() && snapshot != null', code)
        self.assertIn(
            'commands.setOnClickListener(view -> openContextActions());',
            code)
        self.assertIn('details.setOnClickListener(view -> detail());', code)
        self.assertIn('accept.setOnClickListener(view -> confirm());', code)
        self.assertIn('back.setOnClickListener(view -> goBack());', code)

    def test_plain_inventory_does_not_announce_phantom_selection_amount(self):
        native = (ROOT / "src/inventory_ui.cpp").read_text(encoding="utf-8")
        self.assertIn('if( activation == "TOGGLE_ENTRY" ) {', native)
        self.assertIn('_( "Selected: %1$s. Available: %2$s." )', native)

    def test_browsing_wraps_bidirectionally_for_every_reader_list(self):
        ctrl = self.controller
        # All three global reader contexts wrap rather than clamp.
        self.assertIn(
            "debugFocus = Math.floorMod(debugFocus + delta, 4);",
            ctrl)
        self.assertIn(
            "hubActionChoice = Math.floorMod(hubActionChoice + delta, 2);",
            ctrl)
        self.assertIn(
            "gestureChoice = Math.floorMod(gestureChoice + delta,",
            ctrl)
        self.assertIn(
            "hubIndex = Math.floorMod(hubIndex + delta, options.size());",
            ctrl)
        self.assertIn(
            "selected = Math.floorMod(index() + delta, filtered.size());",
            ctrl)
        # Native number editor already wraps and stays short.
        self.assertIn(
            "selected = Math.floorMod(selected + delta, items.size());",
            self.editor)

    def test_focus_restores_item_count_and_keeps_details(self):
        ctrl = self.controller
        self.assertIn(
            "if (spokenPosition > 0 && spokenTotal > 0)",
            ctrl)
        self.assertIn("if (includeDetail && !node.detail.isEmpty())", ctrl)
        self.assertIn("if (!quantityRow) {", ctrl)
        self.assertIn("R.string.a11yPosition, selected + 1, entries.size()", ctrl)
        self.assertIn(
            'listener.speak(items.get(selected).label + ", "',
            self.editor)
        native = (ROOT / "src/inventory_ui.cpp").read_text(encoding="utf-8")
        self.assertIn('node.detail = location.describe( &owner.u );', native)
        self.assertIn('node.field_type = "int";', native)

    def test_visible_items_mark_world_tiles(self):
        native = (ROOT / "src/a11y_world.cpp").read_text(encoding="utf-8")
        self.assertIn("!tile.seen_items.empty()", native)
        self.assertIn('return _( "Visible items" );', native)
        self.assertIn(
            'here.role = noteworthy_here ? "alert" : "document";',
            native)
        self.assertIn('here.value = short_tile_alert( current_tile );', native)
        self.assertIn('here.label = _( "Death camera center" );', native)
        self.assertIn('boolean newlyNoteworthy = worldView', self.controller)
        self.assertIn(
            'if (newlyNoteworthy) { feedback(nextTile.value); }',
            self.controller)

    def test_world_tile_alert_speaks_visible_items_not_ordinary_floor(self):
        world = (ROOT / "src/a11y_world.cpp").read_text(encoding="utf-8")
        perception = (
            ROOT / "src/a11y_perception.cpp"
        ).read_text(encoding="utf-8")
        self.assertIn(
            "std::string short_tile_alert( const tile_observation &tile )",
            world)
        self.assertIn('_( "Item here: %s." )', world)
        self.assertIn('_( "Items here: %1$s, %2$s." )', world)
        self.assertIn('_( "Items here: %1$s and %2$d other types." )', world)
        self.assertIn('_( "Trap: %s." )', world)
        self.assertIn('if( !tile.known_traps.empty() ) {', world)
        self.assertIn('if( !tile.seen_items.empty() ) {', world)
        self.assertIn(
            'here.role = noteworthy_here ? "alert" : "document";',
            world)
        self.assertIn('here.value = current_tile.terrain;', world)
        self.assertIn('!tile.seen_items.empty()', world)
        # Only observed items, never a raw world scan.
        self.assertIn('here.sees_some_items( position, viewer )', perception)
        self.assertIn('here.could_see_items( position, viewer )', perception)
        self.assertIn('result.seen_items.push_back(', perception)

    def test_repeated_alerts_on_identical_neighbor_tiles(self):
        world = (ROOT / "src/a11y_world.cpp").read_text(encoding="utf-8")
        self.assertIn('"world:tile_position"', world)
        self.assertIn('get_avatar().pos_abs()', world)
        self.assertIn('location.actions = { "focus" };', world)
        self.assertIn(
            'previousSnapshot.find("world:tile_position")',
            self.controller)
        self.assertIn('next.find("world:tile_position")', self.controller)
        self.assertIn('boolean steppedToAnotherTile =', self.controller)
        self.assertIn(
            'steppedToAnotherTile || !"alert".equals(previousTile.role)',
            self.controller)
        self.assertIn(
            'if (newlyNoteworthy) { feedback(nextTile.value); }',
            self.controller)

    def test_hub_long_press_keeps_gesture_assignment_and_favorites(self):
        ctrl = self.controller
        self.assertIn('option.setOnLongClickListener(view -> {', ctrl)
        self.assertIn('showHubActionChoices();', ctrl)
        self.assertIn(
            'if (comprehensiveMenu) {\n            showHubActionChoices();',
            ctrl)
        self.assertIn('hubActionChoice = 0;', ctrl)
        self.assertIn(
            'if (choice == 0) {\n            showGestureChoices();',
            ctrl)
        self.assertIn('favorites.add(target, label)', ctrl)
        self.assertIn('favorites.remove(target)', ctrl)

    def test_vietnamese_alert_localization(self):
        vi = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")
        for phrase in ('Item here: %s.', 'Items here: %1$s, %2$s.',
                       'Items here: %1$s and %2$d other types.',
                       'Trap: %s.', 'Creature: %s.', 'Current position'):
            self.assertIn('msgid "' + phrase + '"', vi)

    def test_localized_accessibility_commands_exist(self):
        for locale in ("values", "values-vi"):
            xml = (
                ROOT / "android/app/src/main/res" / locale / "strings.xml"
            ).read_text(encoding="utf-8")
            for name in ("a11yContextCommands", "a11yContextCommandsClose",
                         "a11yNoContextCommands", "a11yContextConfirm",
                         "a11yContextBack", "a11yContextConfirmUnavailable"):
                self.assertIn('name="' + name + '"', xml)


if __name__ == "__main__":
    unittest.main()
