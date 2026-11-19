#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "monster.h"
#include "point.h"
#include "type_id.h"

static const ter_str_id ter_t_floor( "t_floor" );

static monster &spawn_and_clear( const tripoint_bub_ms &pos, bool set_floor )
{
    if( set_floor ) {
        get_map().set( pos, ter_t_floor, furn_str_id::NULL_ID() );
    }
    return spawn_test_monster( "mon_zombie", pos );
}

static const time_point midday = calendar::turn_zero + 12_hours;

TEST_CASE( "monsters_should_not_see_through_floors", "[vision]" )
{
    const map &here = get_map();

    calendar::turn = midday;
    clear_map( -2, 1 );
    monster &upper = spawn_and_clear( { 5, 5, 0 }, true );
    monster &adjacent = spawn_and_clear( { 5, 6, 0 }, true );
    monster &distant = spawn_and_clear( { 5, 3, 0 }, true );
    monster &lower = spawn_and_clear( { 5, 5, -1 }, true );
    monster &deep = spawn_and_clear( { 5, 5, -2 }, true );
    monster &sky = spawn_and_clear( { 5, 5, 1 }, false );



    CHECK( !upper.sees( here, lower ) );
    CHECK( !lower.sees( here, upper ) );

    CHECK( !upper.sees( here, deep ) );
    CHECK( !deep.sees( here,  upper ) );

    CHECK( !sky.sees( here, lower ) );
    CHECK( !lower.sees( here, sky ) );

    CHECK( !adjacent.sees( here, lower ) );
    CHECK( !lower.sees( here,  adjacent ) );

    CHECK( !distant.sees( here, lower ) );
    CHECK( !lower.sees( here, distant ) );

    CHECK( !adjacent.sees( here,  deep ) );
    CHECK( !deep.sees( here, adjacent ) );

    CHECK( !distant.sees( here, deep ) );
    CHECK( !deep.sees( here,  distant ) );



    CHECK( upper.sees( here, sky ) );
    CHECK( sky.sees( here, upper ) );

    CHECK( upper.sees( here, adjacent ) );
    CHECK( adjacent.sees( here, upper ) );

    CHECK( upper.sees( here, distant ) );
    CHECK( distant.sees( here, upper ) );

    CHECK( sky.sees( here, adjacent ) );
    CHECK( adjacent.sees( here, sky ) );

    CHECK( sky.sees( here, distant ) );
    CHECK( distant.sees( here, sky ) );
}
