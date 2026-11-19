#include <memory>
#include <optional>
#include <string>

#include "avatar.h"
#include "avatar_action.h"
#include "cata_catch.h"
#include "character.h"
#include "character_id.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "mapdata.h"
#include "monster.h"
#include "npc.h"
#include "npc_attack.h"
#include "options_helpers.h"
#include "overmap_ui.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const faction_id faction_your_followers( "your_followers" );
static const itype_id itype_knife_large( "knife_large" );
static const mtype_id mon_fish_brook_trout( "mon_fish_brook_trout" );
static const mtype_id mon_sewer_fish( "mon_sewer_fish" );
static const mtype_id mon_zombie( "mon_zombie" );
static const string_id<npc_template> npc_template_test_talker( "test_talker" );
static const ter_str_id ter_t_bridge( "t_bridge" );
static const ter_str_id ter_t_floor( "t_floor" );
static const weather_type_id weather_sunny( "sunny" );

static constexpr tripoint_bub_ms fish_pos{ 65, 65, 0 };
static constexpr tripoint_bub_ms adjacent_pos{ 66, 65, 0 };

static void reset_caches( int zlev )
{
    Character &you = get_player_character();
    map &here = get_map();
    g->reset_light_level();
    here.invalidate_visibility_cache();
    here.update_visibility_cache( zlev );
    here.invalidate_map_cache( zlev );
    here.build_map_cache( zlev );
    here.invalidate_visibility_cache();
    here.update_visibility_cache( zlev );
    here.invalidate_map_cache( zlev );
    here.build_map_cache( zlev );
    you.recalc_sight_limits();
}


TEST_CASE( "monsters_cannot_attack_through_swim_under", "[swim_under][melee]" )
{
    clear_map_and_put_player_underground();
    clear_vehicles();
    clear_avatar();
    map &here = get_map();


    here.ter_set( fish_pos, ter_t_bridge );
    here.ter_set( adjacent_pos, ter_t_floor );

    SECTION( "underwater monster on SWIM_UNDER tile cannot attack adjacent surface creature" ) {

        monster &fish = spawn_test_monster( mon_sewer_fish.str(), fish_pos );
        fish.underwater = true;
        REQUIRE( fish.is_underwater() );
        REQUIRE( here.has_flag( ter_furn_flag::TFLAG_SWIM_UNDER, fish_pos ) );
        REQUIRE( !here.has_flag( ter_furn_flag::TFLAG_SWIM_UNDER, adjacent_pos ) );


        Character &player = get_player_character();
        player.setpos( here, adjacent_pos );
        REQUIRE( !player.is_underwater() );


        CHECK_FALSE( fish.attack_at( adjacent_pos ) );
    }

    SECTION( "underwater monsters on SWIM_UNDER can fight each other" ) {


        monster &predator = spawn_test_monster( mon_sewer_fish.str(), fish_pos );
        predator.underwater = true;


        here.ter_set( adjacent_pos, ter_t_bridge );
        monster &prey = spawn_test_monster( mon_fish_brook_trout.str(), adjacent_pos );
        prey.underwater = true;

        REQUIRE( predator.is_underwater() );
        REQUIRE( prey.is_underwater() );
        REQUIRE( predator.attitude_to( prey ) == Creature::Attitude::HOSTILE );


        CHECK( predator.attack_at( adjacent_pos ) );
    }

    SECTION( "player cannot melee underwater creature on SWIM_UNDER tile" ) {
        monster &fish = spawn_test_monster( mon_sewer_fish.str(), fish_pos );
        fish.underwater = true;

        Character &player = get_player_character();
        player.setpos( here, adjacent_pos );
        REQUIRE( fish.is_underwater() );
        REQUIRE( here.has_flag( ter_furn_flag::TFLAG_SWIM_UNDER, fish_pos ) );


        CHECK_FALSE( player.melee_attack( fish, false ) );
    }
}


