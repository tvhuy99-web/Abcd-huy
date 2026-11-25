"""Ensure spoken radar is opt-in, category-aware, and localized."""

import re
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
ANDROID = ROOT / "android/app"
RADAR = (
    "Title", "Manage", "On", "Off", "Items", "Monsters",
    "Places", "Npcs", "Doors", "Hazards", "Vehicles", "Landmarks",
)


class MasterRadarPreferences(unittest.TestCase):
    def test_radar_opt_in_and_independent_channel_settings(self):
        src = (ANDROID / "src/main/java/com/cleverraven/cataclysmdda/ReaderPreferences.java").read_text(encoding="utf-8")
        self.assertIn('values.getBoolean(RADAR_MASTER, false)', src)
        self.assertIn("RADAR_CHANNEL_ALL = (1 << 8) - 1", src)
        self.assertIn("activeRadarMask()", src)
        self.assertIn("setRadarChannelEnabled(int bit, boolean enabled)", src)
        self.assertIn("isRadarEnabled() ? radarChannels() : 0", src)
        self.assertIn("isDirectionalSonarEnabled()", src)

    def test_every_radar_label_has_english_and_vietnamese_resources(self):
        for folder in ("values", "values-vi"):
            with self.subTest(folder=folder):
                xml = (ANDROID / "src/main/res" / folder / "strings.xml").read_text(encoding="utf-8")
                for suffix in RADAR:
                    name = "a11yRadar" + suffix
                    self.assertEqual(len(re.findall('name="' + name + '"', xml)), 1)

    def test_hub_keeps_saved_group_indices(self):
        hub = (ANDROID / "src/main/java/com/cleverraven/cataclysmdda/GameActionHub.java").read_text(encoding="utf-8")
        self.assertIn("RADAR_SETTINGS = 18", hub)
        self.assertIn("RADAR_OPTION = 12", hub)
        self.assertIn('"reader:radar:toggle", "zones"', hub)
        self.assertIn('"reader:scene:survey", "listitems"', hub)


if __name__ == "__main__":
    unittest.main()
