#include "a11y_fixtures.h"

#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "auto_pickup.h"
#include "avatar.h"
#include "basecamp.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "clzones.h"
#include "computer.h"
#include "coordinates.h"
#include "creature.h"
#include "enums.h"
#include "faction.h"
#include "game.h"
#include "item.h"
#include "item_pocket.h"
#include "magic.h"
#include "map.h"
#include "map_memory.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "map_scale_constants.h"
#include "mission.h"
#include "npc.h"
#include "overmapbuffer.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "rng.h"
#include "safemode_ui.h"
#include "stomach.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"

// See a11y_fixtures.h for the guarantee each builder makes and the oracle it
// serves. Written during the coding phase (phase 1): NOT compiled, NOT run.

namespace
{

const tripoint_bub_ms fixture_origin( 60, 60, 0 );

const itype_id itype_bandages( "bandages" );
const itype_id itype_debug_backpack( "debug_backpack" );
const itype_id itype_backpack( "backpack" );
const itype_id itype_disinfectant( "disinfectant" );
const itype_id itype_gasoline( "gasoline" );
const itype_id itype_jeans( "jeans" );
const itype_id itype_knife_combat( "knife_combat" );
const itype_id itype_test_can_drink( "test_can_drink" );
const itype_id itype_test_rock( "test_rock" );
const itype_id itype_test_textbook_fabrication( "test_textbook_fabrication" );
const itype_id itype_water( "water" );

const spell_id spell_test_spell_lava( "test_spell_lava" );
const spell_id spell_test_spell_pew( "test_spell_pew" );

const bionic_id bio_power_storage( "bio_power_storage" );

const vproto_id vehicle_prototype_car( "car" );
const vproto_id vehicle_prototype_test_shopping_cart( "test_shopping_cart" );

} // namespace

namespace a11y_fixture
{

// clear_map_memory() invalidates every memory submap, and the
// memorize_* API silently drops writes to invalid submaps. In real
// play the region around the avatar is prepared as the map loads
// (map::load / shift); fixtures must redo the same preparation after
// every clear, over the whole area any fixture memorizes into.
static void prepare_fixture_memory( avatar &who, map &here )
{
    who.prepare_map_memory_region(
        here.get_abs( tripoint_bub_ms( fixture_origin.x() - 30, fixture_origin.y() - 30, 0 ) ),
        here.get_abs( tripoint_bub_ms( fixture_origin.x() + 30, fixture_origin.y() + 30, 0 ) ) );
}

void reset_world_for_fixture()
{
    clear_avatar();
    clear_map_without_vision();
    clear_creatures();
    clear_npcs();
    clear_vehicles();
    clear_zones();
    set_time_to_day();
    avatar &who = get_avatar();
    who.setpos( get_map(), fixture_origin );
    who.clear_map_memory();
    prepare_fixture_memory( who, get_map() );
}

// ------------------------------------------------------------------ F-01 --
knowledge_pair_fixture build_knowledge_pair( const knowledge_variant variant )
{
    reset_world_for_fixture();
    map &here = get_map();
    avatar &who = get_avatar();

    knowledge_pair_fixture result;
    result.variant = variant;
    result.avatar_pos = fixture_origin;
    // The wall column: known, identical in both variants.
    const int wall_x = fixture_origin.x() + 3;
    // Secret positions east of the wall; the two variants swap the trap and
    // mine rows and move the monster, so every secret differs.
    const bool is_a = variant == knowledge_variant::a;
    result.monster_pos = tripoint_bub_ms( fixture_origin.x() + 5,
                                          fixture_origin.y() + ( is_a ? -2 : 2 ), 0 );
    result.hidden_trap_pos = tripoint_bub_ms( fixture_origin.x() + 4,
                             fixture_origin.y() + ( is_a ? -5 : 5 ), 0 );
    result.mine_pos = tripoint_bub_ms( fixture_origin.x() + 6,
                                      fixture_origin.y() + ( is_a ? 5 : -5 ), 0 );
    result.crate_pos = tripoint_bub_ms( fixture_origin.x() + 5, fixture_origin.y(), 0 );
    result.crate_content = is_a ? itype_knife_combat : itype_bandages;

    // Live ground, identical in both variants: flat grass, wall column,
    // closed crate furniture on the crate tile.
    build_test_map( ter_id( "t_grass" ) );
    for( int y = fixture_origin.y() - 5; y <= fixture_origin.y() + 5; ++y ) {
        here.ter_set( tripoint_bub_ms( wall_x, y, 0 ), ter_id( "t_brick_wall" ) );
    }
    here.furn_set( result.crate_pos, furn_id( "f_crate_c" ) );

    // Secrets, live only: monster behind the wall, buried trap, buried
    // landmine, and the crate's content placed on the crate tile (the
    // CONTAINER furniture is what a11y::observe_tile() gates on).
    spawn_test_monster( "mon_zombie", result.monster_pos );
    here.trap_set( result.hidden_trap_pos, trap_id( "tr_beartrap_buried" ) );
    here.trap_set( result.mine_pos, trap_id( "tr_landmine_buried" ) );
    here.add_item_or_charges( result.crate_pos, item( result.crate_content ) );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );
    set_time_to_day();

