"""Check Vietnamese action menu labels against their native commands."""
import json
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
PO = ROOT / "lang/po/accessibility/vi.po"
KEYS = ROOT / "data/raw/keybindings.json"
BUILDER = ROOT / "src/a11y_input.cpp"
UI = ROOT / "android/app/src/main/res/values-vi/strings.xml"

LABELS = {
    "Fire wielded item": "Bắn súng hoặc tấn công tầm xa",
    "Autoattack": "Đánh cận chiến một lần, kể cả tay không",
    "Burst-fire wielded item": "Bắn loạt bằng súng đang cầm",
    "Toggle attack mode of wielded item": "Đổi chế độ bắn hoặc đánh của súng",
    "Select default ammo for wielded weapon which do not use a magazine": "Chọn đạn ưu tiên cho súng nạp từng viên",
    "Throw item": "Chọn vật phẩm để ném",
    "Throw wielded item": "Ném vật phẩm đang cầm",
    "Reload item": "Chọn vật phẩm để nạp",
    "Reload weapon": "Tự nạp súng hoặc băng đạn phù hợp",
    "Reload wielded item": "Nạp vật phẩm đang cầm",
    "Use item": "Sử dụng hoặc kích hoạt vật phẩm",
    "Use wielded item": "Sử dụng vật phẩm đang cầm",
    "Wield": "Chọn vật phẩm để cầm trên tay",
    "Pick up items from all nearby tiles": "Chọn đồ để nhặt ở ô đứng và tám ô liền kề",
    "Pick up items from one nearby tile": "Chọn đồ để nhặt tại một ô gần",
    "Unload container into inventory (keep items)": "Tháo đạn hoặc lấy đồ từ vật chứa về túi",
    "Unload container onto ground (drop items)": "Lấy đồ từ vật chứa ra và đặt xuống đất",
    "Consume item (all items)": "Chọn vật phẩm để ăn, uống hoặc sử dụng",
    "Consume item (by type)": "Chọn loại vật phẩm để ăn, uống hoặc sử dụng",
    "Wait for several minutes": "Chọn khoảng thời gian cần chờ",
    "Pause": "Đứng yên một lượt",
    "Toggle auto travel mode": "Bật hoặc tắt chế độ tìm đường theo hướng",
    "Haul items along the ground": "Chọn đồ để kéo lê khi di chuyển",
    "Quick toggle hauling": "Bật hoặc tắt việc kéo lê đồ",
    "Zone activities": "Thực hiện công việc theo các vùng đã thiết lập",
    "Auto pickup manager": "Thiết lập quy tắc tự động nhặt đồ",
    "Recraft last recipe": "Chế tạo lại công thức vừa dùng",
    "Craft as long as possible": "Lặp lại chế tạo khi còn đủ điều kiện",
    "Grab something nearby": "Nắm hoặc buông vật thể gần đó",
    "Peek around corners": "Nhìn quanh góc mà không bước ra",
    "Advanced Inventory Manager": "Quản lý chuyển đồ giữa các vị trí",
    "Insert item": "Chọn đồ để cho vào vật chứa",
    "Open inventory": "Xem vật phẩm đang mang theo",
    "View message log": "Đọc lịch sử thông báo",
    "Toggle auto safe mode": "Bật hoặc tắt tự kích hoạt chế độ an toàn"
}
DETAILS = {
    (
        "Choose a gun you carry to aim and fire. If no gun is available, the "
        "game may use a reach attack or a vehicle turret. Ordinary adjacent "
        "melee uses a different command."
    ): (
        "Chọn một khẩu súng đang mang để ngắm và bắn. Khi không có súng, game có "
        "thể dùng đòn đánh có tầm với hoặc ụ súng trên xe. Đòn đánh cận chiến "
        "thông thường dùng lệnh khác."
    ),
    (
        "Fire a burst only if the gun you are holding supports burst or "
        "automatic fire. This does not perform a melee attack."
    ): (
        "Chỉ bắn loạt khi súng đang cầm hỗ trợ bắn loạt hoặc liên thanh. Lệnh "
        "này không đánh cận chiến."
    ),
    (
        "Cycle the firing and melee modes of the gun you are holding. This does "
        "not attack."
    ): (
        "Chuyển giữa các chế độ bắn hoặc chế độ đánh của súng đang cầm. Thao tác "
        "này không tấn công."
    ),
    (
        "Automatically choose a gun or compatible magazine to reload, starting "
        "with the gun in your hands. This may reload another gun in your "
        "inventory."
    ): (
        "Tự chọn súng hoặc băng đạn phù hợp để nạp, ưu tiên súng đang cầm. Lệnh "
        "cũng có thể nạp súng khác trong túi đồ."
    ),
    (
        "Reload the item in your hands if it supports reloading. This does not "
        "choose another item."
    ): (
        "Nạp vật phẩm đang cầm nếu có thể nạp. Lệnh này không chọn vật phẩm khác."
    ),
    (
        "Automatically choose a target within melee reach and attack once, even "
        "when unarmed. With no target, you may wait one turn if safe mode "
        "allows. This is not a toggle."
    ): (
        "Tự chọn mục tiêu trong tầm đánh cận chiến và đánh một lần, kể cả khi "
        "tay không. Nếu không có mục tiêu, nhân vật có thể chờ một lượt nếu chế "
        "độ an toàn cho phép. Đây không phải nút bật tắt."
    ),
}


class VietnameseHubWordingTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.po = PO.read_text("utf-8")
        cls.keybindings = json.loads(KEYS.read_text("utf-8"))
        cls.builder = BUILDER.read_text("utf-8")
        cls.ui = {
            node.attrib["name"]: node.text or ""
            for node in ET.parse(UI).getroot().findall("string")
        }

    def test_correct_labels_still_match_native_action_names(self):
        names = {
            entry["name"] if isinstance(entry["name"], str) else entry["name"].get("str")
            for entry in self.keybindings
            if entry.get("category") == "DEFAULTMODE" and entry.get("name")
        }
        for original in LABELS:
            with self.subTest(original=original):
                self.assertIn(original, names)

    def test_gettext_has_correct_label_and_detail_translations(self):
        for english, vietnamese in {**LABELS, **DETAILS}.items():
            with self.subTest(english=english):
                pair = ('msgid ' + json.dumps(english, ensure_ascii=False)
                        + chr(10) + 'msgstr '
                        + json.dumps(vietnamese, ensure_ascii=False))
                self.assertIn(pair, self.po)

    def test_native_help_distinguishes_melee_and_gunfire(self):
        for action in ('"fire"', '"fire_burst"', '"select_fire_mode"',
                       '"reload_weapon"', '"reload_wielded"', '"autoattack"'):
            self.assertIn('id == ' + action, self.builder)
        for phrase in ("Ordinary adjacent melee uses a different command.",
                       "With no target, you may wait one turn",
                       "This does not perform a melee attack.",
                       "This may reload another gun in your inventory."):
            self.assertIn(phrase, self.builder)

    def test_combat_gesture_labels_separate_attack_and_movement(self):
        prompt = self.ui["a11yCombatOn"].lower()
        self.assertLess(len(prompt), 135)
        for term in ("bắn", "đánh", "tầm vũ khí", "không bước", "hai ngón"):
            self.assertIn(term, prompt)
        for suffix in ("North", "South", "East", "West",
                       "Northeast", "Northwest", "Southeast", "Southwest"):
            self.assertIn("Tấn công",
                          self.ui["a11yGuideAttack" + suffix])
        self.assertIn("tay không", self.ui["a11yGuideNextWeapon"])
        self.assertIn("tay không", self.ui["a11yGuidePreviousWeapon"])


if __name__ == "__main__":
    unittest.main()
