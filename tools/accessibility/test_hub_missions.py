"""Regression checks for missions, contacts, diary and map."""

import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "src"


class MissionsHubTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.missions = (SRC / "mission_ui.cpp").read_text("utf-8")
        cls.diary = (SRC / "diary_ui.cpp").read_text("utf-8")
        cls.factions = (SRC / "faction_ui.cpp").read_text("utf-8")
        cls.talk = (SRC / "npctalk.cpp").read_text("utf-8")
        cls.map = (SRC / "overmap_ui.cpp").read_text("utf-8")

    def test_objectives_set_only_on_activation(self):
        self.assertIn('intent.action == "activate" && selected_tab ==',
                      self.missions)
        self.assertIn("set_active_mission( *missions[i] );", self.missions)
        self.assertIn("set_active_point_of_interest( points[i] );",
                      self.missions)
        self.assertIn("Travel has not started.", self.missions)
        self.assertIn("direction_name( direction_from(", self.missions)

    def test_diary_creation_and_safe_deletion(self):
        self.assertIn("New diary page created.", self.diary)
        self.assertIn('status( "text_applied", &intent )', self.diary)
        self.assertIn('query_yn( _( "Really delete Page?" ) )', self.diary)

    def test_followers_can_be_contacted_without_accidental_recall(self):
        self.assertIn("Within interaction range. Activate to talk.",
                      self.factions)
        self.assertIn("Within radio range. Activate to talk.",
                      self.factions)
        self.assertIn("Use Contact to recall.", self.factions)
        self.assertIn('return "CONFIRM";', self.factions)
        self.assertIn("applicable_repres.size() == 1", self.factions)

    def test_npc_picker_reads_relative_position(self):
        self.assertIn('"%1$s, %2$s, %3$d tiles away"', self.talk)
        self.assertIn("direction_name( direction_from(", self.talk)
        self.assertIn("if( npc_count == 1 )", self.talk)
        self.assertIn("dialogue_consequence::hostile", self.talk)

    def test_failed_route_errors_are_nonmodal(self):
        self.assertIn("No route was set: the destination tile is unexplored.",
                      self.map)
        self.assertIn('add_msg( m_info, "%s", reason );', self.map)
        self.assertIn("popup( reason );", self.map)
        self.assertIn("if( query_yn( confirm_msg ) )", self.map)


if __name__ == "__main__":
    unittest.main()
