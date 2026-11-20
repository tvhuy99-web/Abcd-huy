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
        self.assertIn("push:\n    branches: [master, test/upstream-full-merge-2026-11-19]", trial)
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

    def test_candidate_retries_only_transient_gradle_maven_throttling(self):
        candidate = read("candidate")
        build = candidate.split(
            "- name: Build installable debug APK", 1)[1].split(
                "- name: Verify APK contents and checksums", 1)[0]
        self.assertIn("for attempt in 1 2 3; do", build)
        self.assertIn("if bash ./gradlew", build)
        self.assertIn("Received status code 429", build)
        self.assertIn("Too Many Requests", build)
        self.assertIn("Could not resolve all artifacts for configuration", build)
        self.assertIn("grep -Fc 'Could not find '", build)
        self.assertIn("[ \"$(grep -Fc", build)
        self.assertIn("-ge 5", build)
        self.assertIn("--refresh-dependencies", build)
        self.assertIn("transient_dependency_error", build)
        self.assertIn("sleep $((attempt * 45))", build)
        self.assertIn("exit 1", build)
        self.assertIn("tee ../build-evidence/gradle.log", build)

    def test_candidate_verifies_combat_gettext_in_installable_apk(self):
        verification = read("candidate").split(
            "- name: Verify APK contents and checksums", 1)[1].split(
                "- name: Seed exact native cache", 1)[0]
        self.assertIn("gettext.GNUTranslations", verification)
        self.assertIn("archive.read('assets/lang/mo/vi/LC_MESSAGES/cataclysm-dda.mo')",
                      verification)
        self.assertIn("'You block %s.'", verification)
        self.assertIn("'You parry %1$s of the attack with your %2$s!'", verification)
        self.assertIn("catalog.gettext(message) == message", verification)

    def test_candidate_baseline_does_not_build_simultaneously(self):
        self.assertIn("max-parallel: 1", read("candidate"))

    def test_trial_release_integrates_without_publishing_test_branch(self):
        trial = read("trial")
        self.assertIn("Reject obsolete master source before costly compilation", trial)
        self.assertIn("Recheck master is still latest before publishing APK", trial)
        self.assertGreaterEqual(
            trial.count("git ls-remote origin refs/heads/master"), 2
        )
        self.assertIn('test "$latest" = "$GITHUB_SHA"', trial)
        self.assertIn("if: success() && github.ref == 'refs/heads/master'", trial)
        self.assertIn('branches: [master, test/upstream-full-merge-2026-11-19]', trial)


if __name__ == "__main__":
    unittest.main()
