"""Contracts for upstream JSON loading, faction defaults, and save compatibility."""

from pathlib import Path
import json
import unittest

ROOT = Path(__file__).resolve().parents[2]


def text(path):
    return (ROOT / path).read_text(encoding="utf-8")


class NativeJsonAuditTests(unittest.TestCase):
    def test_faction_falls_back_to_npc_legacy_relationship(self):
        faction = text("src/faction.cpp")
        self.assertIn('optional( jsobj, false, "mon_faction", mon_faction, mfaction_str_id::NULL_ID() );', faction)
        npc = text("src/npc.cpp").split("mfaction_id npc::get_monster_faction() const", 1)[1]
        self.assertLess(npc.index("my_fac->mon_faction.is_valid()"), npc.index("is_player_ally()"))

    def test_camp_meal_reads_upstream_and_old_android_fields(self):
        load = text("src/activity_actor.cpp").split(
            "camp_meal_activity_actor::deserialize(", 1)[1].split("return actor.clone();", 1)[0]
        for upstream, old, field in (("camp", "camp_position", "camp_position"),
                                      ("kcal", "meal_kcals", "meal_kcals")):
            self.assertLess(load.index(f'data.read( "{upstream}", actor.{field} )'),
                            load.index(f'data.read( "{old}", actor.{field} )'))

    def test_fault_requirements_follow_generic_factory_inheritance(self):
        source = text("src/fault.cpp")
        self.assertNotIn("reqs_temp_storage", source)
        load = source.split("void fault_fix::load(", 1)[1].split("void fault_fix::finalize()", 1)[0]
        self.assertIn('if( jo.has_array( "requirements" ) ) {', load)
        self.assertIn("pending_requirements.clear();", load)
        self.assertIn("pending_requirements.emplace_back(", load)
        finalize = source.split("void fault_fix::finalize()", 1)[1].split(
            "void fault_fix::check()", 1)[0]
        self.assertIn("for( const std::pair<requirement_id, int> &p : pending_requirements )", finalize)
        self.assertIn("pending_requirements.clear();", finalize)

    def test_native_data_check_returns_failure_if_loader_throws(self):
        body = text("src/game_io.cpp").split("bool game::check_mod_data(", 1)[1].split(
            "bool game::is_core_data_loaded()", 1)[0]
        self.assertEqual(body.count("load_ok = false;"), 2)
        self.assertIn("return load_ok;", body)
        self.assertIn('get_option( "SKIP_VERIFICATION" ).setValue( "false" )', body)

    def test_native_check_uses_gameplay_mod_loading_and_logs_errors(self):
        body = text("src/game_io.cpp").split("bool game::check_mod_data(", 1)[1].split(
            "bool game::is_core_data_loaded()", 1)[0]
        self.assertIn("append_dependencies( dependency )", body)
        self.assertIn("test_world->active_mod_order", body)
        self.assertIn("load_world_modfiles();", body)
        self.assertNotIn("load_data_from_dir( mod.path", body)
        main = text("src/main.cpp")
        self.assertIn("initialize_debugging( cli.check_mods )", main)
        self.assertIn("Native validation: load_ok=", main)

    def test_headless_options_initialized_once_and_exit_keeps_diagnostics(self):
        main = text("src/main.cpp")
        body = main.split("bool initialize_interface(", 1)[1].split(
            "void load_static_game_data(", 1)[0]
        self.assertIn("#if !defined(TILES)", body)
        self.assertIn("#if defined(TILES)\n    else if( cli.check_mods )", body)
        self.assertIn("Json::globally_report_unvisited_members( false );\n            exit( loaded", main)

    def test_new_ammo_slot_can_extend_effects_but_still_requires_ammo_type(self):
        factory = text("src/item_factory.cpp").split("void islot_ammo::deserialize(", 1)[1].split(
            "void islot_engine::deserialize(", 1)[0]
        self.assertIn('mandatory( jo, was_loaded, "ammo_type", type )', factory)
        self.assertIn('was_loaded || jo.has_string( "copy-from" ), "effects"', factory)

    def test_vehicle_fuel_is_an_actual_ammo_consumer(self):
        ammo = text("src/ammo.cpp")
        self.assertIn("vehicles::parts::get_all()", ammo)
        self.assertIn("part.fuel_type == fuel->get_id()", ammo)
        self.assertIn("!used_as_vehicle_fuel", ammo)

    def test_complex_recipe_warning_does_not_interrupt_world_loading(self):
        source = text("src/requirements.cpp")
        body = source.split("deduped_requirement_data::deduped_requirement_data(", 1)[1].split(
            "bool deduped_requirement_data::can_make_with_inventory(", 1)[0]
        limit = body.split("if( alternatives_.size() + pending.size() > max_alternatives )", 1)[1]
        limit = limit.split("return;", 1)[0]
        self.assertIn("DebugLog( D_WARNING, D_GAME )", limit)
        self.assertNotIn("debugmsg(", limit)
        self.assertIn("context.str()", limit)
        self.assertIn("alternatives_.size() + pending.size()", limit)
        self.assertIn("is_too_complex_ = true;", limit)
        self.assertIn("alternatives_ = { in };", limit)
        self.assertNotIn("%1s", limit)
        self.assertNotIn("%2s", limit)

    def test_ci_runs_real_data_loader_after_link_in_clean_userdir(self):
        workflow = text(".github/workflows/upstream-merge-full-link.yml")
        self.assertIn("'data/**'", workflow)
        self.assertIn("set -euo pipefail", workflow)
        self.assertIn('mktemp -d', workflow)
        self.assertLess(workflow.index("make -k -j3 all"),
                        workflow.index("--check-mods dda dda_tutorial"))
        tutorial = json.loads(text("data/mods/dda_tutorial/modinfo.json"))[0]
        self.assertIn("dda", tutorial["dependencies"])


    def test_special_placement_does_not_recreate_an_existing_camp(self):
        for path in ("src/overmap_special_fixed.cpp",
                     "src/overmap_special_mutable.cpp"):
            source = text(path)
            guard = "if( !om.find_camp( camp_loc.xy() ) ) {"
            creation = 'get_map().add_camp( camp_loc, "faction_camp", false );'
            self.assertEqual(source.count(guard), 1, path)
            self.assertEqual(source.count(creation), 1, path)
            start = source.index(guard)
            self.assertLess(start, source.index(creation), path)
            # The ownership, name and expansion setup must remain protected.
            block = source[start:source.index("temp_camp->define_camp(", start)]
            self.assertIn("temp_camp->set_owner(", block, path)
            self.assertIn("temp_camp->set_name(", block, path)

    def test_saved_and_legacy_camps_are_not_registered_twice(self):
        source = text("src/savegame.cpp")
        saved = source.split('name == "camps"', 1)[1].split(
            'name == "overmap_special_placements"', 1)[0]
        self.assertIn("if( !find_camp( camp_pos ) )", saved)
        self.assertIn("add_camp( camp_pos, new_camp );", saved)
        self.assertIn("Skipping duplicate saved basecamp", saved)
        game = text("src/game.cpp")
        legacy = game.split("void game::validate_camps()", 1)[1].split(
            "std::set<character_id> game::get_follower_list()", 1)[0]
        self.assertEqual(legacy.count(
            "if( !overmap_buffer.find_camp( camp.camp_omt_pos().xy() ) )"), 2)
        self.assertEqual(legacy.count("overmap_buffer.add_camp( camp );"), 2)

    def test_isherwood_stables_are_a_named_mutable_special_camp(self):
        data = json.loads(text("data/json/overmap/overmap_mutable/farm_isherwood_mutable.json"))
        special = next(obj for obj in data if obj.get("id") == "Isherwood Farm Mutable")
        stable = special["overmaps"]["farm_isherwood_6_north"]
        self.assertEqual(stable["camp"], "isherwood_family")
        self.assertEqual(stable["camp_name"], "Isherwood Stables")


if __name__ == "__main__":
    unittest.main()
