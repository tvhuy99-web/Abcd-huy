#include "avatar.h"
#include "cata_catch.h"
#include "character.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_autodrive_tiles.h"
#include "vpart_position.h"

// Both native A* and the accessible obstacle guard must call this SAME
// classifier, never duplicate collision rules in the reader.
TEST_CASE( "vehicle_driving_uses_shared_tile_classifier", "[vehicle][a11y][autodrive]" )
{
    clear_avatar();
    clear_map_without_vision();
    map &here = get_map();
    Character &driver = get_player_character();
    const tripoint_bub_ms origin( 60, 60, 0 );
    driver.setpos( here, origin );
    vehicle *car = here.add_vehicle( vproto_id( "car" ), origin, -90_degrees, 0,
                                     veh_spawn_status::UNDAMAGED );
    REQUIRE( car != nullptr );

    const tripoint_bub_ms target( 74, 60, 0 );
    REQUIRE( here.inbounds( target ) );
    const vehicle_autodrive::mobility_profile road{ true, false, false, true };
    const vehicle_autodrive::mobility_profile boat{ false, true, false, true };
    const vehicle_autodrive::mobility_profile rotor{ false, false, true, true };
    const auto classify = [&]( const vehicle_autodrive::mobility_profile &profile ) {
        return vehicle_autodrive::classify_tile( *car, here, target, driver, profile );
    };
    using vehicle_autodrive::tile_blocker;

    SECTION( "normal_pavement_sidewalks_and_dirt_are_not_obstacles" ) {
        for( const char *terrain : { "t_pavement", "t_pavement_y", "t_zebra",
                                     "t_sidewalk", "t_dirt" } ) {
            REQUIRE( here.ter_set( target, ter_str_id( terrain ).id() ) );
            CHECK( classify( road ) == tile_blocker::none );
        }
    }

    SECTION( "walls_still_stop_a_vehicle" ) {
        REQUIRE( here.ter_set( target, ter_str_id( "t_wall" ).id() ) );
        CHECK( classify( road ) == tile_blocker::terrain );
    }

    SECTION( "water_and_air_are_vehicle_capability_dependent" ) {
        REQUIRE( here.ter_set( target, ter_str_id( "t_water_sh" ).id() ) );
        CHECK( classify( road ) == tile_blocker::terrain );
        CHECK( classify( boat ) == tile_blocker::none );

        REQUIRE( here.ter_set( target, ter_str_id( "t_open_air" ).id() ) );
        CHECK( classify( road ) == tile_blocker::terrain );
        CHECK( classify( rotor ) == tile_blocker::none );
    }

    SECTION( "furniture_and_visible_traps_block_but_loose_items_do_not" ) {
        REQUIRE( here.ter_set( target, ter_str_id( "t_pavement" ).id() ) );
        REQUIRE( here.furn_set( target, furn_str_id( "f_bench" ).id() ) );
        CHECK( classify( road ) == tile_blocker::furniture );

        REQUIRE( here.furn_set( target, furn_str_id::NULL_ID().id() ) );
        here.add_item_or_charges( target, item( itype_id( "rock" ) ) );
        // Loose rocks do not obstruct the real vehicle A* footprint.
        // They must not trigger phantom emergency braking.
        CHECK( classify( road ) == tile_blocker::none );

        here.i_clear( target );
        here.trap_set( target, trap_str_id( "tr_bubblewrap" ).id() );
        REQUIRE( here.can_see_trap_at( target, driver ) );
        CHECK( classify( road ) == tile_blocker::visible_trap );
    }

    SECTION( "live_creatures_are_blockers_for_accessibility_sensors" ) {
        REQUIRE( here.ter_set( target, ter_str_id( "t_pavement" ).id() ) );
        spawn_test_monster( "mon_zombie", target );
        CHECK( classify( road ) == tile_blocker::creature );
    }

    SECTION( "own_vehicle_and_other_vehicles_are_distinguished" ) {
        const tripoint_bub_ms own_tile = car->bub_part_pos( here, 0 );
        CHECK( vehicle_autodrive::classify_tile( *car, here, own_tile, driver, road ) ==
               tile_blocker::none );

        vehicle *other = here.add_vehicle( vproto_id( "bicycle" ), target, -90_degrees, 0,
                                           veh_spawn_status::UNDAMAGED );
        REQUIRE( other != nullptr );
        REQUIRE( static_cast<bool>( here.veh_at( target ) ) );
        CHECK( classify( road ) == tile_blocker::other_vehicle );
    }

    SECTION( "map_bubble_boundary_is_not_a_physical_collision" ) {
        const tripoint_bub_ms outside( -1, -1, 0 );
        REQUIRE_FALSE( here.inbounds( outside ) );
        CHECK( vehicle_autodrive::classify_tile( *car, here, outside, driver, road ) ==
               tile_blocker::map_boundary );
    }
}