    // Known side, identical in both variants: memorize the whole rectangle
    // (grass, the wall column as brick wall, the crate tile's decoration)
    // through the avatar's own memory API, after the reset cleared it.
    for( int x = fixture_origin.x() - 5; x <= fixture_origin.x() + 6; ++x ) {
        for( int y = fixture_origin.y() - 5; y <= fixture_origin.y() + 5; ++y ) {
            const tripoint_bub_ms tile( x, y, 0 );
            who.memorize_terrain( here.get_abs( tile ),
                                  x == wall_x ? "t_brick_wall" : "t_grass", 0, 0 );
        }
    }
    who.memorize_decoration( here.get_abs( result.crate_pos ), "f_crate_c", 0, 0 );

    // Fixture invariants the oracle relies on.
    REQUIRE( here.ter( result.crate_pos ) == ter_id( "t_grass" ) );
    REQUIRE( who.has_memory_at( here.get_abs( result.crate_pos ) ) );
    REQUIRE( who.get_memorized_tile( here.get_abs( result.crate_pos ) ).get_dec_id() ==
             "f_crate_c" );
    return result;
}

// ------------------------------------------------------------------ F-02 --
pocket_tree_fixture build_pocket_tree()
{
    reset_world_for_fixture();
    avatar &who = get_avatar();

    pocket_tree_fixture result;
    result.outer_container = itype_debug_backpack;
    result.inner_container = itype_backpack;
    result.sealed_container = itype_test_can_drink;
    result.sealed_content = itype_water;
    result.loose_content = itype_knife_combat;

    // Innermost first, exactly as item_pocket_test.cpp builds a sealed can:
    // fill the sealable pocket, then seal it.
    item can( itype_test_can_drink );
    REQUIRE( can.put_in( item( itype_water ), pocket_type::CONTAINER ).success() );
    std::vector<item_pocket *> can_pockets = can.get_container_pockets();
    REQUIRE( can_pockets.size() == 1 );
    REQUIRE( can_pockets.front()->sealable() );
    REQUIRE( can_pockets.front()->seal() );
    REQUIRE( can_pockets.front()->sealed() );

    item inner( itype_backpack );
    REQUIRE( inner.put_in( can, pocket_type::CONTAINER ).success() );
    REQUIRE( inner.put_in( item( itype_knife_combat ), pocket_type::CONTAINER ).success() );

    item outer( itype_debug_backpack );
    REQUIRE( outer.put_in( inner, pocket_type::CONTAINER ).success() );

    who.wear_item( outer );
    REQUIRE( who.is_wearing( itype_debug_backpack ) );
    REQUIRE( who.has_item_with( []( const item & cand ) {
        return cand.typeId() == itype_test_can_drink;
    } ) );
    return result;
}

