#include <set>
#include <string>

#include "a11y_radar.h"
#include "cata_catch.h"

TEST_CASE( "a11y_radar_announces_once_per_boundary_crossing",
           "[a11y][radar]" )
{
    a11y::radar_detail::entry_tracker radar;
    const std::set<std::string> outside;
    const std::set<std::string> at_eight{ "door:global:200:400" };
    // Door is nine tiles away: no alert. Enter eight: alert once.
    CHECK( radar.entering( outside ).empty() );
    CHECK( radar.entering( at_eight ) == at_eight );
    // Distance seven and then eight still belong to the same radius.
    CHECK( radar.entering( at_eight ).empty() );
    CHECK( radar.entering( at_eight ).empty() );
    // Retreat to nine, return to eight: one fresh notification.
    CHECK( radar.entering( outside ).empty() );
    CHECK( radar.entering( at_eight ) == at_eight );
    CHECK( radar.entering( at_eight ).empty() );
}

TEST_CASE( "a11y_radar_tracks_individual_objects_not_shared_names",
           "[a11y][radar]" )
{
    a11y::radar_detail::entry_tracker radar;
    const std::set<std::string> first{ "monster:123" };
    const std::set<std::string> both{ "monster:123", "monster:987" };
    CHECK( radar.entering( first ) == first );
    // Monster 123 moved within range: not new. Monster 987 entered: new.
    const std::set<std::string> second{ "monster:987" };
    CHECK( radar.entering( both ) == second );
    CHECK( radar.entering( both ).empty() );
    // Individually leaving and entering counts as a new crossing.
    CHECK( radar.entering( second ).empty() );
    CHECK( radar.entering( both ) == first );
    radar.clear();
    CHECK( radar.entering( both ) == both );
}

TEST_CASE( "a11y_radar_keeps_door_and_item_at_same_square_separate",
           "[a11y][radar]" )
{
    a11y::radar_detail::entry_tracker radar;
    const std::set<std::string> shared{ "door:1:2", "item:12345" };
    CHECK( radar.entering( shared ) == shared );
    const std::set<std::string> door_only{ "door:1:2" };
    CHECK( radar.entering( door_only ).empty() );
    const std::set<std::string> item_only{ "item:12345" };
    CHECK( radar.entering( shared ) == item_only );
}

TEST_CASE( "a11y_radar_reports_existing_object_state_changes_only_once",
           "[a11y][radar]" )
{
    a11y::radar_detail::change_tracker states;
    const std::map<std::string, std::string> closed{ { "door:10:20", "closed" } };
    const std::map<std::string, std::string> open{ { "door:10:20", "open" } };
    CHECK( states.changed( closed ).empty() );
    CHECK( states.changed( closed ).empty() );
    CHECK( states.changed( open ).at( "door:10:20" ) == "open" );
    CHECK( states.changed( open ).empty() );
    CHECK( states.changed( closed ).at( "door:10:20" ) == "closed" );
    states.clear();
    CHECK( states.changed( open ).empty() ); // No phantom state change on restart.
}

TEST_CASE( "a11y_radar_does_not_treat_entering_or_leaving_as_a_state_change",
           "[a11y][radar]" )
{
    a11y::radar_detail::change_tracker states;
    const std::map<std::string, std::string> first{ { "door:a", "open" } };
    const std::map<std::string, std::string> second{ { "door:b", "closed" } };
    CHECK( states.changed( first ).empty() );
    CHECK( states.changed( second ).empty() );
    CHECK( states.changed( first ).empty() );
}

TEST_CASE( "a11y_radar_distinguishes_new_passage_from_same_wall",
           "[a11y][radar]" )
{
    using namespace a11y::radar_detail;
    spatial_frame corridor;
    spatial_frame next;
    corridor.rays[2] = { true, true, 1, "wall:stable" };
    next.rays[2] = { true, true, 1, "wall:stable" };
    CHECK( compare_spatial( corridor, next ).empty() );

    // Walking toward the exact same wall is silent, regardless of distance.
    corridor.rays[2].distance = 6;
    next.rays[2].distance = 3;
    CHECK( compare_spatial( corridor, next ).empty() );

    // A different wall farther away means a newly opened side passage.
    corridor.rays[2] = { true, true, 1, "wall:old" };
    next.rays[2] = { true, true, 7, "wall:new" };
    const auto opening = compare_spatial( corridor, next );
    REQUIRE( opening.size() == 1 );
    CHECK( opening[0].kind == spatial_change_kind::passage_opens );
    CHECK( opening[0].direction == 2 );

    corridor.rays[2] = { true, false, 8, "" };
    next.rays[2] = { true, true, 2, "wall:near" };
    const auto narrowing = compare_spatial( corridor, next );
    REQUIRE( narrowing.size() == 1 );
    CHECK( narrowing[0].kind == spatial_change_kind::passage_narrows );
    CHECK( narrowing[0].distance == 2 );
}

TEST_CASE( "a11y_radar_announces_only_meaningful_connected_space_changes",
           "[a11y][radar]" )
{
    using namespace a11y::radar_detail;
    spatial_frame hall;
    spatial_frame room;
    hall.reachable_tiles = 18;
    room.reachable_tiles = 50;
    const auto opening = compare_spatial( hall, room );
    REQUIRE( opening.size() == 1 );
    CHECK( opening[0].kind == spatial_change_kind::area_opens );

    const auto closing = compare_spatial( room, hall );
    REQUIRE( closing.size() == 1 );
    CHECK( closing[0].kind == spatial_change_kind::area_narrows );

    spatial_frame similar = hall;
    similar.reachable_tiles = 23;
    CHECK( compare_spatial( hall, similar ).empty() );

    // Near the edge of the loaded map, incomplete rays must be silent.
    hall.rays[0] = { false, true, 1, "unloaded" };
    room.rays[0] = { true, false, 8, "" };
    CHECK( compare_spatial( hall, room ).size() == 1 );
}
