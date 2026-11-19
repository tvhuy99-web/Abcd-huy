#include "avatar.h"
#include "cached_options.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "coordinates.h"
#include "handle_liquid.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"

static const itype_id itype_backpack( "backpack" );

static const itype_id itype_test_liquid_1ml( "test_liquid_1ml" );
static const itype_id
itype_test_watertight_open_sealed_container_250ml( "test_watertight_open_sealed_container_250ml" );





TEST_CASE( "handle_liquid_returns_false_when_target_container_is_full",
           "[liquid][handler]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms player_pos = dummy.pos_bub();
    const tripoint_bub_ms container_pos = player_pos + tripoint::east;


    item container( itype_test_watertight_open_sealed_container_250ml );
    item filler( itype_test_liquid_1ml );
    filler.charges = 250;
    REQUIRE( container.fill_with( filler, 250 ) > 0 );

    REQUIRE( container.get_remaining_capacity_for_liquid( item( itype_test_liquid_1ml ), true ) == 0 );


    here.add_item_or_charges( container_pos, container );
    map_stack items_at = here.i_at( container_pos );
    REQUIRE( !items_at.empty() );
    item_location cont_loc( map_cursor( container_pos ), &items_at.only_item() );
    REQUIRE( cont_loc.get_item() != nullptr );


    item liquid( itype_test_liquid_1ml );
    liquid.charges = 10;


    liquid_dest_opt target;
    target.dest_opt = LD_ITEM;
    target.item_loc = cont_loc;

    const int moves_before = dummy.get_moves();
    const int charges_before = liquid.charges;

    bool result = liquid_handler::handle_liquid( liquid, target, nullptr, 0 );

    CHECK_FALSE( result );
    CHECK( liquid.charges == charges_before );
    CHECK( dummy.get_moves() == moves_before );
}




TEST_CASE( "on_pickup_spillable_container_handles_all_liquid",
           "[liquid][spill]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    dummy.clear_worn();
    dummy.i_add( item( itype_backpack ) );
    clear_map_without_vision();

    restore_on_out_of_scope<test_mode_spilling_action_t> restore_spill( test_mode_spilling_action );
    test_mode_spilling_action = test_mode_spilling_action_t::spill_all;


    item container( itype_test_watertight_open_sealed_container_250ml );
    item liquid( itype_test_liquid_1ml );
    liquid.charges = 5;
    REQUIRE( container.put_in( liquid, pocket_type::CONTAINER ).success() );
    REQUIRE( container.is_bucket_nonempty() );

    item_location container_loc = dummy.i_add( container, true, nullptr, nullptr, false, false );
    container_loc->set_owner( dummy );

    container_loc->on_pickup( dummy );


    CHECK( container_loc->is_container_empty() );


    bool found_liquid = false;
    for( const item &ground_item : here.i_at( dummy.pos_bub() ) ) {
        if( ground_item.typeId() == itype_test_liquid_1ml ) {
            found_liquid = true;
            break;
        }
    }
    CHECK( found_liquid );
}
