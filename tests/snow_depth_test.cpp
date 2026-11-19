#include "cata_catch.h"

#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"

static const weather_type_id weather_snowing( "snowing" );
static const weather_type_id weather_sunny( "sunny" );

TEST_CASE( "snow_accumulation_and_melt", "[snow][weather]" )
{
    clear_map();
    weather_manager &weather = get_weather();
    weather.snow_depth_map.clear();


    calendar::turn -= ( calendar::turn - calendar::turn_zero ) % 1_minutes;

    Character &player = get_player_character();
    const tripoint_abs_omt player_omt = player.pos_abs_omt();

    SECTION( "snow accumulates during snowfall" ) {
        weather.weather_id = weather_snowing;
        weather.temperature = units::from_celsius( -5 );


        for( int i = 0; i < 30; i++ ) {
            calendar::turn += 1_minutes;
            weather.update_snow_depth();
        }

        CHECK( weather.get_snow_depth_mm( player_omt ) > 0.0 );
    }

    SECTION( "snow melts above freezing" ) {

        weather.snow_depth_map[player_omt] = { 200.0, calendar::turn };

        weather.weather_id = weather_sunny;
        weather.temperature = 10_C;

        for( int i = 0; i < 30; i++ ) {
            calendar::turn += 1_minutes;
            weather.update_snow_depth();
        }

        CHECK( weather.get_snow_depth_mm( player_omt ) < 200.0 );
    }

    SECTION( "no snow in warm weather" ) {
        weather.weather_id = weather_sunny;
        weather.temperature = 20_C;

        for( int i = 0; i < 30; i++ ) {
            calendar::turn += 1_minutes;
            weather.update_snow_depth();
        }

        CHECK( weather.get_snow_depth_mm( player_omt ) < 0.001 );
    }

    SECTION( "unvisited OMT seeds from nearest neighbor" ) {
        weather.snow_depth_map[player_omt] = { 150.0, calendar::turn };


        tripoint_abs_omt neighbor = player_omt + tripoint::east;
        CHECK( weather.get_snow_depth_mm( neighbor ) == Approx( 150.0 ) );
    }

    SECTION( "depth is clamped to maximum" ) {
        weather.snow_depth_map[player_omt] = { 790.0, calendar::turn };
        weather.weather_id = weather_snowing;
        weather.temperature = units::from_celsius( -10 );


        for( int i = 0; i < 60; i++ ) {
            calendar::turn += 1_minutes;
            weather.update_snow_depth();
        }

        CHECK( weather.get_snow_depth_mm( player_omt ) <= 800.0 );
    }

    SECTION( "all tracked OMTs accumulate snow, not just the player's" ) {

        tripoint_abs_omt far_omt = player_omt + tripoint( 10, 10, 0 );
        weather.snow_depth_map[far_omt] = { 50.0, calendar::turn };

        weather.weather_id = weather_snowing;
        weather.temperature = units::from_celsius( -5 );

        for( int i = 0; i < 30; i++ ) {
            calendar::turn += 1_minutes;
            weather.update_snow_depth();
        }


        CHECK( weather.get_snow_depth_mm( far_omt ) > 50.0 );
    }

    SECTION( "new OMT entry is seeded from neighbors, not zero" ) {

        weather.snow_depth_map[player_omt] = { 400.0, calendar::turn };

        weather.weather_id = weather_snowing;
        weather.temperature = units::from_celsius( -5 );






        tripoint_abs_omt adjacent = player_omt + tripoint::east;


        CHECK( weather.get_snow_depth_mm( adjacent ) == Approx( 400.0 ) );


        weather.snow_depth_map[adjacent] = { weather.get_snow_depth_mm( adjacent ),
                                             calendar::turn
                                           };

        calendar::turn += 1_minutes;
        weather.update_snow_depth();


        CHECK( weather.get_snow_depth_mm( adjacent ) > 400.0 );
    }

    SECTION( "snow depth is the same regardless of z-level" ) {
        weather.snow_depth_map.clear();
        weather.weather_id = weather_snowing;
        weather.temperature = units::from_celsius( -5 );


        for( int i = 0; i < 30; i++ ) {
            calendar::turn += 1_minutes;
            weather.update_snow_depth();
        }

        double depth_z0 = weather.get_snow_depth_mm( player_omt );
        REQUIRE( depth_z0 > 0.0 );


        tripoint_abs_omt above_omt( player_omt.x(), player_omt.y(), player_omt.z() + 1 );
        CHECK( weather.get_snow_depth_mm( above_omt ) == Approx( depth_z0 ) );
    }

    SECTION( "catch-up delta is clamped" ) {

        weather.snow_depth_map[player_omt] = { 0.0, calendar::turn - 3_days };
        weather.weather_id = weather_snowing;
        weather.temperature = units::from_celsius( -5 );

        calendar::turn += 1_minutes;
        weather.update_snow_depth();


        double depth = weather.get_snow_depth_mm( player_omt );
        CHECK( depth > 0.0 );
        CHECK( depth < 200.0 );
    }
}

