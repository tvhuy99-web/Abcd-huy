"""Source contracts for the universal, read-only accessibility gesture guide.

These checks cover routing, menu append order, focus safety and translations.
Device gesture recognition and native menu navigation still need runtime QA.
"""

from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
CTRL = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
GUIDE = (JAVA / "ReaderGestureGuide.java").read_text(encoding="utf-8")
SNAPSHOT = (JAVA / "ReaderSnapshot.java").read_text(encoding="utf-8")
HUB = (JAVA / "GameActionHub.java").read_text(encoding="utf-8")
GESTURES = (JAVA / "ReaderGestureSurface.java").read_text(encoding="utf-8")
VEHICLE = (ROOT / "src/vehicle_use.cpp").read_text(encoding="utf-8")


class ContextGestureGuideContracts(unittest.TestCase):
    def test_existing_long_hold_is_unchanged(self):
        self.assertIn("listener.openContextActions();", GESTURES)
        self.assertIn("public void onLongPress(MotionEvent event)", GESTURES)
        self.assertIn("public void openContextActions()", CTRL)
        self.assertIn('requestLiveWorldCommand("control_vehicle");', CTRL)
        self.assertIn("openHubSection(GameActionHub.COMBAT);", CTRL)

    def test_all_native_context_drawers_end_with_guide(self):
        commands = CTRL.split("private List<ReaderSnapshot.Node> contextualCommands()", 1)[1]
        commands = commands.split("private boolean flattenedInventoryBrowse()", 1)[0]
        self.assertIn("result.add(ReaderSnapshot.Node.gestureGuide(", commands)
        self.assertLess(commands.index("for (ReaderSnapshot.Node node : snapshot.nodes)"),
                        commands.index("result.add(ReaderSnapshot.Node.gestureGuide("))
        self.assertLess(commands.index("TARGET_TOGGLE_NODE"),
                        commands.index("result.add(ReaderSnapshot.Node.gestureGuide("))
        self.assertIn("if (snapshot == null || gestureGuideOpen) { return; }", CTRL)
        self.assertNotIn("if (!hasContent || commands.isEmpty())", CTRL)
        self.assertIn("ReaderSnapshot.Node.gestureGuide(", CTRL)
        self.assertIn('return localCommand("reader:gesture:guide", label)', SNAPSHOT)

    def test_driving_native_menu_keeps_actions_before_guide(self):
        menu = VEHICLE.split("void vehicle::build_interact_menu(", 1)[1].split(
            "void vehicle::interact_with(", 1
        )[0]
        self.assertIn('menu.add( _( "Gesture guide" ) )', menu)
        self.assertIn("a11y_manual_drive = !a11y_manual_drive;", menu)
        self.assertGreater(menu.index('menu.add( _( "Gesture guide" ) )'),
                           menu.index("a11y_manual_drive = !a11y_manual_drive;"))
        self.assertIn(".keep_menu_open()", menu)
        self.assertIn('guide.title = _( "Gesture guide: driving" );', menu)
        self.assertIn("guide.addentry( index++, true, 0, line );", menu)
        self.assertIn("guide.query();", menu)
        self.assertIn("Scan 30 tiles ahead of the vehicle.", menu)

    def test_combat_hub_and_hub_submenus_keep_final_guide(self):
        entries = CTRL.split("private List<GameActionHub.Entry> hubEntries()", 1)[1].split(
            "private String hubActionLabel(", 1
        )[0]
        self.assertIn("GameActionHub.OPEN_GESTURE_GUIDE", entries)
        self.assertIn("result.add(new GameActionHub.Entry(", entries)
        self.assertIn("case GameActionHub.OPEN_GESTURE_GUIDE:", CTRL)
        self.assertIn("openGestureGuide();", CTRL)
        self.assertIn("hubActionChoice = Math.floorMod(hubActionChoice + delta, 3);", CTRL)
        self.assertIn("int count = choosingHubAction ? 3", CTRL)
        self.assertIn("if (choice == 2) {", CTRL)
        self.assertIn("static final int OPEN_GESTURE_GUIDE = 11;", HUB)

    def test_inventory_pocket_actions_keep_existing_options(self):
        self.assertIn("R.string.a11yPocketOpen, R.string.a11yPocketOtherActions,", CTRL)
        self.assertIn("R.string.a11yGestureGuideTitle", CTRL)
        self.assertIn("Math.floorMod(pocketMenuChoice + delta, 3)", CTRL)
        self.assertIn("pocketMenuChoice + 1, 3", CTRL)
        picker = CTRL.split("private void activatePocketPicker()", 1)[1].split(
            "private void openPocketQuantityEditor(", 1
        )[0]
        self.assertIn("if (pocketMenuChoice == 0)", picker)
        self.assertIn("else if (pocketMenuChoice == 1)", picker)
        self.assertIn("openGestureGuide();", picker)

    def test_target_and_travel_existing_actions_survive(self):
        self.assertIn("TARGET_TOGGLE_NODE", CTRL)
        self.assertIn("TRAVEL_STATUS_NODE", CTRL)
        self.assertIn("targetingOriginalControls = !targetingOriginalControls;", CTRL)
        self.assertIn("travelContextDrawerState = travelState();", CTRL)
        self.assertIn("feedback(travelDetailSpeech(travelContextDrawerState));", CTRL)

    def test_guide_is_read_only_before_vehicle_pause_or_native_activation(self):
        activate = CTRL.split("public void activate()", 1)[1].split(
            "private void openEditor(", 1
        )[0]
        self.assertLess(activate.index("if (gestureGuideOpen)"),
                        activate.index("if (vehicleMode && directWorldGesturesEnabled())"))
        self.assertLess(activate.index("if (commandPaletteOpen && GESTURE_GUIDE_NODE.equals(focus))"),
                        activate.index("if (vehicleMode && directWorldGesturesEnabled())"))
        guide_methods = CTRL.split("private void openGestureGuide()", 1)[1].split(
            "private boolean enterReaderLayer(", 1
        )[0]
        self.assertNotIn('send("activate"', guide_methods)
        self.assertNotIn('requestVehicleAction(', guide_methods)
        self.assertNotIn('moveDirection(', guide_methods)
        self.assertIn("readCurrent();", guide_methods)
        self.assertIn("gestureGuideOpen = false;", guide_methods)

    def test_guide_has_one_focused_item_and_back_restores_original_screen(self):
        self.assertIn("gestureGuideIndex = Math.floorMod(gestureGuideIndex + delta,", CTRL)
        self.assertIn("gestureGuideRows.get(index)", CTRL)
        self.assertIn("row.setSelected(i == gestureGuideIndex)", CTRL)
        self.assertIn("gestureGuideSpeech()", CTRL)
        self.assertIn("if (gestureGuideOpen) { closeGestureGuide(); return; }", CTRL)
        self.assertIn("commandPaletteReturnFocus = focus;", CTRL)
        self.assertIn("focus = commandPaletteReturnFocus;", CTRL)
        self.assertIn("&& !commandPaletteOpen && !gestureGuideOpen", CTRL)
        self.assertIn("gestureGuideOpen = false;", CTRL)

    def test_help_swipes_navigate_even_when_underlying_screen_disabled_swipes(self):
        self.assertIn("default boolean gestureGuideActive()", GESTURES)
        self.assertIn("if (listener.gestureGuideActive())", GESTURES)
        self.assertIn("listener.navigate(horizontal);", GESTURES)
        self.assertIn("listener.navigate(vertical);", GESTURES)
        self.assertIn("public boolean gestureGuideActive()", CTRL)
        self.assertIn("return gestureGuideOpen;", CTRL)

    def test_modes_and_dynamic_user_bindings_are_scoped(self):
        self.assertIn('driving && "DEFAULTMODE".equals(category)', GUIDE)
        self.assertIn('combat && "DEFAULTMODE".equals(category)', GUIDE)
        for category in ("inventory", "crafting", "construction",
                         "surroundings", "targeting", "CHARACTER_CREATION"):
            self.assertIn('"' + category + '".equals(category)', GUIDE)
        self.assertIn("ReaderGestureBindings.CUSTOM_IDS", GUIDE)
        self.assertIn("bindings.targetFor(id)", GUIDE)
        self.assertIn("snapshot.find(target.substring(7))", GUIDE)
        self.assertIn("R.string.a11yGuideScanFront", GUIDE)
        self.assertIn("R.string.a11yGuideScanNorth", GUIDE)
        self.assertIn("R.string.a11yGuideCombatEnemy", GUIDE)
        self.assertIn("R.string.a11yGuideDrivePause", GUIDE)

    def test_all_help_strings_have_english_and_vietnamese_translations(self):
        names = set()
        import re
        for name in re.findall(r"R.string.(a11yGuide\w+|a11yGestureGuideTitle)", GUIDE):
            names.add(name)
        names.add("a11yGuideTravelStatus")
        for language in ("values", "values-vi"):
            resource = ROOT / "android/app/src/main/res" / language / "strings.xml"
            tree = ET.parse(resource)
            strings = [node.attrib["name"] for node in tree.findall("./string")]
            self.assertEqual(len(strings), len(set(strings)))
            for name in names:
                self.assertIn(name, strings)
        native = (ROOT / "lang/po/accessibility/vi.po").read_text("utf-8")
        self.assertIn('msgid "Gesture guide: driving"', native)
        self.assertIn('msgstr "Hướng dẫn cử chỉ lái xe"', native)


if __name__ == "__main__":
    unittest.main()
