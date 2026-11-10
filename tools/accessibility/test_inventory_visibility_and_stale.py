"""Regressions: accessible nested inventory, safe gestures and live node actions.

These source checks deliberately span multiple screen adapters. Native
validation remains the authority for mutations; UI does not invent actions.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
INV = (SRC / "inventory_ui.cpp").read_text(encoding="utf-8")
WORLD = (SRC / "a11y_world.cpp").read_text(encoding="utf-8")
RUNTIME = (SRC / "a11y_runtime.cpp").read_text(encoding="utf-8")
PLAYER = (SRC / "player_display.cpp").read_text(encoding="utf-8")
CTRL = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")


class AccessibleInventoryVisibilityAndStale(unittest.TestCase):
    def test_normal_inventory_exposes_collapsed_and_nested_contents(self):
        self.assertIn("else if( !pickup_mode && !pocket->sealed()", INV)
        self.assertIn("for( const item *child : pocket->all_items_top() )", INV)
        self.assertIn("std::function<void( const item *, const std::string &, unsigned int )> expose", INV)
        self.assertIn("for( const item_pocket *inner : child->get_container_pockets() )", INV)
        self.assertIn("if( inner->sealed() )", INV)
        self.assertIn('item_node.actions = { "focus" };', INV)
        self.assertIn('pocket_node.value = "read_only";', INV)
        self.assertIn('pocket_node.has_children = !contents_rows.empty();', INV)
        self.assertIn('item_node.label = ancestors + " > " + item_node.label;', INV)
        browsing = INV.split("// Normal Inventory was only publishing pocket statuses.", 1)[1]
        browsing = browsing.split("// A sealed or non-browsable pocket", 1)[0]
        for mutator in ("toggle_entry(", "set_collapsed(", "store(", "obtain_item("):
            self.assertNotIn(mutator, browsing)

    def test_normal_item_and_pickup_pocket_share_hold_menu_not_double_tap(self):
        hold = CTRL.split("public void openContextActions()", 1)[1].split(
            "private void closeContextActions()", 1
        )[0]
        self.assertIn('"inventory".equals(snapshot.category)', hold)
        self.assertIn('"checkbox".equals(focused.role) || "button".equals(focused.role)', hold)
        self.assertIn('focused.id.startsWith("item:")', hold)
        self.assertIn("pocketStage = 1", hold)
        self.assertIn('pocketNodes(focused.id)', hold)
        self.assertIn("private boolean pocketAllowsSelection()", CTRL)
        self.assertIn('if ("sealed".equals(pocket.value))', CTRL)
        self.assertIn('if (!pocketAllowsSelection())', CTRL)
        self.assertIn('if (pocketStage == 3 && pocketAllowsSelection())', CTRL)

    def test_noninteractive_nodes_do_not_generate_stale_tap_receipts(self):
        # Previously skill categories were 'focus' only but Java still sent
        # 'activate'; C++ correctly refused it and spoke 'Choose again'.
        self.assertIn('header.id = "pd:skill:category:"', PLAYER)
        self.assertIn('header.actions = { "focus" };', PLAYER)
        tap = CTRL.split("public void activate()", 1)[1].split(
            "private void openEditor(", 1
        )[0]
        self.assertIn('if (!node.actions.contains("activate"))', tap)
        self.assertLess(tap.index('if (!node.actions.contains("activate"))'),
                        tap.index('send("activate", "");'))
        send = CTRL.split('private long send(String action, String value)', 1)[1].split(
            "private List<GameActionHub.Entry> hubEntries()", 1
        )[0]
        self.assertIn('if (!"cancel".equals(action))', send)
        self.assertIn('!target.actions.contains(action)', send)
        self.assertIn('if (!"focus".equals(action) && target != null)', send)
        self.assertIn('contexts.accepts( intent->token )', RUNTIME)
        self.assertIn('const bool valid_request = intent->request > screen.last_request;', RUNTIME)
        self.assertIn('validate_intent_value( *node, *intent, invalid_reason )', RUNTIME)

    def test_busy_swipe_has_only_one_short_lived_retry_and_no_native_backlog(self):
        self.assertIn('private String pendingWorldDirection = "";', CTRL)
        self.assertIn('pendingWorldDirection = focus;', CTRL)
        self.assertIn('pendingWorldSince = now;', CTRL)
        self.assertIn('pendingWorldRevision = owner.revision;', CTRL)
        self.assertIn('if (snapshot.revision <= pendingWorldRevision)', CTRL)
        self.assertIn('elapsed > 1200', CTRL)
        self.assertIn('clearPendingWorldDirection();', CTRL)
        self.assertIn('retryingWorldDirection = true;', CTRL)
        self.assertIn('if (worldView) {\n                    retryPendingWorldDirection();', CTRL)
        inbox = (SRC / "a11y_android.cpp").read_text("utf-8")
        self.assertIn("if( !world_step_ready ||", inbox)
        self.assertIn("world_step_ready = false;", inbox)
        self.assertNotIn("std::deque<semantic_intent> pendingWalkingSteps", inbox)

    def test_item_condition_markers_are_speech_only_and_explicit(self):
        self.assertIn('const std::string durability_bar =', INV)
        self.assertIn('remove_color_tags( location->durability_indicator() )', INV)
        self.assertIn('const std::string rules_marker = "⁺";', INV)
        self.assertIn('node.label.erase( mark, durability_bar.size() );', INV)
        self.assertIn('_( "Reinforced item." )', INV)
        self.assertIn('_( "Item shows wear or damage." )', INV)
        self.assertIn('_( "Item condition intact." )', INV)
        self.assertIn('_( "Pickup allow or deny rule marker." )', INV)
        self.assertIn('node.detail +=', INV)
        # Native item status and inventory save are not modified for speech.
        clean = INV.split("const std::string durability_bar =", 1)[1].split(
            "// A plain inventory selection opens", 1
        )[0]
        self.assertNotIn("set_damage(", clean)
        self.assertNotIn("set_flag(", clean)

    def test_health_neutral_labels_are_omitted_not_invented(self):
        status = WORLD.split("std::string player_status()", 1)[1].split(
            "const int stamina =", 1
        )[0]
        self.assertIn('const auto append_if_present =', status)
        self.assertIn('if( !clean.empty() )', status)
        for value in ('"Hunger: %s"', '"Thirst: %s"',
                      '"Sleepiness: %s"', '"Pain: %s"'):
            self.assertIn(value, status)

    def test_no_unrequested_speech_deduplication(self):
        self.assertNotIn("sameSpeechDeDup", CTRL)
        self.assertNotIn("skipRepeatedFocusAnnouncements", CTRL)


if __name__ == "__main__":
    unittest.main()
