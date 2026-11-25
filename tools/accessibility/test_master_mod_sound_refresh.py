"""Preserve native refresh hooks for newly imported Android mods and soundpacks."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2] / "src"


class MasterNativeRefreshContracts(unittest.TestCase):
    def test_mod_registry_can_be_refreshed_after_saf_import(self):
        header = (ROOT / "mod_manager.h").read_text(encoding="utf-8")
        body = (ROOT / "mod_manager.cpp").read_text(encoding="utf-8")
        self.assertIn("void reload_available_mods();", header)
        reload = body.split("void mod_manager::reload_available_mods()", 1)[1]
        reload = reload.split("void mod_manager::refresh_mod_list()", 1)[0]
        self.assertIn("refresh_mod_list();", reload)
        self.assertIn("set_usable_mods();", reload)
        self.assertIn("load_mods_from( PATH_INFO::user_moddir_path() )", body)

    def test_soundpack_registry_rebuilds_option_choices(self):
        header = (ROOT / "options.h").read_text(encoding="utf-8")
        body = (ROOT / "options.cpp").read_text(encoding="utf-8")
        self.assertIn("refresh_soundpacks();", header)
        refreshed = body.split("options_manager::refresh_soundpacks()", 1)[1]
        refreshed = refreshed.split("options_manager::get_langs_with_translation_files()", 1)[0]
        self.assertIn("build_soundpacks_list()", refreshed)
        self.assertIn('options.find( "SOUNDPACKS" )', refreshed)
        self.assertIn("it->second.vItems = found;", refreshed)
        self.assertIn("return found;", refreshed)

    def test_muted_combat_announcements_remain_raii_scoped(self):
        header = (ROOT / "messages.h").read_text(encoding="utf-8")
        self.assertIn("class mute_combat_speech", header)
        self.assertIn("[[maybe_unused]] bool muted;", header)
        self.assertIn("~mute_combat_speech();", header)


if __name__ == "__main__":
    unittest.main()