// ------------------------------------------------------------------ F-03 --
two_sided_inventory_fixture build_two_sided_inventory()
{
    reset_world_for_fixture();
    map &here = get_map();
    avatar &who = get_avatar();

    two_sided_inventory_fixture result;
    result.ground_pos = fixture_origin;
    result.vehicle_pos = fixture_origin + tripoint_rel_ms::east;
    result.ground_item = itype_knife_combat;
    result.cargo_item = itype_jeans;
    result.carried_item = itype_bandages;
    result.worn_item = itype_test_rock;

    // Ground source (advanced_inventory_test.cpp places its ground item on
    // the avatar's own tile).
    here.add_item_or_charges( result.ground_pos, item( itype_knife_combat ) );

    // Vehicle source: shopping cart east of the avatar, item in its cargo.
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      result.vehicle_pos, 0_degrees, 0,
                                      veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    const std::optional<vpart_reference> cargo = here.veh_at( result.vehicle_pos ).cargo();
    REQUIRE( cargo );
    REQUIRE( cart->add_item( here, cargo->part(), item( itype_jeans ) ) );

    // On-person sources: bandages carried directly; a rock inside the worn
    // backpack's pocket.
    who.i_add( item( itype_bandages ) );
    item pack( itype_debug_backpack );
    REQUIRE( pack.put_in( item( itype_test_rock ), pocket_type::CONTAINER ).success() );
    who.wear_item( pack );
    REQUIRE( who.is_wearing( itype_debug_backpack ) );
    REQUIRE( player_has_item_of_type( itype_bandages ) );
    REQUIRE( player_has_item_of_type( itype_test_rock ) );
    return result;
}

// ------------------------------------------------------------------ F-04 --
wound_fixture build_wound_fixture()
{
    reset_world_for_fixture();
    avatar &who = get_avatar();

    wound_fixture result;
    result.injured_part = bodypart_id( "arm_r" );
    result.bandage_item = itype_bandages;
    result.disinfectant_item = itype_disinfectant;

    // Direct HP assignment (a11y_body_test.cpp mechanism): deterministic,
    // no damage/wound rolls. 3/5 of max keeps the limb damaged but intact
    // and bandageable.
    result.hp_max = who.get_part_hp_max( result.injured_part );
    REQUIRE( result.hp_max > 0 );
    result.hp_before = result.hp_max * 3 / 5;
    who.set_part_hp_cur( result.injured_part, result.hp_before );
    REQUIRE( who.get_part_hp_cur( result.injured_part ) == result.hp_before );
    REQUIRE_FALSE( who.is_limb_broken( result.injured_part ) );

    who.i_add( item( itype_bandages ) );
    who.i_add( item( itype_disinfectant ) );
    REQUIRE( player_has_item_of_type( itype_bandages ) );
    REQUIRE( player_has_item_of_type( itype_disinfectant ) );
    return result;
}

// ------------------------------------------------------------------ F-05 --
combat_field_fixture build_combat_field()
{
    reset_world_for_fixture();
    map &here = get_map();
    avatar &who = get_avatar();

    combat_field_fixture result;
    result.avatar_pos = fixture_origin;
    result.visible_target_pos = fixture_origin + tripoint_rel_ms{ 3, 0, 0 };
    result.empty_pos = fixture_origin + tripoint_rel_ms::east;
    result.ally_pos = fixture_origin + tripoint_rel_ms::west;
    result.other_level_pos = tripoint_bub_ms( fixture_origin.x() + 6, fixture_origin.y(), 1 );
    result.aoe_spell = spell_test_spell_lava;

    // Load submaps up to z = +1 as well (monster_stair_test.cpp clears
    // with clear_map( -2, 2 ) for the same reason).
    clear_map( -2, 2 );
    build_test_map( ter_id( "t_grass" ) );
    // Floor for the other-level target, built explicitly per z-level like
    // monster_stair_test.cpp's build_tower().
    here.ter_set( result.other_level_pos, ter_id( "t_floor" ) );
    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );
    here.invalidate_map_cache( 1 );
    here.build_map_cache( 1, true );
    set_time_to_day();

    spawn_test_monster( "mon_zombie", result.visible_target_pos );
    spawn_test_monster( "mon_zombie", result.other_level_pos );

    npc &ally = spawn_npc( result.ally_pos.xy(), "test_talker" );
    ally.set_attitude( NPCATT_FOLLOW );

    // The empty tile stays empty: nothing is placed there on purpose.
    REQUIRE( here.i_at( result.empty_pos ).empty() );

    who.magic->learn_spell( spell_test_spell_lava, who, true );
    REQUIRE( who.magic->knows_spell( spell_test_spell_lava ) );
    // test_spell_lava has min_aoe 4 in TEST_DATA magic.json: a real area.
    REQUIRE( who.magic->get_spell( spell_test_spell_lava ).aoe( who ) > 0 );
    return result;
}