TEST_CASE( "npc_does_not_target_swim_under_creatures", "[swim_under][npc_attack]" )
{
    clear_map_and_put_player_underground();
    clear_vehicles();
    scoped_weather_override sunny_weather( weather_sunny );
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();


    get_player_character().setpos( here, { 40, 40, 0 } );


    static constexpr point_bub_ms npc_start{ adjacent_pos.xy() };
    static constexpr tripoint_bub_ms npc_start_tri{ npc_start, 0 };
    REQUIRE( creatures.creature_at<Creature>( npc_start_tri ) == nullptr );
    const character_id npc_id = here.place_npc( npc_start, npc_template_test_talker );
    npc &test_npc = *g->find_npc( npc_id );
    clear_character( test_npc );
    test_npc.setpos( here, npc_start_tri );
    test_npc.set_fac( faction_your_followers );
    g->load_npcs();
    REQUIRE( creatures.creature_at<npc>( npc_start_tri ) != nullptr );


    item weapon( itype_knife_large );
    test_npc.set_wielded_item( weapon );

    SECTION( "NPC melee evaluation skips underwater SWIM_UNDER target" ) {
        here.ter_set( fish_pos, ter_t_bridge );
        here.ter_set( adjacent_pos, ter_t_floor );

        monster *fish = g->place_critter_at( mon_sewer_fish, fish_pos );
        REQUIRE( fish != nullptr );
        fish->underwater = true;
        REQUIRE( fish->is_underwater() );
        REQUIRE( here.has_flag( ter_furn_flag::TFLAG_SWIM_UNDER, fish_pos ) );

        test_npc.evaluate_best_attack( fish );
        const npc_attack_rating &rating = test_npc.get_current_attack_evaluation();

        CHECK_FALSE( rating.value().has_value() );
    }

    SECTION( "NPC melee evaluation works for normal hostile target" ) {
        here.ter_set( fish_pos, ter_t_floor );
        here.ter_set( adjacent_pos, ter_t_floor );

        monster *zombie = g->place_critter_at( mon_zombie, fish_pos );
        REQUIRE( zombie != nullptr );
        REQUIRE( !zombie->is_underwater() );

        test_npc.evaluate_best_attack( zombie );
        const npc_attack_rating &rating = test_npc.get_current_attack_evaluation();

        CHECK( rating.value().has_value() );
        CHECK( *rating.value() > 0 );
    }
}


TEST_CASE( "player_can_walk_over_swim_under_creatures", "[swim_under][movement]" )
{
    clear_map_and_put_player_underground();
    clear_vehicles();
    clear_avatar();
    scoped_weather_override sunny_weather( weather_sunny );
    map &here = get_map();

    here.ter_set( fish_pos, ter_t_bridge );
    here.ter_set( adjacent_pos, ter_t_floor );

    SECTION( "bumping into SWIM_UNDER tile with underwater creature does not attack" ) {
        monster &fish = spawn_test_monster( mon_sewer_fish.str(), fish_pos );
        fish.underwater = true;

        REQUIRE( fish.is_underwater() );
        REQUIRE( here.has_flag( ter_furn_flag::TFLAG_SWIM_UNDER, fish_pos ) );

        reset_caches( 0 );



        tripoint_rel_ms direction( fish_pos.raw() - adjacent_pos.raw() );
        avatar &you = get_avatar();
        you.setpos( here, adjacent_pos );
        you.set_moves( 200 );
        bool moved = avatar_action::move( you, here, direction );
        CHECK( moved );
        CHECK( you.pos_bub() == fish_pos );
    }
}


TEST_CASE( "creatures_under_swim_under_are_invisible", "[swim_under][vision]" )
{
    clear_map_and_put_player_underground();
    clear_vehicles();
    clear_avatar();
    scoped_weather_override sunny_weather( weather_sunny );
    map &here = get_map();

    here.ter_set( fish_pos, ter_t_bridge );
    here.ter_set( adjacent_pos, ter_t_floor );

    SECTION( "surface observer cannot see underwater creature on SWIM_UNDER tile" ) {
        monster &fish = spawn_test_monster( mon_sewer_fish.str(), fish_pos );
        fish.underwater = true;

        Character &player = get_player_character();
        player.setpos( here, adjacent_pos );
        REQUIRE( !player.is_underwater() );
        REQUIRE( fish.is_underwater() );
        REQUIRE( here.has_flag( ter_furn_flag::TFLAG_SWIM_UNDER, fish_pos ) );

        reset_caches( 0 );


        CHECK_FALSE( player.sees( here, fish ) );
    }

    SECTION( "underwater observer can see underwater creature on SWIM_UNDER tile" ) {
        here.ter_set( adjacent_pos, ter_t_bridge );

        monster &fish1 = spawn_test_monster( mon_sewer_fish.str(), fish_pos );
        fish1.underwater = true;

        monster &fish2 = spawn_test_monster( mon_sewer_fish.str(), adjacent_pos );
        fish2.underwater = true;

        reset_caches( 0 );


        CHECK( fish1.sees( here, fish2 ) );
    }
}
