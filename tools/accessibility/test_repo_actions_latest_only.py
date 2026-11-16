"""Repository-wide GitHub Actions concurrency and build resource contracts.

Scope: latest wins per CI workflow and PR/branch, not a destructive global
mutex shared between unrelated checks or user event handlers.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / ".github" / "workflows"


def read(name):
    return (WORK / name).read_text(encoding="utf-8")


class RepositoryLatestOnlyActions(unittest.TestCase):
    def test_all_workflows_define_top_level_concurrency(self):
        paths = sorted(WORK.glob("*.yml")) + sorted(WORK.glob("*.yaml"))
        self.assertGreaterEqual(len(paths), 44)
        for path in paths:
            with self.subTest(workflow=path.name):
                text = path.read_text(encoding="utf-8")
                self.assertRegex(text, r"(?m)^concurrency:\s*\n")
                self.assertRegex(text, r"(?m)^  group:\s*\S+")
                self.assertRegex(text, r"(?m)^  cancel-in-progress:\s*(true|false)\s*$")

    def test_master_build_matrix_cancels_superseded_commit(self):
        matrix = read("matrix.yml")
        self.assertIn("group: general-build-", matrix)
        self.assertIn("cancel-in-progress: true", matrix)
        self.assertNotIn(
            "cancel-in-progress: ${{ github.event_name == 'pull_request' }}",
            matrix
        )

    def test_pr_and_master_checks_do_not_cancel_other_workflows(self):
        for name in ("CBA.yml", "clang-tidy.yml", "iwyu.yml",
                     "codeql-analysis.yml", "matrix.yml", "astyle.yml",
                     "flake8.yml", "json.yml", "pr-validator.yml",
                     "gcc16-vehicle-spawn.yml", "accessibility-core.yml"):
            with self.subTest(workflow=name):
                text = read(name)
                self.assertIn("cancel-in-progress: true", text)
                group = re.search(r"(?m)^  group:\s*(.+)$", text)
                self.assertIsNotNone(group)
                self.assertNotEqual(group.group(1), "global")
                self.assertNotIn("android-apk-", group.group(1))

    def test_single_latest_android_apk_and_four_native_workers(self):
        paths = ("android-accessibility.yml",
                 "android-accessibility-release-trial.yml",
                 "android-accessibility-production.yml")
        for name in paths:
            with self.subTest(workflow=name):
                body = read(name)
                self.assertIn("group: android-apk-", body)
                self.assertIn("cancel-in-progress: true", body)
                self.assertIn("-Pj=4", body)
                self.assertIn("--max-workers=4", body)
        self.assertIn("push:\n    branches: [master]",
                      read("android-accessibility-release-trial.yml"))
        self.assertNotIn("      - master\n", read("android-accessibility.yml")
                         .split("  push:", 1)[1].split("  pull_request:", 1)[0])
        self.assertNotIn("  push:", read("android-accessibility-production.yml"))

    def test_comment_commands_are_not_superseded_by_other_comments(self):
        for name in ("comment-commands.yml", "greet-first-time-comment.yml"):
            with self.subTest(workflow=name):
                body = read(name)
                self.assertIn("github.event.comment.id", body)
                self.assertIn("cancel-in-progress: false", body)

    def test_post_processing_is_scoped_by_upstream_run_not_branch(self):
        for name in ("post-spell-check-result.yml", "post-suggestions.yml",
                     "test_labeler.yml", "gh-pages-rebuild.yml"):
            with self.subTest(workflow=name):
                body = read(name)
                self.assertIn("github.event.workflow_run.id", body)
                self.assertIn("cancel-in-progress: false", body)

    def test_heavy_build_matrices_have_four_runner_slots(self):
        self.assertIn("max-parallel: 4", read("release.yml"))
        self.assertIn("max-parallel: 4", read("compose-tilesets.yml"))
        matrix = read("matrix.yml")
        self.assertIn('&& echo 4 || echo 1)', matrix)
        self.assertIn("max-parallel: ${{ fromJSON(needs.matrix-variables.outputs.max_parallel) }}",
                      matrix)

    def test_manual_publish_and_maintenance_are_not_preempted(self):
        for name in ("release-android-bundle.yaml", "stale.yml",
                     "build-translations.yml", "compose-tilesets.yml",
                     "weekly-changelog.yml"):
            with self.subTest(workflow=name):
                self.assertIn("cancel-in-progress: false", read(name))

    def test_cleanup_is_manual_non_destructive_and_doesnt_start_second_apk(self):
        body = read("actions-queue-reset.yml")
        self.assertIn("  workflow_dispatch:", body)
        self.assertNotIn("  push:", body)
        self.assertIn("actions: write", body)
        self.assertIn("['push', 'pull_request']", body)
        self.assertIn("cancelWorkflowRun", body)
        self.assertNotIn("createWorkflowDispatch", body)
        self.assertIn("latest.get(item.key)", body)

    def test_individual_projects_keep_branch_local_parallelism(self):
        for name in ("reader-audit-build.yml", "format_emscripten.yml",
                     "audit-vietnamese.yml", "export-vietnamese-review.yml"):
            with self.subTest(workflow=name):
                body = read(name)
                self.assertIn("cancel-in-progress: true", body)
                self.assertIn("github.ref", body)


if __name__ == "__main__":
    unittest.main()
