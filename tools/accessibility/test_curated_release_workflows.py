"""Compile the embedded Python APK verification scripts before releasing builds."""
import ast
from pathlib import Path
import textwrap
import unittest

ROOT = Path(__file__).resolve().parents[2] / ".github" / "workflows"


class CuratedReleaseWorkflowTests(unittest.TestCase):
    def test_apk_verification_scripts_have_valid_python_syntax(self):
        for filename, step_name in (
            ("android-accessibility.yml", "Verify APK contents and checksums"),
            ("android-accessibility-release-trial.yml",
             "Validate installable signed ARM64 APK"),
        ):
            with self.subTest(filename=filename):
                content = (ROOT / filename).read_text(encoding="utf-8")
                section = content.split("- name: " + step_name, 1)[1]
                body = section.split("python3 - <<'PY'\n", 1)[1]
                script = body.split("\n          PY", 1)[0]
                ast.parse(textwrap.dedent(script))

    def test_four_mod_manifest_verification_and_concurrency(self):
        for filename in ("android-accessibility.yml",
                         "android-accessibility-release-trial.yml"):
            content = (ROOT / filename).read_text(encoding="utf-8")
            for mod in ("CataclysmPlusPlus", "Arcana_StrangeAeons",
                        "Blazemod", "MST_Extra"):
                self.assertIn("assets/data/mods/" + mod + "/modinfo.json", content)
        validation = (ROOT / "curated-vietnamese-mods.yml").read_text(
            encoding="utf-8")
        self.assertIn("concurrency:\n", validation)
        self.assertIn("cancel-in-progress: true", validation)


if __name__ == "__main__":
    unittest.main()
