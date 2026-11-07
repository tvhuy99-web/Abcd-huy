"""Otopack download stays outside the APK, is atomic and removes temporary cues."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
ANDROID = ROOT / "android/app/src/main"
JAVA = ANDROID / "java/com/cleverraven/cataclysmdda"


class OnDemandSoundpack(unittest.TestCase):
    def test_network_permissions_and_not_embedded_in_apk(self):
        manifest = (ANDROID / "AndroidManifest.xml").read_text(encoding="utf-8")
        installer = (JAVA / "OptionalSoundpackInstaller.java").read_text(encoding="utf-8")
        self.assertIn("android.permission.INTERNET", manifest)
        self.assertIn("android.permission.ACCESS_NETWORK_STATE", manifest)
        self.assertIn("https://codeload.github.com/Kenan2000/Otopack-Mods-Updates/zip/", installer)
        self.assertIn('SOURCE_COMMIT = "508da9f0e38ec996f14c930c58848024680ca2be"', installer)

    def test_installer_is_atomic_cancellable_and_defensive(self):
        s = (JAVA / "OptionalSoundpackInstaller.java").read_text(encoding="utf-8")
        self.assertIn('new File(gameRoot, "data/sound/" + PACK_DIRECTORY)', s)
        self.assertIn('".reader_otopack_staging"', s)
        self.assertIn('staging.renameTo(destination)', s)
        self.assertIn("if (!committed) { removeStaging(staging); }", s)
        self.assertIn("if (destination.exists())", s)
        self.assertIn('new File(target, "soundpack.txt")', s)
        self.assertIn('entry.getName()', s)
        self.assertIn('!safePath(relative)', s)
        self.assertIn('MAX_EXTRACTED_BYTES', s)
        self.assertIn('MAX_FILES', s)
        self.assertIn('Thread.currentThread().isInterrupted()', s)
        self.assertIn('getUsableSpace()', s)
        self.assertIn('new ZipInputStream(raw)', s)

    def test_startup_checks_before_game_with_wifi_and_skip(self):
        splash = (JAVA / "SplashScreen.java").read_text(encoding="utf-8")
        self.assertIn('prepareGameSoundpack();', splash)
        self.assertIn('OptionalSoundpackInstaller.isInstalled(gameRoot)', splash)
        self.assertIn('network.isActiveNetworkMetered()', splash)
        self.assertIn('showSoundpackMobileDataWarning();', splash)
        self.assertIn('startSoundpackDownload(gameRoot);', splash)
        self.assertIn('soundpackInstaller.cancel();', splash)
        self.assertIn('showSoundpackFailure', splash)
        self.assertIn('showBootstrapContent(content);', splash)
        self.assertIn('R.string.a11ySoundpackPlayWithout', splash)

    def test_native_selects_otopack_and_preserves_other_choices(self):
        options = (ROOT / "src/options.cpp").read_text(encoding="utf-8")
        self.assertIn('option.first == "otopack"', options)
        self.assertIn('has_otopack ? "otopack"', options)
        self.assertIn('SOUNDPACKS.find( "otopack" )', options)
        self.assertIn('get_option( "SOUNDPACKS" ).setValue( "otopack" )', options)
        self.assertIn('get_option( "SOUNDPACKS" ).getValue() == "basic"', options)

    def test_temporary_step_and_automatic_reader_sounds_are_gone(self):
        engine = (ROOT / "src/handle_action.cpp").read_text(encoding="utf-8")
        bridge = (ROOT / "src/a11y_android.cpp").read_text(encoding="utf-8")
        cues = (JAVA / "ReaderCuePlayer.java").read_text(encoding="utf-8")
        prefs = (JAVA / "ReaderPreferences.java").read_text(encoding="utf-8")
        self.assertNotIn('a11y::android::cue( "footstep" );', engine)
        self.assertNotIn('name != "footstep"', bridge)
        self.assertNotIn('R.raw.a11y_cue_step', cues)
        self.assertNotIn('public static final String FOOTSTEP', cues)
        self.assertIn('values.getBoolean("sound_cues", false)', prefs)
        self.assertFalse((ANDROID / "res/raw/a11y_cue_step.wav").exists())


if __name__ == "__main__":
    unittest.main()
