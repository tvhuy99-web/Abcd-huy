"""Contracts for the sixth manual-mod section and its global gesture help.

These are source-level regression checks. Actual Android file picker, voice,
native mod reload, ZIP import, and APK must also be tested on a device.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORLD = ROOT / "src/worldfactory.cpp"
READER = (ROOT / "android/app/src/main/java/com/cleverraven/"
          "cataclysmdda/GameReaderController.java")
ARCHIVE = (ROOT / "android/app/src/main/java/com/cleverraven/"
           "cataclysmdda/ManualModArchive.java")
ACTIVITY = (ROOT / "android/app/src/main/java/com/cleverraven/"
            "cataclysmdda/ManualModsActivity.java")
BRIDGE = ROOT / "src/a11y_android.cpp"
GESTURES = (ROOT / "android/app/src/main/java/com/cleverraven/"
            "cataclysmdda/BootstrapSelfVoiceSurface.java")


class ManualModImportContracts(unittest.TestCase):
    def setUp(self):
        self.world = WORLD.read_text(encoding="utf-8")
        self.reader = READER.read_text(encoding="utf-8")
        self.archive = ARCHIVE.read_text(encoding="utf-8")
        self.activity = ACTIVITY.read_text(encoding="utf-8")

    def test_sixth_root_comes_after_shared_commands(self):
        self.assertLess(self.world.index('tools_tab.id = "mod:tools"'),
                        self.world.index('manual_tab.id = "mod:manual"'))
        self.assertIn('import_button.id = "mod:manual:import"', self.world)
        self.assertIn('item.id = "mod:manual:item:" + id.str()', self.world)
        self.assertIn('manual_tab.label = "Nhập mod thủ công"', self.world)

    def test_double_tap_uses_native_world_specific_mod_manager(self):
        self.assertIn('"A11Y_INTERNAL_MANUAL_MOD_TOGGLE"', self.world)
        self.assertIn('mman_ui->try_add( *manual_toggle_id, active_mod_order )',
                      self.world)
        self.assertIn('mman_ui->try_rem( std::distance(', self.world)
        self.assertIn('PATH_INFO::user_moddir_path() / "manual"', self.world)

    def test_manual_mod_rows_are_actually_activatable_by_double_tap(self):
        # Android intentionally refuses to dispatch native activation unless
        # this exact action appears in the published semantic node.
        self.assertIn('import_button.actions = { "focus", "activate" };',
                      self.world)
        self.assertIn('item.actions = { "focus", "activate" };',
                      self.world)
        self.assertIn('if (!node.actions.contains("activate"))', self.reader)

    def test_desktop_build_never_links_android_only_mod_picker(self):
        self.assertIn(
            'if( action == "A11Y_INTERNAL_MANUAL_MOD_IMPORT" )', self.world
        )
        self.assertIn(
            '#if defined(__ANDROID__)\n'
            '            a11y::android::open_manual_mod_action( "import", "" );\n'
            '#endif', self.world
        )

    def test_reloading_only_on_game_thread(self):
        self.assertIn("take_manual_mods_changed()", self.world)
        self.assertIn("mman->reload_available_mods()", self.world)
        self.assertIn("pending_manual_mods_changed.store(", BRIDGE.read_text())
        self.assertIn("PushWakeEvent();", BRIDGE.read_text())
        self.assertIn("nativeQueueManualModsChanged();", self.activity)
        self.assertNotIn("reload_available_mods()", self.activity)

    def test_hold_adds_export_delete_but_always_keeps_guide_last(self):
        a = self.reader.index('private List<ReaderSnapshot.Node> contextualCommands()')
        b = self.reader.index('private boolean flattenedInventoryBrowse()', a)
        menu = self.reader[a:b]
        self.assertIn('MANUAL_MOD_EXPORT_NODE', menu)
        self.assertIn('MANUAL_MOD_DELETE_NODE', menu)
        self.assertLess(menu.index("MANUAL_MOD_EXPORT_NODE"),
                        menu.index("ReaderSnapshot.Node.gestureGuide("))
        self.assertLess(menu.index("MANUAL_MOD_DELETE_NODE"),
                        menu.index("ReaderSnapshot.Node.gestureGuide("))
        self.assertIn("listener.openContextActions()", GESTURES.read_text())

    def test_android_saf_and_bounded_extraction(self):
        self.assertIn("Intent.ACTION_OPEN_DOCUMENT", self.activity)
        self.assertIn("Intent.ACTION_CREATE_DOCUMENT", self.activity)
        self.assertIn("confirmDelete()", self.activity)
        self.assertIn("ZipInputStream", self.archive)
        for key in ["MAX_ZIP", "MAX_EXPANDED", "MAX_FILE", "MAX_FILES",
                    "getCanonicalFile()", "staging", "modinfo.json",
                    "target.exists()", "renameTo(target)"]:
            self.assertIn(key, self.archive)

    def test_delete_has_explicit_world_warning_and_confirmation(self):
        self.assertIn("a11yManualModConfirmDelete", self.activity)
        self.assertIn("addButton(page, R.string.a11yManualModCancel", self.activity)
        self.assertIn("addButton(page, R.string.a11yGestureGuideTitle", self.activity)
        self.assertNotIn("new AlertDialog.Builder(", self.activity)
        self.assertIn("showControls(homeControls)", self.activity)
        self.assertIn("ManualModArchive.deleteMod(", self.activity)


if __name__ == "__main__":
    unittest.main()