// ------------------------------------------------------------------ F-06 --
vehicle_fixture build_vehicle_fixture()
{
    reset_world_for_fixture();
    map &here = get_map();

    vehicle_fixture result;
    result.origin = tripoint_bub_ms( fixture_origin.x() + 2, fixture_origin.y(), 0 );
    result.fuel_type = itype_gasoline;
    result.fuel_percent = 50;

    vehicle *veh = here.add_vehicle( vehicle_prototype_car, result.origin, 0_degrees,
                                     result.fuel_percent, veh_spawn_status::UNDAMAGED, false );
    REQUIRE( veh != nullptr );
    result.veh = veh;
    REQUIRE( size( veh->get_all_parts() ) > 1 );
    REQUIRE( veh->fuel_left( here, itype_gasoline ) > 0 );

    // Exactly one broken part: the engine, set to 0 HP directly.
    REQUIRE( !veh->engines.empty() );
    vehicle_part &engine = veh->part( veh->engines[0] );
    result.broken_part_name = engine.name( false );
    veh->set_hp( engine, 0, true );
    REQUIRE( engine.is_broken() );

    // Cargo: fixed-order scan for the first cargo part (veh_at().cargo()),
    // then two fixed items inside it.
    std::optional<vpart_reference> cargo;
    for( int dx = -4; dx <= 4 && !cargo; ++dx ) {
        for( int dy = -4; dy <= 4 && !cargo; ++dy ) {
            const tripoint_bub_ms pos = result.origin + tripoint_rel_ms{ dx, dy, 0 };
            const optional_vpart_position ovp = here.veh_at( pos );
            if( ovp ) {
                cargo = ovp.cargo();
                if( cargo ) {
                    result.cargo_pos = pos;
                }
            }
        }
    }
    REQUIRE( cargo );
    REQUIRE( veh->add_item( here, cargo->part(), item( itype_jeans ) ) );
    REQUIRE( veh->add_item( here, cargo->part(), item( itype_test_rock ) ) );
    return result;
}

// ------------------------------------------------------------------ F-07 --
zones_rules_fixture build_zones_and_rules()
{
    reset_world_for_fixture();
    map &here = get_map();

    zones_rules_fixture result;
    result.safemode_rule = "zombie";
    result.autopickup_item = itype_test_rock;

    const tripoint_bub_ms food_a( fixture_origin.x() - 2, fixture_origin.y() - 2, 0 );
    const tripoint_bub_ms food_b( fixture_origin.x() - 1, fixture_origin.y() - 1, 0 );
    const tripoint_bub_ms unsorted( fixture_origin.x() + 2, fixture_origin.y() + 2, 0 );
    result.food_zone_start = here.get_abs( food_a );
    result.food_zone_end = here.get_abs( food_b );
    result.unsorted_zone_pos = here.get_abs( unsorted );

    // zone_manager::add() with the argument shape of clzones_test.cpp's
    // create_tile_zone() (its last argument is the add() silent flag).
    zone_manager &zm = zone_manager::get_manager();
    zm.clear();
    zm.add( "A11y Loot Food", zone_type_id( "LOOT_FOOD" ), your_fac, false, true,
            result.food_zone_start, result.food_zone_end, nullptr, false );
    zm.add( "A11y Unsorted", zone_type_id( "LOOT_UNSORTED" ), your_fac, false, true,
            result.unsorted_zone_pos, result.unsorted_zone_pos, nullptr, false );
    REQUIRE( zm.has( zone_type_id( "LOOT_FOOD" ), result.food_zone_start ) );
    REQUIRE( zm.has( zone_type_id( "LOOT_UNSORTED" ), result.unsorted_zone_pos ) );

    // Root rule stores: start empty, then one sample rule each.
    safemode &safe = get_safemode();
    safe.clear_character_rules();
    safe.add_rule( result.safemode_rule, Creature::Attitude::HOSTILE, 5,
                   rule_state::BLACKLISTED );
    REQUIRE( safe.has_rule( result.safemode_rule, Creature::Attitude::HOSTILE ) );

    auto_pickup::player_settings &pickup = get_auto_pickup();
    pickup.clear_character_rules();
    const item sample( itype_test_rock );
    pickup.add_rule( &sample, true );
    REQUIRE( pickup.has_rule( &sample ) );
    return result;
}

