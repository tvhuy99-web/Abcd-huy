"""Guard the upstream FAULT JSON schema against omissions during integration."""

import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class FaultJsonLoaderTests(unittest.TestCase):
    def test_attack_cost_modifier_is_read_with_upstream_default(self):
        source = (ROOT / "src/fault.cpp").read_text(encoding="utf-8")
        loader = source.split("void fault::load(", 1)[1].split("void fault::check()", 1)[0]
        self.assertIn(
            'optional( jo, was_loaded, "attack_cost_modifier", attack_cost_modifier_, 1.0 );',
            loader,
        )
        data = json.loads((ROOT / "data/json/faults/faults_melee.json").read_text(
            encoding="utf-8"))
        modified = [entry for entry in data if "attack_cost_modifier" in entry]
        self.assertTrue(modified, "Upstream melee fault data must exercise this field")
        self.assertTrue(any(entry["attack_cost_modifier"] == 1.2 for entry in modified))


if __name__ == "__main__":
    unittest.main()
