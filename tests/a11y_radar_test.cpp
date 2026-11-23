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

TEST_CASE( "a11y_radar_groups_long_furniture_without_counting_each_tile_as_an_object",
           "[a11y][radar]" )
{
    using a11y::radar_detail::tile_sample;
    using a11y::radar_detail::summarize_tile_groups;
    // A cupboard occupying three connected squares is one *spatial group*.
    const std::vector<tile_sample> long_cupboard{
        { "f_cupboard", "cupboard", 100, 200, 3, 0 },
        { "f_cupboard", "cupboard", 101, 200, 4, 0 },
        { "f_cupboard", "cupboard", 102, 200, 5, 0 }
    };
    const auto one = summarize_tile_groups( long_cupboard ).at( "cupboard" );
    CHECK( one.groups == 1 );
    CHECK( one.occupied_tiles == 3 );
    CHECK( one.nearest_distance == 3 );

    // Two separate cupboards are two groups even with the same spoken name.
    const std::vector<tile_sample> separate{
        { "f_cupboard", "cupboard", 100, 200, 2, 0 },
        { "f_cupboard", "cupboard", 104, 200, 6, 0 }
    };
    const auto two = summarize_tile_groups( separate ).at( "cupboard" );
    CHECK( two.groups == 2 );
    CHECK( two.occupied_tiles == 2 );

    // Adjacent map types which happen to share a translation remain distinct.
    // The map cannot identify separate same-type furniture touching directly.
    const std::vector<tile_sample> different_types{
        { "f_cupboard", "cupboard", 100, 200, 2, 0 },
        { "f_cupboard_variant", "cupboard", 101, 200, 3, 0 }
    };
    CHECK( summarize_tile_groups( different_types ).at( "cupboard" ).groups == 2 );
    const std::vector<tile_sample> diagonal{
        { "f_cupboard", "cupboard", 100, 200, 2, 0 },
        { "f_cupboard", "cupboard", 101, 201, 3, 1 }
    };
    CHECK( summarize_tile_groups( diagonal ).at( "cupboard" ).groups == 2 );
}

TEST_CASE( "a11y_radar_reports_nine_to_ten_as_a_total_change",
           "[a11y][radar]" )
{
    using namespace a11y::radar_detail;
    count_tracker counts;
    const count_key cupboard{ 7u, "cupboard" };
    const std::map<count_key, int> nine{ { cupboard, 9 } };
    const std::map<count_key, int> ten{ { cupboard, 10 } };
    CHECK( counts.changed( nine ).at( cupboard ) == 9 );
    CHECK( counts.changed( nine ).empty() );
    CHECK( counts.changed( ten ).at( cupboard ) == 10 );
    CHECK( counts.changed( ten ).empty() );
    CHECK( counts.changed( nine ).at( cupboard ) == 9 );
    counts.clear();
    CHECK( counts.changed( ten ).at( cupboard ) == 10 );
}

TEST_CASE( "a11y_radar_reports_door_south_when_walking_east_to_west",
           "[a11y][radar]" )
{
    using namespace a11y::radar_detail;
    entry_tracker range_entries;
    entry_tracker adjacent_entries;
    const std::set<std::string> door{ "door:100:201" };
    const std::set<std::string> no_adjacency;

    // The door was already discovered within eight squares.
    CHECK( range_entries.entering( door ) == door );
    CHECK( range_entries.entering( door ).empty() );
    CHECK_FALSE( cardinally_adjacent( 1, 1 ) ); // Southeast diagonal.
    CHECK( adjacent_entries.entering( no_adjacency ).empty() );

    // Walking west brings the same door directly south, one square away.
    CHECK( cardinally_adjacent( 0, 1 ) );
    CHECK( adjacent_entries.entering( door ) == door );
    CHECK( range_entries.entering( door ).empty() );
    // Standing beside the door must not repeat the alert.
    CHECK( adjacent_entries.entering( door ).empty() );

    // After passing it, returning to the doorway is a fresh approach.
    CHECK( adjacent_entries.entering( no_adjacency ).empty() );
    CHECK( adjacent_entries.entering( door ) == door );
}
