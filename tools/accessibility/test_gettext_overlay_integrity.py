"""Detect gettext overlay collisions before expensive Android APK builds.

Gettext keys include msgctxt, and can span several quoted lines.  The
overlay is merged with the full compressed Vietnamese catalog by msgcat.
"""

import ast
import re
import unittest
from pathlib import Path


CATALOG = (Path(__file__).resolve().parents[2]
           / "lang/po/accessibility/vi.po")


class GettextOverlayIntegrity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = CATALOG.read_text(encoding="utf-8")

    def test_no_duplicate_gettext_identities_in_overlay(self):
        seen = set()
        # Parse msgid/msgctxt with continuation lines; identical English
        # text in different contexts is a legal, distinct gettext identity.
        for paragraph in re.split(r"\n[ \t]*\n", self.text):
            values = {}
            field = None
            for line in paragraph.splitlines():
                if line.startswith("#"):
                    continue
                match = re.match(
                    r'^(msgctxt|msgid|msgid_plural|msgstr(?:\[\d+\])?)\s+(".*")$',
                    line,
                )
                if match:
                    field = match.group(1)
                    values[field] = ast.literal_eval(match.group(2))
                elif line.startswith('"') and field is not None:
                    values[field] += ast.literal_eval(line)
                else:
                    field = None
            if not values.get("msgid"):
                continue
            identity = (values.get("msgctxt"), values["msgid"])
            self.assertNotIn(identity, seen, f"Duplicate PO entry: {identity}")
            seen.add(identity)
        self.assertGreater(len(seen), 500)

    def test_upstream_plural_none_is_not_overridden_as_singular(self):
        # Upstream vi.po.gz already includes 'none' as a plural entry.
        # A singular-only overlay entry makes msgcat fail even with
        # --use-first; leave that identity to the base catalog.
        self.assertNotRegex(
            self.text, r'(?m)^msgid "none"$'
        )


if __name__ == "__main__":
    unittest.main()
