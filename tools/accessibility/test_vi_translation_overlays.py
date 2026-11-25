"""Regression checks for pinned Vietnamese CDDA mod text overlays."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools.mods import apply_vi_overlays as overlays


class VietnameseModOverlaysTest(unittest.TestCase):
    def test_all_four_localized_mods_are_pinned(self):
        self.assertEqual(set(overlays.OVERLAYS),
                         {"mst_extra", "blazemod", "cataplusplus", "arcana"})
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

    def test_literal_percentages_are_not_printf_tokens(self):
        self.assertEqual(overlays.TOKEN.findall("10% slower, 30% slower"), [])
        self.assertEqual(overlays.TOKEN.findall("%s and %d"), ["%s", "%d"])

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


    def test_effect_message_types_are_not_translatable(self):
        accept = overlays.allowed_arcana_pointer
        for field in ("decay_messages", "apply_message"):
            self.assertTrue(accept("effects/effects.json", f"/14/{field}/0/0"))
            self.assertFalse(accept("effects/effects.json", f"/14/{field}/0/1"))
            self.assertTrue(overlays.is_effect_message_type_pointer(f"/14/{field}/0/1"))
            self.assertFalse(overlays.is_effect_message_type_pointer(f"/14/{field}/0/0"))

    def test_effect_overlay_preserves_original_types_for_all_mods(self):
        for kind, info in overlays.OVERLAYS.items():
            with self.subTest(mod=kind), tempfile.TemporaryDirectory() as folder:
                root = Path(folder) / info["folder"]
                root.mkdir()
                (root / "modinfo.json").write_text(json.dumps([
                    {"type": "MOD_INFO", "id": info["id"]}]), encoding="utf-8")
                path = root / "effects.json"
                source = [{"type": "effect_type", "id": "triffid_garland_buff",
                           "decay_messages": [["Fading.", "bad"]],
                           "apply_message": [["Restored.", "good"],
                                             ["Still active.", "neutral"],
                                             ["Changed.", "mixed"]]},
                          {"type": "effect_type", "id": "other",
                           "apply_message": "Restored.", "rating": "good"}]
                path.write_text(json.dumps(source), encoding="utf-8")
                manifest = {"effects.json": [
                    ["/0/decay_messages/0/0", "Hiệu ứng đang phai nhạt."],
                    ["/0/decay_messages/0/1", "xấu"],
                    ["/0/apply_message/0/1", "tốt"],
                    ["/0/apply_message/1/1", "trung tính"],
                    ["/0/apply_message/2/1", "hỗn hợp"],
                    ["/1/rating", "tốt"],
                ]}
                # The real Arcana source is byte-pinned; use a fixture pin
                # substitute only in this isolated unit test.
                fixture_info = {**info, "source_blobs": None, "expected_strings": 6}
                with patch.dict(overlays.OVERLAYS, {kind: fixture_info}), \
                        patch.object(overlays, "load_overlay", return_value=manifest):
                    self.assertEqual(overlays.apply(kind, folder, dry_run=True), (1, 1))
                    self.assertEqual(overlays.apply(kind, folder), (1, 1))
                result = json.loads(path.read_text(encoding="utf-8"))
                self.assertEqual(result[0]["decay_messages"],
                                 [["Hiệu ứng đang phai nhạt.", "bad"]])
                self.assertEqual(result[0]["apply_message"], source[0]["apply_message"])
                self.assertEqual(result[1]["rating"], "good")


if __name__ == "__main__":
    unittest.main()
