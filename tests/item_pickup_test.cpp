#include <climits>
#include <functional>
#include <list>
#include <optional>
#include <string>
#include <utility>

#include "avatar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "debug.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "rng.h"
#include "type_id.h"

static const itype_id itype_backpack_hiking( "backpack_hiking" );
static const itype_id itype_debug_modular_m4_carbine( "debug_modular_m4_carbine" );
static const itype_id itype_nail( "nail" );
static const itype_id itype_rope_6( "rope_6" );











TEST_CASE( "putting_items_into_inventory_with_put_in_or_i_add", "[pickup][inventory]" )
{
    avatar &they = get_avatar();
    map &here = get_map();
    clear_avatar();
    clear_map_without_vision();


    const tripoint_bub_ms ground = they.pos_bub();
    item &rope_map = here.add_item( ground, item( itype_rope_6 ) );
    item &backpack_map = here.add_item( ground, item( itype_backpack_hiking ) );


    std::string backpack_uid = random_string( 10 );
    std::string rope_uid = random_string( 10 );
    backpack_map.set_var( "uid", backpack_uid );
    rope_map.set_var( "uid", rope_uid );


    REQUIRE_FALSE( they.has_item( backpack_map ) );
    REQUIRE_FALSE( they.has_item( rope_map ) );
    REQUIRE_FALSE( character_has_item_with_var_val( they, "uid", backpack_uid ) );
    REQUIRE_FALSE( character_has_item_with_var_val( they, "uid", rope_uid ) );

    WHEN( "avatar wears a hiking backpack from the ground with wear_item" ) {
        they.clear_worn();


        std::optional<std::list<item>::iterator> worn = they.wear_item( backpack_map );
        item &backpack = **worn;
        item_location backpack_loc( they, &backpack );

        THEN( "they have a copy of the backpack" ) {

            CHECK( backpack.typeId() == backpack_map.typeId() );

            CHECK( backpack.get_var( "uid" ) == backpack_uid );
            CHECK( character_has_item_with_var_val( they, "uid", backpack_uid ) );

            CHECK( they.has_item( backpack ) );

            CHECK_FALSE( they.has_item( backpack_map ) );
        }

        WHEN( "using put_in to put a rope directly into the backpack" ) {
            REQUIRE( backpack.put_in( rope_map, pocket_type::CONTAINER ).success() );

            THEN( "the original rope is not in inventory or the backpack" ) {
                CHECK_FALSE( they.has_item( rope_map ) );
                CHECK_FALSE( backpack_loc.has_item( rope_map ) );
            }
            THEN( "they have a copy of the rope in inventory" ) {
                CHECK( character_has_item_with_var_val( they, "uid", rope_uid ) );
            }

        }

        WHEN( "using i_add to put the rope into inventory" ) {

            item_location rope_new = they.i_add( rope_map );

            THEN( "a copy of the rope item is in inventory and in the backpack" ) {
                CHECK( they.has_item( *rope_new ) );
                CHECK( backpack_loc.has_item( *rope_new ) );
                CHECK( character_has_item_with_var_val( they, "uid", rope_uid ) );
            }
            THEN( "the original rope is not in inventory or the backpack" ) {
                CHECK_FALSE( they.has_item( rope_map ) );
                CHECK_FALSE( backpack_loc.has_item( rope_map ) );
            }
        }
    }





    GIVEN( "avatar is not wearing anything that can store items" ) {
        they.clear_worn();

        WHEN( "avatar tries to get the backpack with pick_up" ) {
            item_location backpack_loc( map_cursor( tripoint_bub_ms( ground ) ), &backpack_map );
            const drop_locations &pack_droplocs = { std::make_pair( backpack_loc, 1 ) };
            they.pick_up( pack_droplocs );
            process_activity( they );

            THEN( "they fail to acquire the backpack" ) {
                CHECK_FALSE( character_has_item_with_var_val( they, "uid", backpack_uid ) );
            }
        }

        WHEN( "avatar tries to get the rope with pick_up" ) {
            item_location rope_loc( map_cursor( tripoint_bub_ms( ground ) ), &rope_map );
            const drop_locations &rope_droplocs = { std::make_pair( rope_loc, 1 ) };
            they.pick_up( rope_droplocs );
            process_activity( they );

            THEN( "they fail to acquire the rope" ) {
                CHECK_FALSE( character_has_item_with_var_val( they, "uid", rope_uid ) );
            }
        }
    }
}











