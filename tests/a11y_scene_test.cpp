#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include "a11y_scene.h"
#include "cata_catch.h"
#include "point.h"

TEST_CASE( "a11y_place_survey_uses_whole_overmap_envelope_not_one_24_tile",
           "[a11y][scene]" )
{
    const a11y::scene_detail::extent site =
        a11y::scene_detail::overmap_envelope( 125, 400, 127, 401 );

    // Three adjoining OMT columns and two rows form one 72x48 frame.
    CHECK( site.west == 3000 );
    CHECK( site.north == 9600 );
    CHECK( site.east == 3071 );
    CHECK( site.south == 9647 );
    CHECK( site.width() == 72 );
    CHECK( site.height() == 48 );

    const a11y::scene_detail::extent negative =
        a11y::scene_detail::overmap_envelope( -3, -2, -2, -1 );
    CHECK( negative.west == -72 );
    CHECK( negative.north == -48 );
    CHECK( negative.width() == 48 );
    CHECK( negative.height() == 48 );
}

TEST_CASE( "a11y_place_survey_reports_world_compass_not_avatar_facing",
           "[a11y][scene]" )
{
    // Every compass heading must have a distinct label (in the active
    // language). No facing state or map visibility is consulted here.
    const std::set<std::string> directions = {
        a11y::scene_detail::compass_direction( point( 0, -1 ) ),
        a11y::scene_detail::compass_direction( point( 1, -1 ) ),
        a11y::scene_detail::compass_direction( point( 1, 0 ) ),
        a11y::scene_detail::compass_direction( point( 1, 1 ) ),
        a11y::scene_detail::compass_direction( point( 0, 1 ) ),
        a11y::scene_detail::compass_direction( point( -1, 1 ) ),
        a11y::scene_detail::compass_direction( point( -1, 0 ) ),
        a11y::scene_detail::compass_direction( point( -1, -1 ) ),
        a11y::scene_detail::compass_direction( point( 0, 0 ) )
    };
    CHECK( directions.size() == 9 );
}

TEST_CASE( "a11y_place_survey_loaded_area_does_not_follow_an_arbitrary_avatar_radius",
           "[a11y][scene]" )
{
    // The avatar may be very near one edge of the loaded map. The survey
    // still has to include all other loaded cells, not only radius 45 or 70.
    const auto bounds = a11y::scene_detail::loaded_envelope( 3000, 4000, 132, 132 );
    CHECK( bounds.west == 3000 );
    CHECK( bounds.north == 4000 );
    CHECK( bounds.east == 3131 );
    CHECK( bounds.south == 4131 );
    CHECK( bounds.width() == 132 );
    CHECK( bounds.height() == 132 );

    const auto negative = a11y::scene_detail::loaded_envelope( -72, -108, 12, 24 );
    CHECK( negative.west == -72 );
    CHECK( negative.south == -85 );
}

TEST_CASE( "a11y_place_survey_groups_connected_window_squares_once",
           "[a11y][scene]" )
{
    // Three adjoining positions are one physical window; another window
    // across a gap and diagonal touches remain independent.
    const std::vector<point> squares{
        point( 22, 6 ), point( 22, 8 ), point( 22, 7 ),
        point( 3, 7 ), point( 5, 5 ), point( 6, 6 )
    };
    const std::vector<point> representatives =
        a11y::scene_detail::representative_tiles( squares, point( 22, 7 ) );
    CHECK( representatives.size() == 4 );
    CHECK( std::find( representatives.begin(), representatives.end(),
                      point( 22, 7 ) ) != representatives.end() );
    CHECK( std::find( representatives.begin(), representatives.end(),
                      point( 3, 7 ) ) != representatives.end() );
    CHECK( std::find( representatives.begin(), representatives.end(),
                      point( 5, 5 ) ) != representatives.end() );
    CHECK( std::find( representatives.begin(), representatives.end(),
                      point( 6, 6 ) ) != representatives.end() );
}

TEST_CASE( "a11y_place_survey_connected_group_representative_is_stable",
           "[a11y][scene]" )
{
    // A repeated survey of the same place picks the same near tile even
    // when the map's enumeration order changes.
    const std::vector<point> forward{ point( 22, 8 ), point( 22, 7 ), point( 22, 6 ) };
    const std::vector<point> reverse{ point( 22, 6 ), point( 22, 7 ), point( 22, 8 ) };
    CHECK( a11y::scene_detail::representative_tiles( forward, point( 22, 7 ) ) ==
           a11y::scene_detail::representative_tiles( reverse, point( 22, 7 ) ) );
}
