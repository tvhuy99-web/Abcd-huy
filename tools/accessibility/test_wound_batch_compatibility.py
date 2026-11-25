"""Keep upstream treatment counts and old Android save compatibility."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class WoundBatchCompatibility(unittest.TestCase):
    def test_old_save_defaults_to_one_and_new_save_retains_amount(self):
        header = (ROOT / "src/activity_actor_definitions.h").read_text("utf-8")
        actor = (ROOT / "src/activity_actor.cpp").read_text("utf-8")
        definition = header.split("class fix_wound_activity_actor :", 1)[1].split(
            "class gunmod_add_activity_actor", 1)[0]
        self.assertIn("int amount = 1;", definition)
        body = actor.split("void fix_wound_activity_actor::finish(", 1)[1].split(
            "void gunmod_add_activity_actor::start(", 1)[0]
        self.assertIn('jsout.member( "amount", amount );', body)
        self.assertIn('data.read( "amount", actor.amount );', body)

    def test_finish_charges_only_for_existing_wounds(self):
        actor = (ROOT / "src/activity_actor.cpp").read_text("utf-8")
        body = actor.split("void fix_wound_activity_actor::finish(", 1)[1].split(
            "void fix_wound_activity_actor::serialize(", 1)[0]
        self.assertIn("wd.type == mended_wound", body)
        self.assertIn("treated_damage.push_back( wd.get_hp_damage() )", body)
        self.assertIn("is_crafting_component, treated )", body)
        self.assertIn("who.consume_items( e, treated );", body)
        self.assertIn("who.consume_tools( e, treated );", body)
        self.assertIn("fix.mod_hp * treated", body)

    def test_semantic_requirements_show_the_actual_treatment_quantity(self):
        ui = (ROOT / "src/medical_ui.cpp").read_text("utf-8")
        self.assertIn("stored.wd == opt.wd && stored.fix == opt.fix", ui)
        self.assertIn("existing->amount + opt.amount", ui)
        self.assertIn('is_crafting_component, opt.amount );', ui)
        self.assertIn('a11y::append_requirements( requirements, "treatment"', ui)
        self.assertIn('string_format( "%s (x%d)", fix.get_name(), opt.amount )', ui)


if __name__ == "__main__":
    unittest.main()