// ------------------------------------------------------------------ F-08 --
camp_fixture build_faction_camp()
{
    reset_world_for_fixture();
    clear_basecamps();
    map &here = get_map();
    avatar &who = get_avatar();

    camp_fixture result;
    const tripoint_bub_ms mid( MAPSIZE_X / 2, MAPSIZE_Y / 2, 0 );
    result.camp_omt = project_to<coords::omt>( here.get_abs( mid ) );
    result.resident_pos = mid + tripoint_rel_ms{ 2, 0, 0 };
    result.food_kcal = 100;

    // Camp placement copied from faction_camp_test.cpp's place_test_camp().
    here.add_camp( result.camp_omt, "faction_camp" );
    const std::optional<basecamp *> bcp = overmap_buffer.find_camp( result.camp_omt.xy() );
    REQUIRE( bcp );
    basecamp &camp = **bcp;
    camp.set_owner( your_fac );
    camp.define_camp( result.camp_omt, "faction_base_bare_bones_NPC_camp_0", false );
    result.camp = &camp;

    // Food and storage zones, as in faction_camp_test.cpp's first case.
    const tripoint_abs_ms zone_loc = here.get_abs( mid );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_id( "CAMP_FOOD" ), your_fac, {},
                       "food" );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_id( "CAMP_STORAGE" ), your_fac, {},
                       "storage" );

    // Resident.
    npc &resident = spawn_npc( result.resident_pos.xy(), "test_talker" );
    resident.set_attitude( NPCATT_FOLLOW );

    // Assigned work for the avatar (mission_test.cpp's pattern).
    mission::clear_all();
    who.reset_all_missions();
    mission *job = mission::reserve_new( mission_type_id( "TEST_MISSION_FIND_ITEM" ),
                                         character_id() );
    REQUIRE( job != nullptr );
    if( job->get_assigned_player_id() == who.getID() ) {
        job->set_assigned_player_id( character_id( -2 ) );
    }
    job->assign( who );
    REQUIRE( !who.get_active_missions().empty() );

    // Exact food supply: 100000 nutrients calories at turn_zero read back
    // as exactly 100 kcal (faction_camp_test.cpp, camp_calorie_counting).
    faction *camp_faction = who.get_faction();
    REQUIRE( camp_faction != nullptr );
    camp_faction->empty_food_supply();
    std::map<time_point, nutrients> food;
    food[calendar::turn_zero].calories = 100000;
    camp_faction->add_to_food_supply( food );
    REQUIRE( camp_faction->food_supply().kcal() == result.food_kcal );
    return result;
}

void clear_faction_camp( const camp_fixture &fixture )
{
    overmap_buffer.clear_camps( fixture.camp_omt.xy() );
    if( faction *camp_faction = get_player_character().get_faction() ) {
        camp_faction->empty_food_supply();
    }
}

