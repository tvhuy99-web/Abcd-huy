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
