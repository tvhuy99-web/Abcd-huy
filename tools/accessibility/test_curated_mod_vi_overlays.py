"""Regression tests for Vietnamese replacement of four pinned community mods."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools.mods import apply_vi_overlays


class CuratedVietnameseModsTest(unittest.TestCase):
    def test_all_pinned_overlays_decode_and_have_expected_text(self):
        expected = {
            "mst_extra": (23, 235, "MST_Extra"),
            "blazemod": (50, 703, "Blazemod"),
            "cataplusplus": (52, 1656, "CataclysmPlusPlus"),
            "arcana": (185, 6794, "Arcana_StrangeAeons"),
        }
        for name, (files, strings, folder) in expected.items():
            with self.subTest(mod=name):
                manifest = apply_vi_overlays.load_overlay(name)
                self.assertEqual(len(manifest), files)
                self.assertEqual(sum(len(x) for x in manifest.values()), strings)
                self.assertEqual(apply_vi_overlays.OVERLAYS[name]["folder"], folder)
                self.assertTrue(all(not p.startswith("/") and ".." not in Path(p).parts
                                    for p in manifest))
                self.assertTrue(all(isinstance(value, str) and value
                                    for patches in manifest.values()
                                    for pointer, value in patches))

    def test_no_identifier_changes_and_bright_nights_is_excluded(self):
        for name in ("mst_extra", "blazemod", "cataplusplus", "arcana"):
            manifest = apply_vi_overlays.load_overlay(name)
            self.assertFalse(any("MST_Extra_BN" in path for path in manifest))
            technical = ("/id", "/type", "/copy-from", "/dependencies", "/path")
            self.assertFalse(any(pointer.endswith(technical)
                                 for patches in manifest.values()
                                 for pointer, _ in patches))

    def test_sample_application_preserves_all_technical_fields(self):
        with tempfile.TemporaryDirectory() as temporary:
            mods = Path(temporary)
            folder = mods / "MST_Extra"
            folder.mkdir()
            modinfo = [{"type": "MOD_INFO", "id": "MST_Extra",
                        "name": "MST Extra", "dependencies": ["dda"]}]
            (folder / "modinfo.json").write_text(json.dumps(modinfo), encoding="utf-8")
            (folder / "items").mkdir()
            item = [{"type": "ITEM", "id": "unmodified_id",
                     "name": {"str": "clay oven"},
                     "description": "A clay oven.",
                     "price": 45, "weight": "5 kg", "flags": ["FIRE"]}]
            path = folder / "items/test.json"
            path.write_text(json.dumps(item), encoding="utf-8")
            selection = {"items/test.json": [
                ["/0/name/str", "lò đất sét"],
                ["/0/description", "Một lò đất sét."],
            ]}
            with patch.object(apply_vi_overlays, "load_overlay", return_value=selection):
                self.assertEqual(apply_vi_overlays.apply("mst_extra", mods, dry_run=True),
                                 (1, 2))
                self.assertIn("clay oven", path.read_text(encoding="utf-8"))
                self.assertEqual(apply_vi_overlays.apply("mst_extra", mods), (1, 2))
            updated = json.loads(path.read_text(encoding="utf-8"))[0]
            self.assertEqual(updated["id"], "unmodified_id")
            self.assertEqual(updated["price"], 45)
            self.assertEqual(updated["flags"], ["FIRE"])
            self.assertEqual(updated["name"]["str"], "lò đất sét")

    def test_arcana_manifest_is_pinned_to_exact_upstream(self):
        manifest = apply_vi_overlays.load_overlay("arcana")
        info = apply_vi_overlays.OVERLAYS["arcana"]
        pin = json.loads((Path(apply_vi_overlays.__file__).parent /
                          info["source_blobs"]).read_text(encoding="utf-8"))
        self.assertEqual(pin["revision"], info["revision"])
        self.assertEqual(len(pin["files"]), 339)
        self.assertEqual(set(manifest) - set(pin["files"]), set())

    def test_arcana_text_selection_never_touches_game_data(self):
        accept = apply_vi_overlays.allowed_arcana_pointer
        self.assertTrue(accept("npcs/TALK_HERMIT.json", "/0/responses/0/text"))
        self.assertTrue(accept("spells/spells_item.json", "/0/name/str"))
        self.assertTrue(accept("monsters/speech.json", "/0/sound"))
        self.assertFalse(accept("modinfo.json", "/0/id"))
        self.assertFalse(accept("overmap_and_mapgen/mapgen.json", "/0/object/rows/0"))
        self.assertFalse(accept("items/melee.json", "/0/price"))
        self.assertFalse(accept("monsters/monsters.json", "/0/flags/0"))

    def test_rejects_technical_pointer(self):
        with self.assertRaisesRegex(ValueError, "technical field"):
            apply_vi_overlays.target_for_pointer({"id": "foo"}, "/id")


if __name__ == "__main__":
    unittest.main()
