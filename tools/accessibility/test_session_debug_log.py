"""Regression contracts for one manually controlled journal per app launch."""
import unittest
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
RES = ROOT / "android/app/src/main/res"
SRC = ROOT / "src"


class SessionDebugLogSourceTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.log = (JAVA / "SessionDebugLog.java").read_text("utf-8")
        cls.activity = (JAVA / "DebugLogActivity.java").read_text("utf-8")
        cls.menu = (SRC / "main_menu.cpp").read_text("utf-8")
        cls.native = (SRC / "a11y_android.cpp").read_text("utf-8")
        cls.game = (JAVA / "CataclysmDDA.java").read_text("utf-8")
        cls.splash = (JAVA / "SplashScreen.java").read_text("utf-8")
        cls.tts = (JAVA / "AndroidSpeech.java").read_text("utf-8")
        cls.reader = (JAVA / "GameReaderController.java").read_text("utf-8")
        cls.gestures = (JAVA / "ReaderGestureSurface.java").read_text("utf-8")

    def test_main_menu_can_open_manual_journal_controls(self):
        self.assertIn("DEBUG_LOG,", self.menu)
        self.assertIn('pgettext( "Main Menu", "<D|d>ebug Log" )', self.menu)
        self.assertIn("main_menu_opts::DEBUG_LOG", self.menu)
        self.assertIn("a11y::android::open_debug_log();", self.menu)
        self.assertIn("onAccessibilityOpenDebugLog", self.native)
        self.assertIn("new Intent(this, DebugLogActivity.class)", self.game)
        self.assertIn('android:name="DebugLogActivity"',
                      (RES.parent / "AndroidManifest.xml").read_text("utf-8"))

    def test_single_recording_switch_starts_and_stops_all_diagnostics(self):
        ordered = ["enabled = new CheckBox(this)",
                   "export = new Button(this)",
                   "clear = new Button(this)"]
        indexes = [self.activity.index(name) for name in ordered]
        self.assertEqual(indexes, sorted(indexes))
        self.assertNotIn("CheckBox profiling", self.activity)
        self.assertNotIn("spokenOnlyWorld = new CheckBox(this)", self.activity)
        accessibility = (JAVA / "AccessibilitySettingsActivity.java").read_text("utf-8")
        self.assertIn("setSpokenOnlyWorldEnabled(spokenOnly)", accessibility)
        self.assertIn("SessionDebugLog.setEnabled(this, checked)", self.activity)
        self.assertIn("CataclysmDDA.nativeConfigureReaderPerformance(", self.activity)
        self.assertIn("reader.isSpokenOnlyWorldEnabled(), checked", self.activity)
        self.assertIn('getBoolean("enabled", false)', self.log)
        self.assertIn("log.recording = enabled;", self.log)
        self.assertIn("SessionDebugLog.isRecording()", self.reader)
        self.assertNotIn("isPerformanceProfilingEnabled()", self.reader)

    def test_fresh_file_for_each_launch_and_no_legacy_crash_recovery(self):
        self.assertIn('new File(directory, ACTIVE)', self.log)
        self.assertIn("freshJournal.exists() && !freshJournal.delete()", self.log)
        self.assertIn('new FileOutputStream(freshJournal, false)', self.log)
        self.assertIn('activeFile = freshJournal;', self.log)
        self.assertIn('{"active.marker", "crash_recovery.txt"}', self.log)
        self.assertIn('record("SESSION", "START app="', self.log)
        self.assertIn('recordLocked("SESSION", "RECORDING_ENABLED', self.log)
        self.assertNotIn("activeFile.renameTo(crashFile)", self.log)
        self.assertNotIn("hasCrash(", self.log)
        self.assertNotIn("hasCrash(", self.activity)
        self.assertNotIn("crash_recovery.txt", self.activity)
        self.assertNotIn("debug.log.prev", self.log)
        self.assertNotIn("NATIVE_LOG", self.log)
        self.assertNotIn("normal_exit delete_active", self.log)

    def test_manual_off_means_no_events_even_crashes(self):
        self.assertIn("if (log != null && log.recording)", self.log)
        self.assertIn("if (!recording || closed) { return; }", self.log)
        self.assertIn("if (fatal.compareAndSet(false, true) && recording)", self.log)
        self.assertIn("log.recording = false;", self.log)
        self.assertIn("return log != null && log.recording && !log.closed;", self.log)
        self.assertIn('if( !performance_profile_active.load( std::memory_order_acquire ) )',
                      self.native)
        self.assertNotIn("auto_paused", self.native)
        self.assertNotIn("stable_walking_steps >= 48", self.native)
        self.assertNotIn("setVerboseWorldTrace", self.log)

    def test_current_launch_export_only_even_when_recording_off(self):
        self.assertIn("public static byte[] snapshot(Context context)", self.log)
        self.assertIn("return readBounded(log.activeFile);", self.log)
        self.assertIn("SessionDebugLog.snapshot(this)", self.activity)
        self.assertIn("Intent.ACTION_CREATE_DOCUMENT", self.activity)
        self.assertIn("getContentResolver().openOutputStream", self.activity)
        self.assertIn("MAX_BYTES = 5L * 1024 * 1024", self.log)
        self.assertIn("KEEP_BYTES = 4L * 1024 * 1024", self.log)
        self.assertIn("trimOldest()", self.log)
        self.assertIn("SessionDebugLog.finishNormally();", self.game)
        self.assertIn("a11y::android::normal_exit();", self.menu)
        self.assertNotIn("SessionDebugLog.finishNormally();", self.splash)
        self.assertIn("Thread.setDefaultUncaughtExceptionHandler", self.log)

    def test_tts_gestures_native_traces_present_only_when_on(self):
        self.assertIn('SessionDebugLog.event("TTS_REQUEST"', self.tts)
        self.assertIn('SessionDebugLog.event("TTS_EVENT"', self.tts)
        self.assertIn('SessionDebugLog.event("TTS_STATE"', self.tts)
        self.assertIn('SessionDebugLog.event("GESTURE"', self.gestures)
        self.assertIn('SessionDebugLog.event("TOUCH"', self.gestures)
        self.assertIn('SessionDebugLog.event("INTENT_RESULT"', self.reader)
        self.assertIn('SessionDebugLog.event("SCREEN"', self.reader)
        self.assertIn('SessionDebugLog.event("NATIVE_ERROR"', self.game)
        self.assertIn("onAccessibilityNativeDiagnostic", self.native)
        self.assertIn('diagnostic_event( "NATIVE_ACTION"', (
            SRC / "handle_action.cpp").read_text("utf-8"))

    def test_localized_manual_switch_names(self):
        for locale in ("values", "values-vi"):
            xml = ET.parse(RES / locale / "strings.xml").getroot()
            keys = {x.attrib.get("name") for x in xml.findall("string")}
            for label in ("a11yDebugLogTitle", "a11yDebugLogEnabled",
                          "a11yDebugLogExport", "a11yDebugLogClear",
                          "a11ySpokenOnlyWorld"):
                self.assertIn(label, keys)
        translation = (ROOT / "lang/po/accessibility/vi.po").read_text(
            encoding="utf-8")
        self.assertIn('msgid "<D|d>ebug Log"', translation)
        self.assertIn('msgstr "<N|n>hật ký gỡ lỗi"', translation)


if __name__ == "__main__":
    unittest.main()
