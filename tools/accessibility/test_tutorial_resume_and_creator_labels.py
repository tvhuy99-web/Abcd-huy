"""Regression checks for missing character-creator names and tutorial save recovery."""

import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(name):
    return (ROOT / name).read_text(encoding="utf-8")


class TutorialResumeAndCreatorLabels(unittest.TestCase):
    def test_creator_preserves_names_for_semantic_reader(self):
        source = read("src/newcharacter.cpp")
        helper = source.split("static uilist_entry get_uilist_entry(", 1)[1].split(
            "static void append_screen_reader_active", 1)[0]
        self.assertIn('get_option<bool>( "SCREEN_READER_MODE" ) && !a11y::enabled()', helper)
        self.assertIn('uilist_entry cc_entry( legacy_reader_mode ? std::string() : name );', helper)
        self.assertNotIn('screen_reader_mode ? std::string() : name', helper)
        self.assertIn('node.label = remove_color_tags( entry.txt );', read("src/uilist.cpp"))
        entries = source.split("void character_creator_ui::update_uilist_entries()", 1)[1]
        for step in ("SCENARIO", "PROFESSION", "BACKGROUND", "STATS", "TRAITS", "SKILLS"):
            with self.subTest(step=step):
                # Backgrounds use the upstream HOBBY or BACKGROUND tab name.
                name = "CHARCREATOR_" + step
                if step == "HOBBY" and name not in entries:
                    name = "CHARCREATOR_BACKGROUND"
                section = entries.split("case " + name, 1)[1]
                self.assertIn("get_uilist_entry(", section.split("case CHARCREATOR_", 1)[0])

    def test_new_tutorial_persists_required_mods_before_save(self):
        source = read("src/worldfactory.cpp")
        special = source.split("WORLD *worldfactory::make_new_world( special_game_type", 1)[1]
        special = special.split("void worldfactory::set_active_world", 1)[0]
        required = 'active_mod_order = { mod_id( "dda" ), mod_id( "dda_tutorial" ) }'
        self.assertLess(special.index(required), special.index("special_world->save()"))

    def test_legacy_load_restores_required_mods_without_writing_save(self):
        source = read("src/mod_manager.cpp")
        load = source.split("void mod_manager::load_mods_list(", 1)[1].split(
            "void mod_manager::check_mods_list(", 1)[0]
        self.assertIn('world->world_name == "TUTORIAL"', load)
        self.assertIn('active_mod_order = { mod_id( "dda" ), mod_id( "dda_tutorial" ) }', load)
        self.assertNotIn("save_mods_list(", load)
        check = source.split("void mod_manager::check_mods_list(", 1)[1].split(
            "const mod_manager::t_mod_list", 1)[0]
        self.assertIn("if( !required.is_valid() )", check)
        self.assertIn("throw std::runtime_error", check)
        save = read("src/game_io.cpp").split("bool game::save()", 1)[1]
        self.assertIn("if( gametype() == special_game_type::TUTORIAL )", save)
        self.assertIn("save_mods_list( world_generator->active_world )", save)

    def test_resume_restores_mode_before_content_load_and_does_not_reinitialize(self):
        setup = read("src/game.cpp").split("void game::setup( bool loading_saved_character )", 1)[1]
        setup = setup.split("bool game::has_gametype()", 1)[0]
        self.assertIn("if( loading_saved_character )", setup)
        self.assertIn('world_generator->active_world->world_name == "TUTORIAL"', setup)
        self.assertLess(setup.index("get_special_game( special_game_type::TUTORIAL )"),
                        setup.index("load_world_modfiles();"))
        self.assertNotIn("gamemode->init()", setup)
        menu = read("src/main_menu.cpp").split("bool main_menu::load_game(", 1)[1]
        self.assertIn("g->setup( true );", menu)
        self.assertIn("cleanup.cancel();", menu)

    def test_progress_uses_serialized_avatar_values(self):
        source = read("src/gamemode_tutorial.cpp")
        self.assertIn('"A11Y_TUTORIAL_SEEN_" + io::enum_to_string<tut_lesson>( lesson )', source)
        self.assertIn('get_value( tutorial_seen_key( lesson ) ).str() == "1"', source)
        self.assertIn('set_value( tutorial_seen_key( lesson ), std::string( "1" ) )', source)
        self.assertNotIn("tutorials_seen[", source)
        init = source.split("bool tutorial_game::init()", 1)[1].split(
            "void tutorial_game::per_turn()", 1)[0]
        self.assertIn("player_character.remove_value( tutorial_seen_key(", init)
        self.assertIn("set_scenario( scenario::generic() );", init)
        message = source.split("void tutorial_game::add_message(", 1)[1]
        self.assertLess(message.index("translated_lesson.empty()"),
                        message.index("set_value( tutorial_seen_key("))
        self.assertIn("last_lesson_key, translated_lesson", message)
        serializer = read("src/savegame_json.cpp")
        self.assertIn('jsout.member( "values", values );', serializer)
        self.assertIn('jsin.read( "values", values );', serializer)
        self.assertIn('g->gametype() == special_game_type::TUTORIAL', serializer)
        self.assertIn('scen_ident == "(null)" || scen_ident.empty()', serializer)
        self.assertIn("if( !legacy_tutorial )", serializer)

    def test_required_content_ids_have_definitions(self):
        entries = []
        for path in (ROOT / "data/mods/dda_tutorial").rglob("*.json"):
            value = json.loads(path.read_text(encoding="utf-8"))
            if isinstance(value, list):
                entries.extend(x for x in value if isinstance(x, dict))
        ids = {x.get("id") for x in entries if isinstance(x.get("id"), str)}
        for key in ("dda_tutorial", "mon_dummy_tutorial", "tr_tutorial_10"):
            self.assertIn(key, ids)
        self.assertTrue(any("tutorial_underground" in str(x.get("id", ""))
                            for x in entries if x.get("type") == "overmap_terrain"))


if __name__ == "__main__":
    unittest.main()
