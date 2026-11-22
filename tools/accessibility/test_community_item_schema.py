"""Offline tests for the upstream item schema migration applied to APK assets."""
import json
from pathlib import Path
import tempfile
import unittest

from tools.mods.migrate_item_schema import migrate_document, migrate_item, migrate_mods


class CommunityItemSchemaTest(unittest.TestCase):
    def test_mst_ammo_armor_and_food_subtypes(self):
        for slot, props in (
                ("AMMO", {"damage": {"damage_type": "stab", "amount": 8}, "count": 2}),
                ("ARMOR", {"armor": [{"covers": ["torso"], "coverage": 50}], "warmth": 6}),
                ("COMESTIBLE", {"charges": 1, "calories": 110, "quench": 15})):
            with self.subTest(slot=slot):
                item = {"type": "ITEM", "id": "example", "subtype": [slot],
                        "name": {"str": "Vật phẩm"}, **props}
                expected = dict(item)
                del expected["subtype"]
                expected["subtypes"] = [slot]
                self.assertEqual(migrate_item(item), expected)
                self.assertEqual(migrate_item(expected), expected)
                self.assertIn("subtype", item)

    def test_mst_generic_canteen_preserves_armor_and_pockets(self):
        old = {"type": "GENERIC", "id": "canteen_birchbark",
               "pocket_data": [{"pocket_type": "CONTAINER", "max_contains_volume": "1500 ml"}],
               "armor_data": {"sided": True, "armor": [{"coverage": 10, "encumbrance": 4}]}}
        new = migrate_item(old)
        self.assertEqual(new["type"], "ITEM")
        self.assertEqual(new["subtypes"], ["ARMOR"])
        self.assertTrue(new["sided"])
        self.assertEqual(new["armor"], old["armor_data"]["armor"])
        self.assertEqual(new["pocket_data"], old["pocket_data"])
        self.assertNotIn("armor_data", new)
        self.assertEqual(migrate_item(new), new)

    def test_legacy_item_types_and_inheritance(self):
        old = {"type": "TOOL_ARMOR", "id": "example", "copy-from": "base",
               "max_charges": 4, "warmth": 10, "extend": {"subtype": ["BOOK"]}}
        new = migrate_item(old)
        self.assertEqual(new["subtypes"], ["TOOL", "ARMOR"])
        self.assertEqual(new["extend"], {"subtypes": ["BOOK"]})
        self.assertEqual(new["copy-from"], "base")
        self.assertEqual(new["max_charges"], 4)
        self.assertEqual(new["warmth"], 10)
        self.assertEqual(migrate_item({"type": "GENERIC", "id": "jar"}),
                         {"type": "ITEM", "id": "jar"})

    def test_generic_and_combined_legacy_subtypes(self):
        obj = {"type": "ITEM", "id": "distortion_amp_motor_item",
               "subtypes": ["GENERIC"]}
        self.assertEqual(migrate_item(obj), {"type": "ITEM", "id": obj["id"], "subtypes": []})
        combo = {"type": "ITEM", "subtype": ["GENERIC", "TOOL_ARMOR", "TOOL"]}
        self.assertEqual(migrate_item(combo)["subtypes"], ["TOOL", "ARMOR"])
        self.assertEqual(migrate_item(migrate_item(combo)), migrate_item(combo))

    def test_alcohol_moves_to_consumption_flag_without_losing_other_actions(self):
        old = {"type": "ITEM", "subtype": ["COMESTIBLE"], "id": "posca_hard",
               "copy-from": "posca", "use_action": "ALCOHOL_WEAK",
               "extend": {"flags": ["NO_AUTO_CONSUME"]}, "calories": 110}
        new = migrate_item(old)
        self.assertEqual(new["extend"]["flags"], ["NO_AUTO_CONSUME", "ALCOHOL_WEAK"])
        self.assertNotIn("use_action", new)
        self.assertEqual(new["calories"], 110)
        self.assertEqual(migrate_item(new), new)
        other = {"type": "COMESTIBLE", "use_action": ["ALCOHOL", {"type": "heal"}]}
        self.assertEqual(migrate_item(other)["use_action"], [{"type": "heal"}])
        self.assertEqual(migrate_item(other)["flags"], ["ALCOHOL"])

    def test_nonitems_and_nested_data_are_untouched(self):
        objects = [{"type": "item_group", "subtype": "collection", "id": "tools_survival"},
                   {"type": "recipe", "result": "jar", "nested": {"type": "GENERIC"}},
                   {"type": "ITEM", "id": "modern", "subtypes": ["TOOL"],
                    "pocket_data": [{"pocket_type": "CONTAINER"}]}]
        self.assertEqual(migrate_document(objects, "example.json"), (objects, 0))

    def test_conflicts_are_not_silently_overwritten(self):
        with self.assertRaisesRegex(ValueError, "Conflicting subtype"):
            migrate_item({"type": "ITEM", "subtype": ["AMMO"], "subtypes": ["ARMOR"]})
        with self.assertRaisesRegex(ValueError, "Conflicting armor"):
            migrate_item({"type": "GENERIC", "armor": [], "armor_data": {"armor": [1]}})

    def test_folder_migration_is_idempotent_and_checks_all_before_writing(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            mod = root / "test_mod"
            mod.mkdir()
            (mod / "modinfo.json").write_text("[]", encoding="utf-8")
            path = mod / "a.json"
            old = '[{"type":"GENERIC","id":"jar","name":"Bình"}]'
            path.write_text(old, encoding="utf-8")
            bad = mod / "b.json"
            bad.write_text('[{"type":"ITEM","subtype":["INVALID"]}]', encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Unknown item subtype"):
                migrate_mods(root, ("test_mod",))
            self.assertEqual(path.read_text(encoding="utf-8"), old)
            bad.unlink()
            with self.assertRaisesRegex(ValueError, "Unmigrated"):
                migrate_mods(root, ("test_mod",), check=True)
            self.assertEqual(migrate_mods(root, ("test_mod",)), 1)
            self.assertEqual(json.loads(path.read_text(encoding="utf-8"))[0]["name"], "Bình")
            self.assertEqual(migrate_mods(root, ("test_mod",), check=True), 0)


if __name__ == "__main__":
    unittest.main()
