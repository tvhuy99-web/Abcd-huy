"""Prevent savegame regressions when merging harvest activity actors."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class HarvestActivitySaveCompatibility(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        header = (ROOT / "src/activity_actor_definitions.h").read_text("utf-8")
        cpp = (ROOT / "src/activity_actor.cpp").read_text("utf-8")
        cls.header = header.split("class harvest_activity_actor :", 1)[1].split(
            "class plant_seed_activity_actor :", 1)[0]
        cls.actor = cpp.split("void harvest_activity_actor::start(", 1)[1].split(
            "void plant_seed_activity_actor::start(", 1)[0]
        cls.examine = (ROOT / "src/iexamine.cpp").read_text("utf-8")

    def test_harvest_target_is_absolute_across_map_rebases(self):
        self.assertIn("const tripoint_abs_ms &target", self.header)
        self.assertIn("tripoint_abs_ms target;", self.header)
        self.assertNotIn("tripoint_bub_ms target;", self.header)
        self.assertGreaterEqual(
            self.actor.count("const tripoint_bub_ms pos = here.get_bub( target );"),
            2,
        )
        self.assertGreaterEqual(
            self.examine.count(
                "harvest_activity_actor( get_map().get_abs( examp ), auto_forage )"
            ),
            5,
        )

    def test_new_saves_store_absolute_target(self):
        self.assertIn('jsout.member( "target_abs", target );', self.actor)
        self.assertNotIn('jsout.member( "target", target );', self.actor)

    def test_old_saves_migrate_bubble_target_to_absolute(self):
        self.assertIn('if( jsobj.has_member( "target_abs" ) )', self.actor)
        self.assertIn('jsobj.read( "target_abs", actor.target );', self.actor)
        self.assertIn("tripoint_bub_ms legacy_target", self.actor)
        self.assertIn('jsobj.read( "target", legacy_target )', self.actor)
        self.assertIn(
            "actor.target = get_map().get_abs( legacy_target );", self.actor
        )

    def test_harvest_activity_stays_registered_for_saved_actors(self):
        self.assertIn('ACT_HARVEST( "ACT_HARVEST" )', self.header)
        self.assertIn(
            "harvest_activity_actor::deserialize", self.actor
        )


if __name__ == "__main__":
    unittest.main()
