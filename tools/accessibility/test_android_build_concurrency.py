"""One newest Android APK build on master, with four native compile jobs.

This source-contract suite complements GitHub Actions YAML validation.
It ensures future changes do not restore duplicate master APK workflows or
silently lower the supported runner native parallelism.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORKFLOWS = ROOT / ".github" / "workflows"

FILES = {
    "candidate": WORKFLOWS / "android-accessibility.yml",
    "trial": WORKFLOWS / "android-accessibility-release-trial.yml",
    "production": WORKFLOWS / "android-accessibility-production.yml",
}


def read(name):
    return FILES[name].read_text(encoding="utf-8")


class AndroidSingleLatestBuildContracts(unittest.TestCase):
    def test_shared_concurrency_across_android_apk_workflows(self):
        for name in FILES:
            with self.subTest(workflow=name):
                body = read(name)
                self.assertIn("group: android-apk-", body)
                self.assertIn("cancel-in-progress: true", body)

    def test_release_trial_is_only_automatic_master_apk(self):
        trial = read("trial")
        candidate = read("candidate")
        production = read("production")
        self.assertIn("push:\n    branches: [master]", trial)
        self.assertIn("workflow_dispatch:", trial)
        candidate_push = candidate.split("  push:\n", 1)[1].split(
            "  pull_request:\n", 1
        )[0]
        self.assertNotIn("      - master\n", candidate_push)
        self.assertIn("  pull_request:\n", candidate)
        self.assertIn("  workflow_dispatch:\n", candidate)
        self.assertNotIn("  push:\n", production)
        self.assertIn("  workflow_dispatch:\n", production)

    def test_four_native_compilers_and_gradle_workers(self):
        for name in FILES:
            with self.subTest(workflow=name):
                body = read(name)
                self.assertIn("-Pj=4", body)
                self.assertIn("--max-workers=4", body)
                self.assertNotIn("-Pj=3", body)

    def test_candidate_baseline_does_not_build_simultaneously(self):
        self.assertIn("max-parallel: 1", read("candidate"))

    def test_trial_only_publishes_current_master_commit(self):
        trial = read("trial")
        self.assertIn("Reject obsolete master source before costly compilation", trial)
        self.assertIn("Recheck master is still latest before publishing APK", trial)
        self.assertGreaterEqual(
            trial.count("git ls-remote origin refs/heads/master"), 2
        )
        self.assertIn('test "$latest" = "$GITHUB_SHA"', trial)


if __name__ == "__main__":
    unittest.main()
