"""Regression checks for non-remappable gameplay gestures and spoken character details.

On-device instrumentation still verifies actual touch timing and TalkBack behavior.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
ANDROID_TEST = ROOT / "android/app/src/androidTest/java/com/cleverraven/cataclysmdda"


def source(name):
    return (JAVA / name).read_text("utf-8")


class LockedGestureAndCharacterDetailsContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bindings = source("ReaderGestureBindings.java")
        cls.controller = source("GameReaderController.java")
        cls.surface = source("ReaderGestureSurface.java")
        cls.prefs = source("ReaderPreferences.java")
        cls.setup = source("ReaderSetupActivity.java")
        cls.world = (ROOT / "src/a11y_world.cpp").read_text("utf-8")

    def test_only_25_unused_gestures_remain_customizable(self):
        b = self.bindings
        block = b.split("static final String[] IDS = {", 1)[1].split("};", 1)[0]
        ids = re.findall(r'"([^"]+)"', block)
        self.assertEqual(len(ids), 36)
        locked = ["swipe_up", "swipe_down", "swipe_left", "swipe_right",
                  "tap_2_1", "tap_2_2", "tap_2_3", "tap_3_1",
                  "tap_3_2", "tap_3_3", "hold_1"]
        self.assertEqual(len(locked), 11)
        for gesture in locked:
            self.assertIn('"' + gesture + '".equals(gesture)', b)
        for free in ("tap_4_1", "tap_4_2", "tap_4_3",
                     "hold_2", "hold_3", "swipe_3_up", "swipe_4_right",
                     "swipe_up_left", "swipe_down_up"):
            self.assertIn(f'"{free}"', block)
            self.assertNotIn(f'"{free}".equals(gesture)', b)
        self.assertIn("if (!isReserved(IDS[i]))", b)
        self.assertIn("CUSTOM_IDS = gestures.toArray", b)
        self.assertIn("CUSTOM_LABELS = new int[labels.size()]", b)

    def test_existing_overrides_cannot_escape_persisted_map_filter(self):
        b = self.bindings
        self.assertIn("writeBindings(readBindings());", b)
        self.assertIn("!isReserved(gesture) && !target.isEmpty()", b)
        self.assertIn("indexOf(gesture) < 0 || isReserved(gesture)", b)
        self.assertIn('if (indexOf(gesture) < 0 || isReserved(gesture)', b)
        self.assertIn("static int customIndexOf(String gesture)", b)

    def test_ui_only_offers_free_gestures_and_cannot_restore_overrides(self):
        c = self.controller
        self.assertIn("ReaderGestureBindings.CUSTOM_IDS.length", c)
        self.assertIn("ReaderGestureBindings.CUSTOM_IDS[chosen]", c)
        self.assertIn("ReaderGestureBindings.CUSTOM_LABELS[index]", c)
        self.assertIn("ReaderGestureBindings.customIndexOf(existing)", c)
        self.assertIn("ReaderGestureBindings.isReserved(gestureId)", c)
        self.assertNotIn("ReaderGestureBindings.IDS.length + 2", c)

    def test_builtin_1_and_2_finger_gestures_are_fixed(self):
        s = self.surface
        self.assertIn('listener.openContextActions();', s)
        self.assertIn('if (fingers == 2) {', s)
        self.assertIn("listener.twoFingerTap(count);", s)
        self.assertIn("listener.threeFingerTap();", s)
        self.assertIn("listener.characterDetails();", s)
        dispatch = s.split("private void dispatchMultiTap(", 1)[1]
        self.assertLess(dispatch.index("listener.characterDetails();"),
                        dispatch.index("listener.handleCustomGesture(gesture)"))
        self.assertIn("listener.scanDirection(", s)
        self.assertIn("listener.openActionMenu();", s)

    def test_combat_two_finger_swipes_use_world_movement_without_cap(self):
        c = self.controller
        # Three-finger movement is inactive during combat but its custom
        # map-mode bindings must remain available outside combat.
        segment = c.split("public boolean handleCustomGesture(", 1)[1]
        segment = segment.split("gestureBindings.targetFor(gestureId)", 1)[0]
        for gesture in ("swipe_3_up", "swipe_3_down",
                        "swipe_3_left", "swipe_3_right"):
            self.assertIn(f'case "{gesture}":', segment)
            self.assertNotIn(f'case "{gesture}": moveInCombat(', segment)
            self.assertNotIn(f'"{gesture}".equals(gesture)', self.bindings)
        scan = c.split("public void scanDirection(", 1)[1]
        scan = scan.split("public void twoFingerTap(", 1)[0]
        self.assertIn("moveInCombat(dx, dy);", scan)
        self.assertIn("requestLiveWorldCommand(id);", scan)
        self.assertIn("private boolean worldStepAllowed()", c)
        self.assertIn("return !combatMode && worldStepAllowed();", c)
        self.assertIn("private void moveInCombat(int dx, int dy)", c)
        self.assertIn("boolean enabled = combatMode && worldStepAllowed();", c)
        self.assertIn("dispatchWorldMovement(dx, dy);", c)
        retry = c.split("private void retryPendingWorldDirection()", 1)[1]
        self.assertIn("!worldStepAllowed()", retry)
        self.assertIn("!retryingWorldDirection && worldStepAllowed()", c)
        self.assertNotIn("combatMode ? 1 :", c)
        device = (ANDROID_TEST / "GameMenuGestureDispatchTest.java").read_text("utf-8")
        self.assertIn("threeFingerSwipeNoLongerMovesInCombatAndRestoresCustomBindingOnMap", device)
        self.assertIn("allFourTwoFingerCombatDirectionsAreDetected", device)
        self.assertIn("1, actions.savedThreeFingerSwipes);", device)


    def test_native_combat_buttons_obey_computed_world_gate(self):
        world = self.world
        # A default snapshot starts with swipe_movement_allowed=False.
        # The eight combat buttons must be constructed only AFTER the native
        # world eligibility gate has been computed.
        gate = world.index("result.swipe_movement_allowed =")
        attack = world.index("attack.enabled = result.swipe_movement_allowed;")
        weapon = world.index("node.enabled = result.swipe_movement_allowed;")
        self.assertLess(gate, attack)
        self.assertLess(gate, weapon)
        self.assertIn('reader:combat:attack:', world)
        self.assertIn('reader:combat:weapon-next', world)
        self.assertIn('reader:combat:weapon-prev', world)
        self.assertIn('reader:combat:reload', world)

    def test_swipe_mapping_ui_removed_and_old_prefs_ignored(self):
        self.assertIn("return GESTURE_ACTION_NAVIGATE;", self.prefs)
        self.assertNotIn("getGestureAction(\"horizontal_swipe_action\"", self.prefs)
        self.assertNotIn("getGestureAction(\"vertical_swipe_action\"", self.prefs)
        setup = self.setup.split("TextView gestureHeading", 1)[1].split(
            "sample = button(", 1)[0]
        self.assertNotIn("cycleSwipeAction(true)", setup)
        self.assertNotIn("cycleSwipeAction(false)", setup)
        self.assertNotIn("view -> tryGestureMapping()", setup)
        self.assertIn("a11y_restore_gesture_defaults", setup)

    def test_character_details_are_real_native_snapshot_and_readonly(self):
        w = self.world
        self.assertIn("const std::string status_summary = player_status();", w)
        self.assertIn('"world:character_detail"', w)
        self.assertIn("character.get_name()", w)
        for attribute in ("get_str()", "get_dex()", "get_int()", "get_per()"):
            self.assertIn("character." + attribute, w)
        self.assertIn("character.weapname_simple()", w)
        self.assertIn('_( "End of character information." )', w)
        self.assertIn("void characterDetails()", self.controller)
        self.assertIn('snapshot.find("world:character_detail")', self.controller)
        self.assertIn("ReaderSpeechQueue.LONG_TEXT", self.controller)
        self.assertIn('"world-character-detail"', self.controller)
        self.assertNotIn('send("activate", "")', self.controller.split(
            "public void characterDetails()", 1)[1].split("public void openActionMenu", 1)[0])

    def test_vietnamese_spoken_labels_and_android_gesture_tests(self):
        po = (ROOT / "lang/po/accessibility/vi.po").read_text("utf-8")
        for label in ("Character information. %s.",
                      "Strength: %1$d. Dexterity: %2$d. Intelligence: %3$d. Perception: %4$d.",
                      "Wielding: %s.", "End of character information."):
            self.assertIn('msgid "' + label + '"', po)
        gesture_device = (ANDROID_TEST / "GameMenuGestureDispatchTest.java").read_text("utf-8")
        bindings_device = (ANDROID_TEST / "ReaderGestureBindingsTest.java").read_text("utf-8")
        self.assertIn("doubleThreeFingerTapReadsDetails", gesture_device)
        self.assertIn("singleFingerHoldAlwaysOpensContextActions", gesture_device)
        self.assertIn("Should reject reserved gesture", bindings_device)
        self.assertIn('assertEquals(25, ReaderGestureBindings.CUSTOM_IDS.length)', bindings_device)


if __name__ == "__main__":
    unittest.main()