// ------------------------------------------------------------------ F-09 --
abilities_fixture build_abilities()
{
    reset_world_for_fixture();
    map &here = get_map();
    avatar &who = get_avatar();

    abilities_fixture result;
    result.computer_pos = fixture_origin + tripoint_rel_ms::east;
    result.installed_bionic = bio_power_storage;
    result.uninstalled_cbm = itype_id( "bio_power_storage" );
    result.known_spell = spell_test_spell_pew;
    result.spellbook_item = itype_test_textbook_fabrication;
    result.spellbook_spell = spell_test_spell_lava.str();

    // Installed bionic (bionics_test.cpp mechanism).
    who.add_bionic( bio_power_storage );
    REQUIRE( who.has_bionic( bio_power_storage ) );

    // Uninstalled CBM item: the item type whose id matches the bionic id
    // in data/json/items/bionics.json.
    who.i_add( item( result.uninstalled_cbm ) );
    REQUIRE( player_has_item_of_type( result.uninstalled_cbm ) );

    // Known spell (magic_spell_test.cpp mechanism).
    who.magic->learn_spell( spell_test_spell_pew, who, true );
    REQUIRE( who.magic->knows_spell( spell_test_spell_pew ) );

    // Spellbook PROP (limitation, also in FIXTURES.vi.md): no item in core
    // data or TEST_DATA teaches a spell (searched data/json/items/book/
    // and data/mods/TEST_DATA/books.json; DEBUG_spellbook belongs to the
    // Magiclysm mod, which the test harness does not load by default). So
    // the fixture carries a real BOOK item whose "spell_id" variable names
    // the spell it stands for. An oracle must treat that variable as the
    // fixture's declared teaching spell, not as engine spellbook data.
    item spellbook( itype_test_textbook_fabrication );
    spellbook.set_var( "spell_id", result.spellbook_spell );
    who.i_add( spellbook );
    REQUIRE( character_has_item_with_var_val( who, "spell_id", result.spellbook_spell ) );

    // A computer really on the map (map::add_computer), with one option.
    computer *terminal = here.add_computer( result.computer_pos,
                                            "A11y Fixture Terminal", 0 );
    REQUIRE( terminal != nullptr );
    terminal->add_option( "Fixture geiger counter", COMPACT_GEIGER, 0 );
    REQUIRE( terminal->options.size() == 1 );
    result.terminal = terminal;
    REQUIRE( here.computer_at( result.computer_pos ) == terminal );
    return result;
}

// ----------------------------------------------------------------- F-15 ---
lifecycle_stage_fixture build_lifecycle_stage( const int stage )
{
    REQUIRE( ( stage == 1 || stage == 2 ) );
    // The isolation step: a fresh character state plus an explicitly
    // cleared map memory, mirroring game.cpp's clear_map_memory() on
    // session replacement. reset_world_for_fixture() already clears both;
    // the extra clear_map_memory() call here is the fixture's contract.
    reset_world_for_fixture();
    map &here = get_map();
    avatar &who = get_avatar();
    who.clear_map_memory();
    prepare_fixture_memory( who, here );

    lifecycle_stage_fixture result;
    result.stage = stage;
    if( stage == 1 ) {
        result.known_marker_pos = fixture_origin + tripoint_rel_ms{ 2, 0, 0 };
        result.known_terrain_id = "t_floor";
        result.known_spell = spell_test_spell_pew;
    } else {
        result.known_marker_pos = fixture_origin + tripoint_rel_ms{ 4, 0, 0 };
        result.known_terrain_id = "t_brick_wall";
        result.known_spell = spell_test_spell_lava;
    }
    who.memorize_terrain( here.get_abs( result.known_marker_pos ),
                          result.known_terrain_id, 0, 0 );
    who.magic->learn_spell( result.known_spell, who, true );

    REQUIRE( who.has_memory_at( here.get_abs( result.known_marker_pos ) ) );
    REQUIRE( who.magic->knows_spell( result.known_spell ) );
    // The other stage's knowledge must not leak into this stage.
    const spell_id other_spell = stage == 1 ? spell_test_spell_lava : spell_test_spell_pew;
    REQUIRE_FALSE( who.magic->knows_spell( other_spell ) );
    return result;
}

