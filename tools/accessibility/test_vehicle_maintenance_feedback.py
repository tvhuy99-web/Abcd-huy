"""Source-level guardrails for accessible vehicle maintenance.

These verify target identity, truthful availability and non-duplicating speech.
The game's native vehicle tests and real Android TTS remain necessary.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SRC = (ROOT / "src/veh_interact.cpp").read_text(encoding="utf-8")
HEADER = (ROOT / "src/veh_interact.h").read_text(encoding="utf-8")
PO = (ROOT / "lang/po/accessibility/vi.po").read_text(encoding="utf-8")
MESSAGES = (ROOT / "src/messages.cpp").read_text(encoding="utf-8")
REPAIR = (ROOT / "src/veh_utils.cpp").read_text(encoding="utf-8")


class VehicleMaintenanceFeedbackTests(unittest.TestCase):

    def test_reader_availability_scan_restores_selected_part(self):
        adapter = SRC.split("} else if( current_mode == mode::remove", 1)[1]
        adapter = adapter.split("snapshot.nodes.insert", 1)[0]
        self.assertIn("const vehicle_part *saved_part = owner.sel_vehicle_part;", adapter)
        self.assertIn("const vpart_info *saved_part_info = owner.sel_vpart_info;", adapter)
        self.assertIn("owner.sel_vehicle_part = saved_part;", adapter)
        self.assertIn("owner.sel_vpart_info = saved_part_info;", adapter)
        self.assertLess(adapter.index("can_remove_part( here, part_index,"),
                        adapter.index("owner.sel_vehicle_part = saved_part;"))

    def test_confirm_revalidates_and_pins_actual_removed_part(self):
        handler = SRC.split("void veh_interact::do_remove(", 1)[1]
        handler = handler.split("void veh_interact::do_siphon(", 1)[0]
        self.assertIn("const bool confirm = action == \"REMOVE\" || action == \"CONFIRM\";", handler)
        self.assertIn("const int chosen_part = parts_here[pos];", handler)
        self.assertIn("if( confirm && !can_remove_part( here, chosen_part, player_character ) )", handler)
        self.assertIn("sel_vehicle_part = &veh->part( chosen_part );", handler)
        self.assertIn("sel_vpart_info = &sel_vehicle_part->info();", handler)
        self.assertLess(handler.index("if( confirm && !can_remove_part("),
                        handler.index("sel_cmd = VEHICLE_REMOVE;"))

    def test_immediate_activate_rechecks_new_repair_or_install_row(self):
        install = SRC.split("void veh_interact::do_install(", 1)[1]
        install = install.split("bool veh_interact::move_in_list(", 1)[0]
        repair = SRC.split("void veh_interact::do_repair(", 1)[1]
        repair = repair.split("void veh_interact::do_mend(", 1)[0]
        for action in (install, repair):
            self.assertIn("bool pending_confirm = false;", action)
            self.assertIn('pending_confirm ? "CONFIRM" : main_context.handle_input()', action)
            self.assertIn("pending_confirm = true;", action)
            self.assertIn("continue;", action)
        self.assertIn("selected_candidate != rendered_candidate", install)
        self.assertIn("parts_here[need_repair[pos]] != rendered_part_index", repair)

    def test_repair_ui_uses_actual_requirements_and_brief_reason(self):
        self.assertIn("bool repair_can_confirm = false;", HEADER)
        self.assertIn("std::string repair_block_reason;", HEADER)
        self.assertIn("repair_can_confirm = ok;", SRC)
        self.assertIn("node.enabled = owner.repair_can_confirm;", SRC)
        self.assertIn("node.disabled_reason = owner.repair_block_reason.empty()", SRC)
        self.assertIn("reader_missing_vehicle_requirements( requirements", SRC)
        self.assertIn("if( ( action == \"REPAIR\" || action == \"CONFIRM\" ) && !ok )", SRC)
        self.assertIn("reader_vehicle_rejection( repair_block_reason );", SRC)
        self.assertIn("reader_install_unavailable( *vp, owner.msg,", SRC)
        self.assertIn('node.value = _( "Ready to install." );', SRC)

    def test_narration_does_not_claim_instant_completion(self):
        assign = SRC.split("void veh_interact::assign_activity(", 1)[1]
        assign = assign.split("void orient_part(", 1)[0]
        self.assertIn("Started repairing %s.", assign)
        self.assertIn("!player_character.activity.is_null()", assign)
        self.assertNotIn("Repair completed", assign)
        self.assertIn("a11y::announce_game_message(", MESSAGES)
        self.assertIn("You repair the %1$s's %2$s.", REPAIR)

    def test_vehicle_maintenance_rejections_are_spoken(self):
        mend = SRC.split("void veh_interact::do_mend(", 1)[1]
        mend = mend.split("void veh_interact::do_refill(", 1)[0]
        refill = SRC.split("void veh_interact::do_refill(", 1)[1]
        refill = refill.split("void veh_interact::calc_overview(", 1)[0]
        self.assertGreaterEqual(mend.count("reader_vehicle_rejection( *msg );"), 4)
        self.assertGreaterEqual(refill.count("reader_vehicle_rejection( *msg );"), 2)

    def test_new_speech_has_vietnamese_gettext_entries(self):
        for key in ("Ready to repair.", "Ready to install.", "Ready to remove.",
                    "Missing required materials.", "Started repairing %s.",
                    "Started removing %s."):
            self.assertIn('msgid "' + key + '"', PO)


if __name__ == "__main__":
    unittest.main()
