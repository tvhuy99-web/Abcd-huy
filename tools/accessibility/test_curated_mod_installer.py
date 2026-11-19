"""Offline regression checks for the curated community-mod installer."""
import io
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

from tools.mods.install_curated import extract_mod, install, read_catalog, verify_modinfo


def sample_zip(mod, extra=None):
    out = io.BytesIO()
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as archive:
        folder = "repo-revision/" + (mod["source_subdir"] + "/" if mod["source_subdir"] else "")
        info = [{"type": "MOD_INFO", "id": mod["mod_id"],
                 "dependencies": mod["required_mod_ids"], "name": mod["name"]}]
        archive.writestr(folder + "modinfo.json", json.dumps(info))
        archive.writestr(folder + "items.json", "[]")
        for key, value in (extra or {}).items():
            archive.writestr("repo-revision/" + key, value)
    return out.getvalue()


class CuratedModsTest(unittest.TestCase):
    def setUp(self):
        self.mods = {mod["id"]: mod for mod in read_catalog()}

    def test_five_distinct_mods_and_pinned_sources(self):
        self.assertEqual(len(self.mods), 5)
        self.assertTrue(all(len(x["revision"]) == 40 for x in self.mods.values()))
        self.assertEqual(self.mods["secronom"]["redistributable"], False)
        self.assertTrue(all(x["redistributable"] for k, x in self.mods.items() if k != "secronom"))

    def test_unlicensed_requires_explicit_local_flag(self):
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaisesRegex(ValueError, "local-use"):
                install(self.mods["secronom"], Path(root), downloader=lambda m: sample_zip(m))
            self.assertEqual(list(Path(root).iterdir()), [])

    def test_install_and_preserve_existing_mod(self):
        mod = self.mods["arcana"]
        with tempfile.TemporaryDirectory() as root:
            root = Path(root)
            dest = install(mod, root, downloader=lambda m: sample_zip(m))
            self.assertEqual(verify_modinfo(dest, mod)["id"], "Arcana")
            self.assertTrue((dest / "CURATED_SOURCE.txt").is_file())
            with self.assertRaises(FileExistsError):
                install(mod, root, downloader=lambda m: sample_zip(m))

    def test_reject_bad_id_without_exposing_partial_install(self):
        mod = self.mods["mst_extra"].copy()
        mod["mod_id"] = "wrong_id"
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaisesRegex(ValueError, "Unexpected MOD_INFO"):
                install(mod, Path(root), downloader=lambda m: sample_zip(self.mods["mst_extra"]))
            self.assertEqual(list(Path(root).iterdir()), [])

    def test_zip_traversal_is_blocked(self):
        mod = self.mods["cataplusplus"]
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaisesRegex(ValueError, "Unsafe ZIP"):
                extract_mod(sample_zip(mod, {"../escape.txt": "bad"}), mod, root)

    def test_selected_subfolder_does_not_import_siblings(self):
        mod = self.mods["cataplusplus"]
        with tempfile.TemporaryDirectory() as root:
            extract_mod(sample_zip(mod, {"other_mod/modinfo.json": "{}"}), mod, root)
            self.assertFalse((Path(root) / "other_mod").exists())


if __name__ == "__main__":
    unittest.main()
