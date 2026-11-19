#pragma once
#ifndef CATA_TESTS_A11Y_FIXTURES_H
#define CATA_TESTS_A11Y_FIXTURES_H

#include <string>
#include <vector>

#include "coordinates.h"
#include "point.h"
#include "type_id.h"

class basecamp;
class computer;
class vehicle;




















namespace a11y_fixture
{






void reset_world_for_fixture();





















enum class knowledge_variant { a, b };

struct knowledge_pair_fixture {
    knowledge_variant variant = knowledge_variant::a;
    tripoint_bub_ms avatar_pos;
    tripoint_bub_ms monster_pos;
    tripoint_bub_ms hidden_trap_pos;
    tripoint_bub_ms mine_pos;
    tripoint_bub_ms crate_pos;
    itype_id crate_content;
};

knowledge_pair_fixture build_knowledge_pair( knowledge_variant variant );












struct pocket_tree_fixture {
    itype_id outer_container;
    itype_id inner_container;
    itype_id sealed_container;
    itype_id sealed_content;
    itype_id loose_content;
};

pocket_tree_fixture build_pocket_tree();












struct two_sided_inventory_fixture {
    tripoint_bub_ms ground_pos;
    tripoint_bub_ms vehicle_pos;
    itype_id ground_item;
    itype_id cargo_item;
    itype_id carried_item;
    itype_id worn_item;
};

two_sided_inventory_fixture build_two_sided_inventory();










struct wound_fixture {
    bodypart_id injured_part;
    int hp_before = 0;
    int hp_max = 0;
    itype_id bandage_item;
    itype_id disinfectant_item;
};

wound_fixture build_wound_fixture();



















struct combat_field_fixture {
    tripoint_bub_ms avatar_pos;
    tripoint_bub_ms visible_target_pos;
    tripoint_bub_ms empty_pos;
    tripoint_bub_ms ally_pos;
    tripoint_bub_ms other_level_pos;
    spell_id aoe_spell;
};

combat_field_fixture build_combat_field();

















struct vehicle_fixture {
    vehicle *veh = nullptr;
    tripoint_bub_ms origin;
    tripoint_bub_ms cargo_pos;
    std::string broken_part_name;
    itype_id fuel_type;
    int fuel_percent = 50;
};

vehicle_fixture build_vehicle_fixture();




















struct zones_rules_fixture {
    tripoint_abs_ms food_zone_start;
    tripoint_abs_ms food_zone_end;
    tripoint_abs_ms unsorted_zone_pos;
    std::string safemode_rule;
    itype_id autopickup_item;
};

zones_rules_fixture build_zones_and_rules();




















struct camp_fixture {
    basecamp *camp = nullptr;
    tripoint_abs_omt camp_omt;
    tripoint_bub_ms resident_pos;
    int food_kcal = 100;
};

camp_fixture build_faction_camp();
void clear_faction_camp( const camp_fixture &fixture );

















struct abilities_fixture {
    tripoint_bub_ms computer_pos;
    computer *terminal = nullptr;
    bionic_id installed_bionic;
    itype_id uninstalled_cbm;
    spell_id known_spell;
    itype_id spellbook_item;
    std::string spellbook_spell;
};

abilities_fixture build_abilities();

















struct lifecycle_stage_fixture {
    int stage = 0;
    tripoint_bub_ms known_marker_pos;
    std::string known_terrain_id;
    spell_id known_spell;
};

lifecycle_stage_fixture build_lifecycle_stage( int stage );




















struct minesweeper_spec {
    int width = 8;
    int height = 8;
    int mine_count = 10;
    unsigned int seed = 0;
    std::vector<point> mines;
};

minesweeper_spec minesweeper_beginner_spec( unsigned int seed = 2201 );












struct lightson_spec {
    point size;
    unsigned int seed = 0;
    std::vector<point> change_coords;
    std::vector<point> dark_cells;
};

lightson_spec lightson_spec_for_seed( unsigned int seed = 2202 );







struct sokoban_spec {
    int level_index = 0;
    point size;
    point player;
    std::vector<point> packages;
    std::vector<point> goals;
    std::vector<std::string> rows;
};

sokoban_spec sokoban_level_one_spec();











struct kitten_spec {
    int width = 60;
    int height = 20;
    int object_count = 21;
    int bogus_count = 20;
    unsigned int seed = 0;
    bool positions_derivable = false;
};

kitten_spec kitten_spec_for_seed( unsigned int seed = 2203 );











struct snake_spec {
    point board_size;
    point head;
    point fruit;
    point direction = point::east;
    int starting_length = 10;
    unsigned int seed = 0;
};

snake_spec snake_spec_for_board( int width, int height, unsigned int seed = 2204 );

}

#endif
