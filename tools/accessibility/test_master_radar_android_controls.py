"""Guard Android opt-in radar controls, focus, and silent movement policy."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class RadarAndroidControls(unittest.TestCase):
    def test_native_mask_follows_lifecycle_and_is_disabled_by_default(self):
        reader = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        prefs = (JAVA / "ReaderPreferences.java").read_text(encoding="utf-8")
        self.assertIn("lastRadarMask = -1", reader)
        self.assertIn("enabled && foreground && windowFocused", reader)
        self.assertIn("preferences.activeRadarMask() : 0", reader)
        self.assertIn("CataclysmDDA.nativeConfigureAutomaticRadar(radarMask)", reader)
        self.assertIn("values.getBoolean(RADAR_MASTER, false)", prefs)

    def test_radar_alerts_are_suppressed_while_busy_or_driving(self):
        reader = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        self.assertIn('RADAR_ALERT_PREFIX = "A11Y_RADAR_ALERT:"', reader)
        self.assertIn('RADAR_DANGER_PREFIX = "A11Y_RADAR_DANGER:"', reader)
        self.assertIn("!preferences.isRadarEnabled() || comprehensiveMenu", reader)
        self.assertIn("vehicleMode || autonomousDriveTrip || spoken.isEmpty()", reader)
        self.assertIn("urgent ? ReaderSpeechQueue.URGENT", reader)

    def test_hub_preserves_original_groups_and_eight_categories(self):
        reader = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        self.assertIn("hubSection == GameActionHub.RADAR_SETTINGS", reader)
        self.assertIn("case GameActionHub.RADAR_OPTION:", reader)
        self.assertIn("preferences.setRadarChannelEnabled", reader)
        self.assertIn("hubSection = 13;", reader)
        self.assertIn("RADAR_CHANNEL_LANDMARKS", reader)
        self.assertIn("reader:radar:toggle", reader)
        self.assertIn("if (placeSurvey) {", reader)
        self.assertIn("openPlaceSurvey(message.substring(SCENE_PREFIX.length()))", reader)


if __name__ == "__main__":
    unittest.main()
