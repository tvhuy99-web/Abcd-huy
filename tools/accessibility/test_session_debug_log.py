"""Regression coverage for opt-in exportable, crash-safe session logging."""
import unittest
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
RES = ROOT / "android/app/src/main/res"


class SessionDebugLogSourceTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.log = (JAVA / "SessionDebugLog.java").read_text("utf-8")
        cls.activity = (JAVA / "DebugLogActivity.java").read_text("utf-8")
        cls.menu = (ROOT / "src/main_menu.cpp").read_text("utf-8")
        cls.native = (ROOT / "src/a11y_android.cpp").read_text("utf-8")
        cls.game = (JAVA / "CataclysmDDA.java").read_text("utf-8")
        cls.splash = (JAVA / "SplashScreen.java").read_text("utf-8")
        cls.tts = (JAVA / "AndroidSpeech.java").read_text("utf-8")
        cls.reader = (JAVA / "GameReaderController.java").read_text("utf-8")
        cls.gestures = (JAVA / "ReaderGestureSurface.java").read_text(
            "utf-8"
        )

    def test_tenth_main_menu_item_and_native_android_route(self):
        self.assertIn('DEBUG_LOG,', self.menu)
        self.assertIn('pgettext( "Main Menu", "<D|d>ebug Log" )', self.menu)
        self.assertIn('main_menu_opts::DEBUG_LOG', self.menu)
        self.assertIn('a11y::android::open_debug_log();', self.menu)
        self.assertIn('onAccessibilityOpenDebugLog', self.native)
        self.assertIn('new Intent(this, DebugLogActivity.class)', self.game)
        self.assertIn('android:name="DebugLogActivity"',
                      (RES.parent / "AndroidManifest.xml").read_text("utf-8"))

    def test_three_only_ui_controls_in_order(self):
        ordered = [
            "enabled = new CheckBox(this)",
            "export = new Button(this)",
            "clear = new Button(this)",
        ]
        index = [self.activity.index(item) for item in ordered]
        self.assertEqual(index, sorted(index))
        self.assertIn('enabled.setChecked(SessionDebugLog.enabled(this))',
                      self.activity)
        self.assertIn('SessionDebugLog.setEnabled(this, checked)',
                      self.activity)
        self.assertIn('SessionDebugLog.clear(this)', self.activity)

    def test_five_mebibyte_ring_and_previous_crash_preservation(self):
        self.assertIn('MAX_BYTES = 5L * 1024 * 1024', self.log)
        self.assertIn('KEEP_BYTES = 4L * 1024 * 1024', self.log)
        self.assertIn("trimOldest()", self.log)
        self.assertIn('readBounded(activeFile)', self.log)
        self.assertIn('private static final String MARKER = "active.marker"',
                      self.log)
        self.assertIn('activeFile.renameTo(crashFile)', self.log)
        self.assertIn('new FileOutputStream(activeFile, false)', self.log)
        self.assertIn('log.markerFile.delete()', self.log)
        self.assertIn('SessionDebugLog.finishNormally();', self.game)
        self.assertIn('a11y::android::normal_exit();', self.menu)
        self.assertNotIn('SessionDebugLog.finishNormally();', self.splash)
        self.assertIn('log.crashFile.exists()', self.log)

    def test_export_is_explicit_and_covers_native_debug(self):
        self.assertIn("Intent.ACTION_CREATE_DOCUMENT", self.activity)
        self.assertIn("getContentResolver().openOutputStream", self.activity)
        self.assertIn(
            'new String[] {"crash.log", "debug.log.prev", "debug.log"}',
            self.log,
        )
        self.assertIn('"[EXPORT TRUNCATED TO 5 MiB]', self.log)
        self.assertIn('SessionDebugLog.hasCrash(this)', self.activity)
        self.assertIn('SessionDebugLog.snapshot(this, crash)', self.activity)

    def test_tts_gesture_intent_crash_and_native_latency_are_recorded(self):
        self.assertIn('SessionDebugLog.event("TTS_REQUEST"', self.tts)
        self.assertIn('SessionDebugLog.event("TTS_EVENT"', self.tts)
        self.assertIn('SessionDebugLog.event("TTS_STATE"', self.tts)
        self.assertIn('SessionDebugLog.event("GESTURE"', self.gestures)
        self.assertIn('SessionDebugLog.event("TOUCH"', self.gestures)
        self.assertIn('SessionDebugLog.event("INTENT_RESULT"', self.reader)
        self.assertIn('SessionDebugLog.event("SCREEN"', self.reader)
        self.assertIn('SessionDebugLog.event("NATIVE_ERROR"', self.game)
        self.assertIn('Thread.setDefaultUncaughtExceptionHandler', self.log)
        self.assertIn('onAccessibilityNativeDiagnostic', self.native)
        self.assertIn('diagnostic_event( "NATIVE_ACTION"', (
            ROOT / "src/handle_action.cpp"
        ).read_text("utf-8"))

    def test_localized_button_names(self):
        for locale in ("values", "values-vi"):
            xml = ET.parse(RES / locale / "strings.xml").getroot()
            keys = {x.attrib.get("name") for x in xml.findall("string")}
            for label in ("a11yDebugLogTitle", "a11yDebugLogEnabled",
                          "a11yDebugLogExport", "a11yDebugLogClear"):
                self.assertIn(label, keys)
        translation = (
            ROOT / "lang/po/accessibility/vi.po"
        ).read_text(encoding="utf-8")
        self.assertIn('msgid "<D|d>ebug Log"', translation)
        self.assertIn('msgstr "<N|n>hật ký gỡ lỗi"', translation)


if __name__ == "__main__":
    unittest.main()