// ------------------------------------------------------------- minigames ---
minesweeper_spec minesweeper_beginner_spec( const unsigned int seed )
{
    // Derivation from minesweeper_game::new_level() (Beginner branch and
    // bomb loop) in src/iuse_software_minesweeper.cpp; see the header.
    minesweeper_spec result;
    result.seed = seed;
    rng_set_engine_seed( seed );
    std::vector<std::vector<bool>> mined( result.height,
                                          std::vector<bool>( result.width, false ) );
    while( static_cast<int>( result.mines.size() ) < result.mine_count ) {
        const point candidate( rng( 0, result.width - 1 ), rng( 0, result.height - 1 ) );
        if( !mined[candidate.y][candidate.x] ) {
            mined[candidate.y][candidate.x] = true;
            result.mines.push_back( candidate );
        }
    }
    return result;
}

lightson_spec lightson_spec_for_seed( const unsigned int seed )
{
    // Derivation from lightson_game::new_level() + generate_change_coords()
    // + reset_level()/toggle_lights_at() in src/iuse_software_lightson.cpp;
    // see the header for the exact call order being replayed.
    lightson_spec result;
    result.seed = seed;
    rng_set_engine_seed( seed );
    const int half_perimeter = rng( 8, 11 );
    const int width = rng( 4, 6 );
    const int height = half_perimeter - width;
    result.size = point( width, height );
    const int steps = static_cast<int>( half_perimeter / 2.0 + rng_float( 0.0, 2.0 ) );

    const int cell_count = width * height;
    for( int k = 0; k < steps; ++k ) {
        point candidate;
        bool duplicate = false;
        do {
            const int candidate_index = rng( 0, cell_count - 1 );
            candidate = point( candidate_index % width, candidate_index / width );
            duplicate = k != 0 && std::find( result.change_coords.begin(),
                                             result.change_coords.end(),
                                             candidate ) != result.change_coords.end();
        } while( duplicate );
        result.change_coords.push_back( candidate );
    }

    // reset_level(): all lights on, then each change coord toggles itself
    // and its orthogonal neighbours.
    std::vector<bool> lit( static_cast<size_t>( cell_count ), true );
    const auto toggle = [&]( const point & p ) {
        if( p.x >= 0 && p.x < width && p.y >= 0 && p.y < height ) {
            lit[static_cast<size_t>( p.y * width + p.x )] =
                !lit[static_cast<size_t>( p.y * width + p.x )];
        }
    };
    for( const point &coord : result.change_coords ) {
        toggle( coord );
        toggle( coord + point::north );
        toggle( coord + point::south );
        toggle( coord + point::west );
        toggle( coord + point::east );
    }
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            if( !lit[static_cast<size_t>( y * width + x )] ) {
                result.dark_cells.emplace_back( x, y );
            }
        }
    }
    return result;
}

sokoban_spec sokoban_level_one_spec()
{
    // Rows verbatim from data/raw/sokoban.txt (the first level, "; #1");
    // coordinates read off them per the legend in
    // sokoban_game::parse_level(). No seed exists in this game.
    sokoban_spec result;
    result.rows = {
        " #####",
        " #   ####",
        " #   #  #",
        " ##    .#",
        "### ###.#",
        "# $ # #.#",
        "# $$# ###",
        "#@  #",
        "#####",
    };
    result.size = point( 9, 9 );
    result.player = point( 1, 7 );
    result.packages = { point( 2, 5 ), point( 2, 6 ), point( 3, 6 ) };
    result.goals = { point( 7, 3 ), point( 7, 4 ), point( 7, 5 ) };
    return result;
}

kitten_spec kitten_spec_for_seed( const unsigned int seed )
{
    // Geometry/counts only -- positions are not derivable from a seed
    // without running the constructor's UI loop (see the header).
    kitten_spec result;
    result.seed = seed;
    return result;
}

snake_spec snake_spec_for_board( const int width, const int height, const unsigned int seed )
{
    // Start state and first-fruit draw from snake_game::start_game() in
    // src/iuse_software_snake.cpp, for the caller-declared board size.
    snake_spec result;
    result.board_size = point( width, height );
    result.head = point( width / 2, height / 2 );
    result.seed = seed;
    rng_set_engine_seed( seed );
    point fruit;
    do {
        fruit = point( rng( 1, width - 2 ), rng( 1, height - 2 ) );
    } while( fruit == result.head );
    result.fruit = fruit;
    return result;
}

} // namespace a11y_fixture