TEST_CASE( "snow_accumulates_uniformly_across_visited_OMTs", "[snow][weather]" )
{
    clear_map();
    clear_character( get_player_character() );
    weather_manager &weather = get_weather();
    weather.snow_depth_map.clear();

    calendar::turn -= ( calendar::turn - calendar::turn_zero ) % 1_minutes;

    Character &player = get_player_character();
    map &here = get_map();

    weather.weather_id = weather_snowing;
    weather.temperature = units::from_celsius( -5 );





    const tripoint_bub_ms origin = player.pos_bub();
    const std::vector<point> omt_path = {
        point::zero,
        point::north,
        point::north_east,
        point::east,
        point::south_east,
        point::south,
        point::south_west,
        point::west,
        point::north_west
    };

    std::vector<tripoint_abs_omt> visited_omts;

    for( const point &omt_offset : omt_path ) {

        const tripoint_bub_ms dest(
            origin.x() + omt_offset.x * 24,
            origin.y() + omt_offset.y * 24,
            origin.z()
        );
        player.setpos( here, dest );
        visited_omts.push_back( player.pos_abs_omt() );


        for( int m = 0; m < 60; m++ ) {
            calendar::turn += 1_minutes;
            weather.update_snow_depth();
        }
    }


    REQUIRE( visited_omts.size() == 9 );






    const double lower_bound = 300.0;

    for( size_t i = 0; i < visited_omts.size(); i++ ) {
        const double depth = weather.get_snow_depth_mm( visited_omts[i] );
        CAPTURE( i, visited_omts[i], depth );
        CHECK( depth > lower_bound );
    }


    const double first = weather.get_snow_depth_mm( visited_omts.front() );
    const double last = weather.get_snow_depth_mm( visited_omts.back() );
    CHECK( std::abs( first - last ) < 10.0 );
}

TEST_CASE( "snow_movement_penalty", "[snow][movement]" )
{
    clear_map();
    clear_character( get_player_character() );
    weather_manager &weather = get_weather();
    weather.snow_depth_map.clear();

    Character &player = get_player_character();
    map &here = get_map();
    const tripoint_abs_omt player_omt = player.pos_abs_omt();


    here.ter_set( player.pos_bub(), ter_id( "t_grass" ) );
    here.ter_set( player.pos_bub() + tripoint::above, ter_id( "t_open_air" ) );
    here.build_map_cache( player.posz() );
    here.build_map_cache( player.posz() + 1 );

    SECTION( "heavy snow increases movement cost" ) {
        weather.snow_depth_map[player_omt] = { 600.0, calendar::turn };
        weather.weather_id = weather_sunny;
        weather.temperature = units::from_celsius( -5 );

        const int cost_with_snow = player.run_cost( 100 );

        weather.snow_depth_map[player_omt] = { 0.0, calendar::turn };
        const int cost_without_snow = player.run_cost( 100 );

        CHECK( cost_with_snow > cost_without_snow );
    }

    SECTION( "no penalty indoors" ) {
        weather.snow_depth_map[player_omt] = { 600.0, calendar::turn };


        here.ter_set( player.pos_bub() + tripoint::above, ter_id( "t_floor" ) );
        here.build_map_cache( player.posz() );
        here.build_map_cache( player.posz() + 1 );


        const int cost_roofed = player.run_cost( 100 );


        here.ter_set( player.pos_bub() + tripoint::above, ter_id( "t_open_air" ) );
        here.build_map_cache( player.posz() );
        here.build_map_cache( player.posz() + 1 );
        const int cost_unroofed = player.run_cost( 100 );

        CHECK( cost_roofed < cost_unroofed );
    }
}
