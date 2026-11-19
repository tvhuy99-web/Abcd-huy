#include <set>
#include <string>

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
