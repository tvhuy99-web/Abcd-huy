"""Guard upstream-pinned Actions without discarding fork concurrency controls."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / ".github" / "workflows"


class UpstreamWorkflowActionPins(unittest.TestCase):
    def test_upstream_actions_and_fork_concurrency(self):
        expected = {
            "cmake-format.yml": ("actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1",),
            "linter.yml": ("actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1", "actions/upload-artifact@cf430e030ddbb5b0abf93d22962f4752f3646cd9"),
            "post-suggestions.yml": ("actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1", "dawidd6/action-download-artifact@a92c7ae74cf1df8810783758581e485ac3aaa309"),
            "text-changes-analyzer.yml": ("actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1", "actions/upload-artifact@cf430e030ddbb5b0abf93d22962f4752f3646cd9"),
        }
        for name, actions in expected.items():
            with self.subTest(workflow=name):
                body = (WORK / name).read_text(encoding="utf-8")
                for action in actions:
                    self.assertIn("uses: " + action, body)
                self.assertIn("concurrency:", body)
                self.assertIn("cancel-in-progress:", body)
        post = (WORK / "post-suggestions.yml").read_text(encoding="utf-8")
        self.assertIn("github.event.workflow_run.id", post)
        self.assertIn("cancel-in-progress: false", post)


if __name__ == "__main__":
    unittest.main()
