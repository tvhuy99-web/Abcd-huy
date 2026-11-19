#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "monster.h"
#include "point.h"
#include "type_id.h"

static const mtype_id mon_bear( "mon_bear" );
static const mtype_id mon_beaver( "mon_beaver" );
static const mtype_id mon_chicken( "mon_chicken" );
static const mtype_id mon_cougar( "mon_cougar" );
static const mtype_id mon_cow( "mon_cow" );
static const mtype_id mon_deer( "mon_deer" );
static const mtype_id mon_dog( "mon_dog" );
static const mtype_id mon_guinea_fowl( "mon_guinea_fowl" );
static const mtype_id mon_horse( "mon_horse" );
static const mtype_id mon_opossum( "mon_opossum" );
static const mtype_id mon_test_zombie( "mon_test_zombie" );
static const mtype_id mon_turkey_domestic( "mon_turkey_domestic" );
static const mtype_id mon_wolf( "mon_wolf" );

static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_ladder_down( "t_ladder_down" );
static const ter_str_id ter_t_ladder_up( "t_ladder_up" );
static const ter_str_id ter_t_stairs_down( "t_stairs_down" );
static const ter_str_id ter_t_stairs_up( "t_stairs_up" );
static const ter_str_id ter_t_wall( "t_wall" );





static void build_tower( map &here, const point_bub_ms &origin,
                         const ter_str_id &up_ter, const ter_str_id &down_ter )
{
    for( int dx = 0; dx < 3; ++dx ) {
        for( int dy = 0; dy < 3; ++dy ) {
            const bool is_wall = dx == 0 || dx == 2 || dy == 0 || dy == 2;
            const bool is_center = dx == 1 && dy == 1;
            const tripoint_bub_ms p0( origin.x() + dx, origin.y() + dy, 0 );
            const tripoint_bub_ms p1( origin.x() + dx, origin.y() + dy, 1 );
            const tripoint_bub_ms p2( origin.x() + dx, origin.y() + dy, 2 );
            if( is_wall ) {
                here.ter_set( p0, ter_t_wall.id() );
                here.ter_set( p1, ter_t_wall.id() );
            } else if( is_center ) {
                here.ter_set( p0, up_ter.id() );
                here.ter_set( p1, down_ter.id() );
            } else {
                here.ter_set( p0, ter_t_floor.id() );
                here.ter_set( p1, ter_t_floor.id() );
            }

            here.ter_set( p2, ter_t_floor.id() );
        }
    }
}



static std::set<int> run_creature_in_tower( map &here, const point_bub_ms &tower_origin,
        const mtype_id &mon_type, int start_z, int dest_z, int turns )
{

    const tripoint_bub_ms spawn_pos( tower_origin.x() + 1, tower_origin.y() + 1, start_z );

    const tripoint_bub_ms dest( tower_origin.x() + 1, tower_origin.y() + 1, dest_z );

    monster &mon = spawn_test_monster( mon_type.str(), spawn_pos, false );
    mon.anger = 100;
    mon.morale = 100;
    mon.set_dest( here.get_abs( dest ) );

    std::set<int> visited_z;
    visited_z.insert( mon.posz() );

    for( int turn = 0; turn < turns; ++turn ) {



        mon.mod_moves( mon.get_speed() );
        while( mon.get_moves() > 0 ) {
            mon.move();
            visited_z.insert( mon.posz() );
        }
    }

    g->remove_zombie( mon );
    return visited_z;
}

static void setup_towers( map &here, const ter_str_id &up_ter, const ter_str_id &down_ter,
                          int count, int spacing, std::vector<point_bub_ms> &origins )
{
    origins.clear();
    for( int i = 0; i < count; ++i ) {
        origins.emplace_back( 10 + i * spacing, 10 );
        build_tower( here, origins.back(), up_ter, down_ter );
    }
    for( int z = 0; z <= 2; ++z ) {
        here.invalidate_map_cache( z );
        here.build_map_cache( z, true );
    }
}



static const std::vector<std::pair<mtype_id, std::string>> ground_creatures = {
    { mon_test_zombie, "zombie" },
    { mon_dog, "dog" },
    { mon_cow, "cow" },
    { mon_horse, "horse" },
    { mon_deer, "deer" },
    { mon_chicken, "chicken" },
    { mon_turkey_domestic, "domestic turkey" },
    { mon_guinea_fowl, "guinea fowl" },
    { mon_cougar, "cougar" },
    { mon_wolf, "wolf" },
    { mon_beaver, "beaver" },
    { mon_bear, "bear" },
};


static const std::vector<std::pair<mtype_id, std::string>> climbing_creatures = {
    { mon_opossum, "opossum" },
};

static constexpr int test_turns = 30;

TEST_CASE( "creatures_ascend_regular_stairs", "[monster][z-level]" )
{
    clear_map( -2, 2 );
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 60, 60, 0 ) );

    std::vector<point_bub_ms> origins;
    setup_towers( here, ter_t_stairs_up, ter_t_stairs_down,
                  ground_creatures.size() + climbing_creatures.size(), 5, origins );

    int idx = 0;
    for( const auto &[mon_type, label] : ground_creatures ) {
        DYNAMIC_SECTION( label << " ascends stairs (z=0 to z=1)" ) {
            std::set<int> visited = run_creature_in_tower(
                                        here, origins[idx], mon_type, 0, 1, test_turns );
            CHECK( visited.count( 1 ) == 1 );
        }
        ++idx;
    }
    for( const auto &[mon_type, label] : climbing_creatures ) {
        DYNAMIC_SECTION( label << " ascends stairs (z=0 to z=1)" ) {
            std::set<int> visited = run_creature_in_tower(
                                        here, origins[idx], mon_type, 0, 1, test_turns );
            CHECK( visited.count( 1 ) == 1 );
        }
        ++idx;
    }
}

