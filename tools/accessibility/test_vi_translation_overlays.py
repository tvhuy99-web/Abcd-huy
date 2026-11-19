"""Regression checks for pinned Vietnamese CDDA mod text overlays."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools.mods import apply_vi_overlays as overlays


class VietnameseModOverlaysTest(unittest.TestCase):
    def test_all_three_localized_mods_are_pinned(self):
        self.assertEqual(set(overlays.OVERLAYS),
                         {"mst_extra", "blazemod", "cataplusplus"})
        for kind in overlays.OVERLAYS:
            self.assertEqual(len(overlays.OVERLAYS[kind]["revision"]), 40)
            self.assertTrue(overlays.load_overlay(kind))

    def test_cata_plus_plus_user_translation_is_dda_only(self):
        strings = overlays.load_overlay("cataplusplus")
        self.assertEqual(len(strings), 52)
        self.assertEqual(sum(len(entries) for entries in strings.values()), 1656)
        self.assertTrue(all(name.endswith(".json") and "nocts_cata_mod_BN" not in name
                            for name in strings))
        self.assertEqual(overlays.OVERLAYS["cataplusplus"]["id"], "Cata++")

    def test_technical_id_cannot_be_changed(self):
        for pointer in ["/0/id", "/0/type", "/0/copy-from", "/0/dependencies"]:
            with self.assertRaises(ValueError):
                overlays.target_for_pointer([{"id": "Cata++"}], pointer)

    def test_applying_text_keeps_game_data_unchanged(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder) / "CataclysmPlusPlus"
            root.mkdir(parents=True)
            source = [{"type": "MOD_INFO", "id": "Cata++", "dependencies": ["dda"],
                       "name": "Cata++"}]
            (root / "modinfo.json").write_text(json.dumps(source), encoding="utf-8")
            obj = [{"id": "sword", "type": "ITEM", "damage": 55,
                    "name": {"str": "Heavy sword"},
                    "description": "A blade hits for %s damage."}]
            (root / "items.json").write_text(json.dumps(obj), encoding="utf-8")
            translated = {"items.json": [["/0/name/str", "Kiếm nặng"],
                                        ["/0/description", "Lưỡi kiếm gây %s sát thương."]]}
            with patch.object(overlays, "load_overlay", return_value=translated):
                files, strings = overlays.apply("cataplusplus", folder)
            self.assertEqual((files, strings), (1, 2))
            after = json.loads((root / "items.json").read_text(encoding="utf-8"))
            self.assertEqual(after[0]["id"], "sword")
            self.assertEqual(after[0]["type"], "ITEM")
            self.assertEqual(after[0]["damage"], 55)
            self.assertEqual(after[0]["name"]["str"], "Kiếm nặng")

    def test_printf_placeholders_rejected_when_translation_changes_them(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder) / "CataclysmPlusPlus"
            root.mkdir(parents=True)
            (root / "modinfo.json").write_text(
                '[{"type":"MOD_INFO","id":"Cata++"}]', encoding="utf-8")
            source = '[{"description":"Item %s deals %d damage","id":"sword"}]'
            (root / "items.json").write_text(source, encoding="utf-8")
            with patch.object(overlays, "load_overlay", return_value={
                    "items.json": [["/0/description", "Vật phẩm %s gây sát thương"]]}):
                with self.assertRaisesRegex(ValueError, "Broken placeholder"):
                    overlays.apply("cataplusplus", folder)
            self.assertEqual((root / "items.json").read_text(encoding="utf-8"),
                             source)


if __name__ == "__main__":
    unittest.main()