TEST_CASE( "pickup_m4_with_a_rope_in_a_hiking_backpack", "[pickup][container]" )
{
    avatar &they = get_avatar();
    map &here = get_map();
    clear_avatar();
    clear_map_without_vision();


    const tripoint_bub_ms ground = they.pos_bub();
    item &m4a1 = here.add_item( ground, item( itype_debug_modular_m4_carbine ) );
    item &rope_map = here.add_item( ground, item( itype_rope_6 ) );
    item &backpack_map = here.add_item( ground, item( itype_backpack_hiking ) );


    REQUIRE( rope_map.is_container() );
    REQUIRE( backpack_map.is_container() );
    REQUIRE( rope_map.can_contain( m4a1 ).success() );
    REQUIRE( backpack_map.can_contain( m4a1 ).success() );
    REQUIRE( backpack_map.can_contain( rope_map ).success() );


    std::string m4_uid = random_string( 10 );
    std::string rope_uid = random_string( 10 );
    m4a1.set_var( "uid", m4_uid );
    rope_map.set_var( "uid", rope_uid );

    GIVEN( "avatar is wearing a backpack with a short rope in it" ) {

        CAPTURE( here.i_at( ground ).size() );

        std::optional<std::list<item>::iterator> worn = they.wear_item( backpack_map );
        item &backpack = **worn;
        item_location backpack_loc( they, &backpack );
        REQUIRE( they.has_item( backpack ) );

        item_location rope = they.i_add( rope_map );
        REQUIRE( they.has_item( *rope ) );

        WHEN( "they pick up the M4" ) {

            item_location m4_loc( map_cursor( they.pos_abs() ), &m4a1 );
            const drop_locations &thing = { std::make_pair( m4_loc, 1 ) };
            CHECK_FALSE( backpack_loc.has_item( m4a1 ) );

            they.pick_up( thing );
            process_activity( they );


            THEN( "they should have the rope and M4 still in possession" ) {
                CHECK( character_has_item_with_var_val( they, "uid", rope_uid ) );
                CHECK( character_has_item_with_var_val( they, "uid", m4_uid ) );
            }
        }
    }
}


static int ground_charges_of( map &here, const tripoint_bub_ms &pos,
                              const itype_id &type )
{
    int total = 0;
    for( const item &it : here.i_at( pos ) ) {
        if( it.typeId() == type ) {
            total += it.charges;
        }
    }
    return total;
}




TEST_CASE( "charged_item_pickup_full_merge_no_debugmsg", "[pickup][charges]" )
{
    avatar &they = get_avatar();
    map &here = get_map();
    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms ground = they.pos_bub();


    item backpack_on_ground( itype_backpack_hiking );
    they.wear_item( backpack_on_ground );


    item seed_nails( itype_nail );
    seed_nails.charges = 10;
    they.i_add( seed_nails );

    const int inv_before = they.charges_of( itype_nail );
    REQUIRE( inv_before == 10 );


    they.cache_has_item_with( itype_nail );


    item ground_nails( itype_nail );
    ground_nails.charges = 5;
    here.add_item( ground, ground_nails );

    const int ground_before = ground_charges_of( here, ground, itype_nail );
    REQUIRE( ground_before == 5 );
    const int total_before = inv_before + ground_before;


    item_location nail_loc( map_cursor( they.pos_abs() ),
                            &here.i_at( ground ).only_item() );
    const drop_locations to_pick_up = { std::make_pair( nail_loc, 0 ) };

    std::string dmsg = capture_debugmsg_during( [&]() {
        they.pick_up( to_pick_up );
        process_activity( they );
    } );

    const int inv_after = they.charges_of( itype_nail );
    const int ground_after = ground_charges_of( here, ground, itype_nail );

    CHECK( dmsg.empty() );
    CHECK( inv_after + ground_after == total_before );
    CHECK( inv_after == 15 );
    CHECK( ground_after == 0 );
}

TEST_CASE( "charged_item_pickup_partial_merge_no_debugmsg", "[pickup][charges]" )
{
    avatar &they = get_avatar();
    map &here = get_map();
    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms ground = they.pos_bub();


    item backpack_on_ground( itype_backpack_hiking );
    they.wear_item( backpack_on_ground );


    item probe_nail( itype_nail );
    probe_nail.charges = 1;
    int remaining = INT_MAX;
    they.can_stash_partial( probe_nail, remaining );
    const int total_capacity = INT_MAX - remaining;
    REQUIRE( total_capacity > 100 );


    const int margin = 50;
    item prefill_nails( itype_nail );
    prefill_nails.charges = total_capacity - margin;
    they.i_add_or_fill( prefill_nails, true, nullptr, nullptr,
                        false, false );

    const int inv_before = they.charges_of( itype_nail );
    REQUIRE( inv_before > 0 );


    they.cache_has_item_with( itype_nail );


    int post_fill_remaining = INT_MAX;
    they.can_stash_partial( probe_nail, post_fill_remaining );
    const int can_still_fit = INT_MAX - post_fill_remaining;


    const int ground_count = can_still_fit + 50;
    REQUIRE( can_still_fit > 0 );
    REQUIRE( ground_count > can_still_fit );
    const int expected_transferred = can_still_fit;
    const int expected_remaining = ground_count - expected_transferred;

    item ground_nails( itype_nail );
    ground_nails.charges = ground_count;
    here.add_item( ground, ground_nails );

    const int ground_before = ground_charges_of( here, ground, itype_nail );
    REQUIRE( ground_before == ground_count );


    item_location nail_loc( map_cursor( they.pos_abs() ),
                            &here.i_at( ground ).only_item() );
    const drop_locations to_pick_up = { std::make_pair( nail_loc, 0 ) };

    std::string dmsg = capture_debugmsg_during( [&]() {
        they.pick_up( to_pick_up );
        process_activity( they );
    } );

    const int inv_after = they.charges_of( itype_nail );
    const int ground_after = ground_charges_of( here, ground, itype_nail );

    CHECK( dmsg.empty() );
    CHECK( inv_after == inv_before + expected_transferred );
    CHECK( ground_after == expected_remaining );
}
