"""Regression contracts ensuring 100% gameplay screen accessibility coverage."""
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "src"
VI_PO = Path(__file__).resolve().parents[2] / "lang/po/accessibility/vi.po"


class RemainingScreensCoverageTest(unittest.TestCase):
    def test_wave1_five_previously_blind_screens_have_screen_scope(self):
        checks = [
            ("npctalk_rules.cpp", 'a11y::screen_scope reader( &ctxt, "follower-rules"', "follower:rule:engagement"),
            ("iexamine.cpp", 'a11y::screen_scope vend_reader( &ctxt, "vending-machine"', 'node.id = "vend:item:"'),
            ("ui_extended_description.cpp", 'a11y::screen_scope reader( &ctxt, "extended-description"', 'node.id = "ext:tab:"'),
            ("martialarts.cpp", 'a11y::screen_scope reader( &ctxt, "martial-art-details"', "ma:section:general"),
            ("character_health.cpp", 'a11y::screen_scope reader( &ctxt, "blood-test-results"', 'node.id = "blood:effect:"'),
        ]
        for filename, scope_token, node_token in checks:
            with self.subTest(file=filename):
                text = (SRC / filename).read_text(encoding="utf-8")
                self.assertIn(scope_token, text)
                self.assertIn(node_token, text)

    def test_body_status_ui_covers_all_three_tabs(self):
        text = (SRC / "body_status_ui.cpp").read_text(encoding="utf-8")
        self.assertIn('"body-status"', text)
        self.assertIn('"body-status-medical"', text)
        self.assertIn('"body-status-armor"', text)
        self.assertIn('node.id = "body:med:"', text)
        self.assertIn('node.id = "body:armor:"', text)
        self.assertIn("clothing_info::clothing_protection(", text)

    def test_wave2_teleporter_modular_suit_and_vehicle_shape_covered(self):
        tele = (SRC / "magic_teleporter_list.cpp").read_text(encoding="utf-8")
        self.assertIn("teleport_selector.desc_enabled = true;", tele)
        self.assertIn("teleport_selector.addentry_desc(", tele)

        iuse = (SRC / "iuse.cpp").read_text(encoding="utf-8")
        self.assertIn('a11y::screen_scope suit_reader( &ctxt, "modular-suit"', iuse)
        self.assertIn('node.id = "suit:pocket:"', iuse)

        vshape = (SRC / "veh_shape.cpp").read_text(encoding="utf-8")
        self.assertIn('a11y::screen_scope shape_reader( &ctxt, "vehicle-shape"', vshape)
        self.assertIn('node.id = "vehshape:tile:"', vshape)

    def test_new_screen_titles_have_vietnamese_translations(self):
        vi = VI_PO.read_text(encoding="utf-8")
        for phrase in ("Máy bán hàng tự động", "Mô tả mở rộng", "Chi tiết môn võ thuật",
                       "Tình trạng y tế", "Sắp xếp giáp", "Đổi hình dạng bộ phận phương tiện"):
            self.assertIn(phrase, vi)



    def test_final_audit_ux_fixes_are_present(self):
        body = (SRC / "body_status_ui.cpp").read_text("utf-8")
        self.assertLess(body.index("std::optional<a11y::screen_scope> body_reader;"),
                        body.index("while( !bs_uistate.quit_menu )"))
        self.assertIn("get_effects_from_bp", body)

        ext = (SRC / "ui_extended_description.cpp").read_text("utf-8")
        self.assertIn("cur_target = switch_target;", ext)

        blood = (SRC / "character_health.cpp").read_text("utf-8")
        self.assertIn("focused = intent.node;", blood)

        veh = (SRC / "veh_shape.cpp").read_text("utf-8")
        self.assertIn('( action == "TIMEOUT" )', veh)


if __name__ == "__main__":
    unittest.main()
