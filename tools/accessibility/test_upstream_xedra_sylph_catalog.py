"""Keep synchronized upstream Xedra Sylph effects and spell migrations."""

import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / "data" / "mods" / "Xedra_Evolved"
FILES = (
    "effects/effects.json", "jmath.json",
    "mutations/paraclesians/sylph_eocs.json",
    "mutations/paraclesians/sylph_mutation_spells.json",
    "mutations/paraclesians/sylph_mutations.json",
    "mutations/paraclesians/sylph_spell_learning_eocs.json",
    "obsoletion_and_migration/spells.json",
    "obsoletion_and_migration/tool_to_obsolete_spells.json",
)


def load(path):
    return json.loads((BASE / path).read_text(encoding="utf-8"))


class UpstreamXedraSylphCatalog(unittest.TestCase):
    def test_coordinated_files_remain_valid(self):
        for filename in FILES:
            with self.subTest(filename=filename):
                records = load(filename)
                self.assertIsInstance(records, list)
                self.assertGreater(len(records), 0)
                self.assertTrue(all(isinstance(r, dict) and r.get("type") for r in records))

    def test_sylph_and_save_migration_triggers_exist(self):
        eocs = load("mutations/paraclesians/sylph_eocs.json")
        mutations = load("mutations/paraclesians/sylph_mutations.json")
        migration = load("obsoletion_and_migration/tool_to_obsolete_spells.json")
        self.assertIn("EOC_SYLPH_LIGHTNING_MELEE_ATTACKS_ON", {r.get("id") for r in eocs})
        self.assertIn("SYLPH_SKIN_1", {r.get("id") for r in mutations})
        self.assertIn("EOC_MIGRATE_SPELL", {r.get("id") for r in migration})


if __name__ == "__main__":
    unittest.main()