TEST_CASE( "creatures_descend_regular_stairs", "[monster][z-level]" )
{
    clear_map( -2, 2 );
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 60, 60, 0 ) );

    std::vector<point_bub_ms> origins;
    setup_towers( here, ter_t_stairs_up, ter_t_stairs_down,
                  ground_creatures.size() + climbing_creatures.size(), 5, origins );

    int idx = 0;
    for( const auto &[mon_type, label] : ground_creatures ) {
        DYNAMIC_SECTION( label << " descends stairs (z=1 to z=0)" ) {
            std::set<int> visited = run_creature_in_tower(
                                        here, origins[idx], mon_type, 1, 0, test_turns );
            CHECK( visited.count( 0 ) == 1 );
        }
        ++idx;
    }
    for( const auto &[mon_type, label] : climbing_creatures ) {
        DYNAMIC_SECTION( label << " descends stairs (z=1 to z=0)" ) {
            std::set<int> visited = run_creature_in_tower(
                                        here, origins[idx], mon_type, 1, 0, test_turns );
            CHECK( visited.count( 0 ) == 1 );
        }
        ++idx;
    }
}

TEST_CASE( "ground_creatures_blocked_by_ladders", "[monster][z-level]" )
{
    clear_map( -2, 2 );
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 60, 60, 0 ) );

    std::vector<point_bub_ms> origins;
    setup_towers( here, ter_t_ladder_up, ter_t_ladder_down,
                  ground_creatures.size(), 5, origins );

    int idx = 0;
    for( const auto &[mon_type, label] : ground_creatures ) {
        DYNAMIC_SECTION( label << " cannot climb ladder (z=0 to z=1)" ) {
            std::set<int> visited = run_creature_in_tower(
                                        here, origins[idx], mon_type, 0, 1, test_turns );
            CHECK( visited.count( 1 ) == 0 );
        }
        ++idx;
    }
}

TEST_CASE( "climbing_creatures_use_ladders", "[monster][z-level]" )
{
    clear_map( -2, 2 );
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 60, 60, 0 ) );

    std::vector<point_bub_ms> origins;
    setup_towers( here, ter_t_ladder_up, ter_t_ladder_down,
                  climbing_creatures.size(), 5, origins );

    int idx = 0;
    for( const auto &[mon_type, label] : climbing_creatures ) {
        DYNAMIC_SECTION( label << " climbs ladder (z=0 to z=1)" ) {
            std::set<int> visited = run_creature_in_tower(
                                        here, origins[idx], mon_type, 0, 1, test_turns );
            CHECK( visited.count( 1 ) == 1 );
        }
        ++idx;
    }
}




static void build_two_stair_tower( map &here, const point_bub_ms &origin )
{
    for( int dx = 0; dx < 4; ++dx ) {
        for( int dy = 0; dy < 3; ++dy ) {
            const tripoint_bub_ms p0( origin.x() + dx, origin.y() + dy, 0 );
            const tripoint_bub_ms p1( origin.x() + dx, origin.y() + dy, 1 );
            const tripoint_bub_ms p2( origin.x() + dx, origin.y() + dy, 2 );
            const bool is_col_a = dx == 1 && dy == 1;
            const bool is_col_b = dx == 2 && dy == 1;
            here.ter_set( p0, is_col_a ? ter_t_stairs_up.id() : ter_t_wall.id() );
            if( is_col_a ) {
                here.ter_set( p1, ter_t_stairs_down.id() );
            } else if( is_col_b ) {
                here.ter_set( p1, ter_t_stairs_up.id() );
            } else {
                here.ter_set( p1, ter_t_wall.id() );
            }
            here.ter_set( p2, is_col_b ? ter_t_stairs_down.id() : ter_t_wall.id() );
        }
    }
    for( int z = 0; z <= 2; ++z ) {
        here.invalidate_map_cache( z );
        here.build_map_cache( z, true );
    }
}




TEST_CASE( "wandering_creature_traverses_three_story_tower", "[monster][z-level][stumble]" )
{
    clear_map( -2, 2 );
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 60, 60, 0 ) );

    const point_bub_ms tower_origin( 10, 10 );
    build_two_stair_tower( here, tower_origin );

    const tripoint_bub_ms spawn_pos( tower_origin.x() + 2, tower_origin.y() + 1, 1 );
    monster &mon = spawn_test_monster( mon_dog.str(), spawn_pos, false );
    mon.anger = 0;
    mon.morale = 0;
    REQUIRE( mon.is_wandering() );

    std::set<int> visited_z;
    visited_z.insert( mon.posz() );

    constexpr int budget_turns = 1000;
    for( int turn = 0; turn < budget_turns; ++turn ) {
        mon.mod_moves( mon.get_speed() );
        while( mon.get_moves() > 0 ) {
            mon.move();
            visited_z.insert( mon.posz() );
        }
    }

    g->remove_zombie( mon );

    CAPTURE( visited_z );
    CHECK( visited_z.count( 0 ) == 1 );
    CHECK( visited_z.count( 1 ) == 1 );
    CHECK( visited_z.count( 2 ) == 1 );
}
