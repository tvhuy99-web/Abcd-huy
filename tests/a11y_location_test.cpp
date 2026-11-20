#include <optional>
#include <set>

#include "a11y_location.h"
#include "cata_catch.h"
#include "point.h"

TEST_CASE( "a11y_local_coordinates_use_one_origin_across_overmap_tiles",
           "[a11y][location]" )
{
    // An individual OMT is 24x24, but the place extends through OMTs
    // (125, 400), (126, 400), and (126, 401).
    const point northwest( 125, 400 );
    CHECK( a11y::location_detail::local_from_omt_corner(
               point( 3002, 9604 ), northwest ) == point( 3, 5 ) );
    CHECK( a11y::location_detail::local_from_omt_corner(
               point( 3041, 9627 ), northwest ) == point( 42, 28 ) );
    // Negative global coordinates must be translated, not remainderd.
    CHECK( a11y::location_detail::local_from_omt_corner(
               point( -48, -47 ), point( -2, -2 ) ) == point( 1, 2 ) );
}

TEST_CASE( "a11y_roads_split_at_intersections_not_at_24_tile_boundaries",
           "[a11y][location][road]" )
{
    std::set<point> roads;
    for( int x = 177; x <= 183; ++x ) {
        roads.insert( point( x, 0 ) );
    }
    roads.insert( point( 180, -1 ) );
    const auto probe = [&]( const point &p ) -> std::optional<bool> {
        return roads.count( p ) > 0;
    };

    const auto west = a11y::location_detail::find_road_section( point( 178, 0 ), probe );
    const auto east = a11y::location_detail::find_road_section( point( 181, 0 ), probe );
    const auto junction = a11y::location_detail::find_road_section( point( 180, 0 ), probe );
    REQUIRE( west.complete );
    REQUIRE( east.complete );
    REQUIRE( junction.complete );
    const std::set<point> expected_west = {
        point( 177, 0 ), point( 178, 0 ), point( 179, 0 )
    };
    const std::set<point> expected_east = {
        point( 181, 0 ), point( 182, 0 ), point( 183, 0 )
    };
    const std::set<point> expected_junction = { point( 180, 0 ) };
    CHECK( west.tiles == expected_west );
    CHECK( east.tiles == expected_east );
    CHECK( junction.tiles == expected_junction );
    CHECK( west.tiles.count( point( 180, 0 ) ) == 0 );
}

TEST_CASE( "a11y_roads_reject_incomplete_or_overlarge_regions",
           "[a11y][location][road]" )
{
    const auto unknown = []( const point &p ) -> std::optional<bool> {
        if( p == point( 0, -1 ) ) {
            return std::nullopt;
        }
        return p == point( 0, 0 );
    };
    const auto uncertain = a11y::location_detail::find_road_section( point( 0, 0 ), unknown );
    CHECK_FALSE( uncertain.complete );
    CHECK( uncertain.tiles.empty() );

    std::set<point> straight;
    for( int x = 0; x < 100; ++x ) {
        straight.insert( point( x, 0 ) );
    }
    const auto long_road = [&]( const point &p ) -> std::optional<bool> {
        return straight.count( p ) > 0;
    };
    const auto truncated = a11y::location_detail::find_road_section(
                               point( 50, 0 ), long_road, 20 );
    CHECK_FALSE( truncated.complete );
    CHECK( truncated.tiles.empty() );
}

TEST_CASE( "a11y_roads_handle_closed_loops_without_guessing_an_origin",
           "[a11y][location][road]" )
{
    const std::set<point> loop = {
        point( 0, 0 ), point( 1, 0 ), point( 2, 0 ),
        point( 2, 1 ), point( 2, 2 ), point( 1, 2 ),
        point( 0, 2 ), point( 0, 1 )
    };
    const auto probe = [&]( const point &p ) -> std::optional<bool> {
        return loop.count( p ) > 0;
    };
    const auto a = a11y::location_detail::find_road_section( point( 0, 0 ), probe );
    const auto b = a11y::location_detail::find_road_section( point( 2, 2 ), probe );
    REQUIRE( a.complete );
    REQUIRE( b.complete );
    CHECK( a.tiles == loop );
    CHECK( b.tiles == loop );
}
