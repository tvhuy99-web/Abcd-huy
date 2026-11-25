"""Verify the Vietnamese unit expansion at the speech-only queue boundary."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class VietnameseMeasurementSpeechTest(unittest.TestCase):
    def test_expansion_uses_selected_voice_at_queue_boundary(self):
        source = (JAVA / "ReaderSpeechQueue.java").read_text(encoding="utf-8")
        self.assertIn("ReaderSpokenText.expandVietnameseUnits(text, initLocale)", source)
        self.assertIn("Locale.forLanguageTag(preferences.getSpeechLanguageTag())", source)

    @unittest.skipUnless(shutil.which("javac") and shutil.which("java"), "JDK required")
    def test_units_and_locale_isolation(self):
        harness = r'''
package com.cleverraven.cataclysmdda;
import java.util.Locale;
public class UnitSpeechTest {
    static void eq(String actual, String expected) {
        if (!expected.equals(actual)) { throw new AssertionError(actual + " != " + expected); }
    }
    public static void main(String[] args) {
        Locale vi = Locale.forLanguageTag("vi-VN");
        String units = "20°C; 68°F; 60 km/h; 2 mSv/h; 5 kJ; 90 kcal; 100 mL";
        eq(ReaderSpokenText.expandVietnameseUnits(units, vi),
           "20 độ C; 68 độ F; 60 ki lô mét trên giờ; 2 mi li si vớt trên giờ; 5 ki lô jun; 90 ki lô ca lo; 100 mi li lít");
        eq(ReaderSpokenText.expandVietnameseUnits(units, Locale.ENGLISH), units);
        eq(ReaderSpokenText.expandVietnameseUnits(units, null), units);
        eq(ReaderSpokenText.clean(units), units);
        eq(ReaderSpokenText.expandVietnameseUnits("1.5mL, -10°C; 5 mL/s; 2 kJoule; 9 mL2", vi),
           "1.5 mi li lít, -10 độ C; 5 mL/s; 2 kJoule; 9 mL2");
        eq(ReaderSpokenText.clean("<color_red>5/5..</color>"), "5/5.");
    }
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "UnitSpeechTest.java"
            path.write_text(harness, encoding="utf-8")
            subprocess.run(["javac", "-encoding", "UTF-8", "-d", tmp,
                            str(JAVA / "ReaderSpokenText.java"), str(path)], check=True)
            subprocess.run(["java", "-cp", tmp,
                            "com.cleverraven.cataclysmdda.UnitSpeechTest"], check=True)


if __name__ == "__main__":
    unittest.main()
