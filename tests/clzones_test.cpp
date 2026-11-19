#include <climits>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <list>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "activity_actor.h"
#include "activity_actor_definitions.h"
#include "activity_item_handling.h"
#include "avatar.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "character_attire.h"
#include "clone_ptr.h"
#include "clzones.h"
#include "coordinates.h"
#include "enums.h"
#include "game.h"
#include "game_constants.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "map_selector.h"
#include "npc.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "visitable.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const activity_id ACT_MOVE_LOOT( "ACT_MOVE_LOOT" );

static const faction_id faction_your_followers( "your_followers" );

static const itype_id itype_556( "556" );
static const itype_id itype_ammolink223( "ammolink223" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_belt223( "belt223" );
static const itype_id itype_bottle_glass( "bottle_glass" );
static const itype_id itype_chem_washing_soda( "chem_washing_soda" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_test_apple( "test_apple" );
static const itype_id itype_test_bitter_almond( "test_bitter_almond" );
static const itype_id itype_test_heavy_boulder( "test_heavy_boulder" );
static const itype_id itype_test_liquid_1ml( "test_liquid_1ml" );
static const itype_id itype_test_milk( "test_milk" );
static const itype_id itype_test_rod_14cm( "test_rod_14cm" );
static const itype_id
itype_test_watertight_open_sealed_container_250ml( "test_watertight_open_sealed_container_250ml" );
static const itype_id itype_test_wine( "test_wine" );

static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_wall( "t_wall" );

static const vproto_id vehicle_prototype_test_shopping_cart( "test_shopping_cart" );
static const vproto_id vehicle_prototype_test_turret_rig( "test_turret_rig" );

static const zone_type_id zone_type_LOOT_CHEMICAL( "LOOT_CHEMICAL" );
static const zone_type_id zone_type_LOOT_CUSTOM( "LOOT_CUSTOM" );
static const zone_type_id zone_type_LOOT_DEFAULT( "LOOT_DEFAULT" );
static const zone_type_id zone_type_LOOT_DRINK( "LOOT_DRINK" );
static const zone_type_id zone_type_LOOT_FOOD( "LOOT_FOOD" );
static const zone_type_id zone_type_LOOT_PDRINK( "LOOT_PDRINK" );
static const zone_type_id zone_type_LOOT_PFOOD( "LOOT_PFOOD" );
static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );
static const zone_type_id zone_type_UNLOAD_ALL( "UNLOAD_ALL" );
static const zone_type_id zone_type_VEHICLE_REPAIR( "VEHICLE_REPAIR" );

namespace
{
template <class T>
int _count_items_or_charges( const T &items, const itype_id &id )
{
    int n = 0;
    for( const item &it : items ) {
        if( it.typeId() == id ) {
            n += it.count();
        }
    }
    return n;
}

int count_items_or_charges( const tripoint_bub_ms src, const itype_id &id,
                            const std::optional<vpart_reference> &vp )
{
    if( vp ) {
        return _count_items_or_charges( vp->vehicle().get_items( vp->part() ), id );
    }
    return _count_items_or_charges( get_map().i_at( src ), id );
}

void create_tile_zone( const std::string &name, const zone_type_id &zone_type, tripoint_abs_ms pos,
                       bool veh = false )
{
    zone_manager &zm = zone_manager::get_manager();
    zm.add( name, zone_type, faction_your_followers, false, true, pos, pos, nullptr, veh );
}

}

static bool find_liquid_container( const tripoint_bub_ms &start_pos )
{
    bool found_container_with_liquid = false;
    map_cursor( start_pos ).visit_items(
    [&found_container_with_liquid]( item_location it ) {
        if( it->typeId() == itype_test_watertight_open_sealed_container_250ml ) {
            found_container_with_liquid = it.has_item_with( []( const item & nested ) {
                return nested.typeId() == itype_test_liquid_1ml;
            } );
        }
        return VisitResponse::SKIP;
    }
    );
    return found_container_with_liquid;
}

TEST_CASE( "zone_unloading_ammo_belts", "[zones][items][ammo_belt][activities][unload]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();
    std::optional<vpart_reference> vp;
    bool const in_vehicle = GENERATE( false, true );
    CAPTURE( in_vehicle );

    clear_avatar();
    clear_map_without_vision();

    tripoint_abs_ms const start = here.get_abs( tripoint_bub_ms::zero + tripoint::east );
    dummy.set_pos_abs_only( start );

    if( in_vehicle ) {
        REQUIRE( here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                   tripoint_bub_ms::zero + tripoint::east, 0_degrees, 0, veh_spawn_status::UNDAMAGED ) );
        vp = here.veh_at( start ).cargo();
        REQUIRE( vp );
        vp->vehicle().set_owner( dummy );
    }

    create_tile_zone( "Unload All", zone_type_UNLOAD_ALL, start, in_vehicle );

    item ammo_belt = item( itype_belt223, calendar::turn );
    ammo_belt.ammo_set( ammo_belt.ammo_default() );
    int belt_ammo_count_before_unload = ammo_belt.ammo_remaining( );

    REQUIRE( belt_ammo_count_before_unload > 0 );

    WHEN( "unloading ammo belts using UNLOAD_ALL " ) {
        if( in_vehicle ) {
            vp->vehicle().add_item( here, vp->part(), ammo_belt );
        } else {
            here.add_item_or_charges( tripoint_bub_ms( tripoint::east ), ammo_belt );
        }
        dummy.assign_activity( unload_loot_activity_actor() );
        CAPTURE( dummy.activity.id() );
        process_activity( dummy );

        THEN( "check that the ammo and linkages are both unloaded and the ammo belt is removed" ) {
            CHECK( count_items_or_charges( tripoint_bub_ms::zero + tripoint::east, itype_belt223, vp ) == 0 );
            CHECK( count_items_or_charges( tripoint_bub_ms::zero + tripoint::east,
                                           itype_ammolink223, vp ) == belt_ammo_count_before_unload );
            CHECK( count_items_or_charges( tripoint_bub_ms::zero + tripoint::east, itype_556,
                                           vp ) == belt_ammo_count_before_unload );
        }
    }
}




TEST_CASE( "zone_sorting_comestibles_", "[zones][items][food][activities]" )
{
    clear_map_without_vision();
    zone_manager &zm = zone_manager::get_manager();

    const tripoint_abs_ms origin_pos;
    create_tile_zone( "Food", zone_type_LOOT_FOOD, tripoint_abs_ms::zero + tripoint::east );
    create_tile_zone( "Drink", zone_type_LOOT_DRINK, tripoint_abs_ms::zero + tripoint::west );

    SECTION( "without perishable zones" ) {
        GIVEN( "a non-perishable food" ) {
            item nonperishable_food( itype_test_bitter_almond );
            REQUIRE_FALSE( nonperishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( nonperishable_food, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a non-perishable drink" ) {
            item nonperishable_drink( itype_test_wine );
            REQUIRE_FALSE( nonperishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( nonperishable_drink, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }
        }

        GIVEN( "a perishable food" ) {
            item perishable_food( itype_test_apple );
            REQUIRE( perishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( perishable_food, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a perishable drink" ) {
            item perishable_drink( itype_test_milk );
            REQUIRE( perishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( perishable_drink, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }
        }
    }

    SECTION( "with perishable zones" ) {
        create_tile_zone( "PFood", zone_type_LOOT_PFOOD, tripoint_abs_ms::zero + tripoint::north );
        create_tile_zone( "PDrink", zone_type_LOOT_PDRINK, tripoint_abs_ms::zero + tripoint::south );

        GIVEN( "a non-perishable food" ) {
            item nonperishable_food( itype_test_bitter_almond );
            REQUIRE_FALSE( nonperishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( nonperishable_food, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( nonperishable_food, pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( nonperishable_food, pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_container_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a non-perishable drink" ) {
            item nonperishable_drink( itype_test_wine );
            REQUIRE_FALSE( nonperishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( nonperishable_drink, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( nonperishable_drink, pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( nonperishable_drink, pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_container_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }
        }

        GIVEN( "a perishable food" ) {
            item perishable_food( itype_test_apple );
            REQUIRE( perishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the perishable food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( perishable_food, origin_pos ) == zone_type_LOOT_PFOOD );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( perishable_food, pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the perishable food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_PFOOD );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( perishable_food, pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_container_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a perishable drink" ) {
            item perishable_drink( itype_test_milk );
            REQUIRE( perishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the perishable drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( perishable_drink, origin_pos ) == zone_type_LOOT_PDRINK );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( perishable_drink, pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the perishable drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_PDRINK );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( perishable_drink, pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_container_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }
        }
    }
}




TEST_CASE( "zone_sorting_skips_items_with_unreachable_destinations",
           "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();


    const tripoint_bub_ms start_pos = tripoint_bub_ms::zero + tripoint::east;
    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    dummy.set_pos_abs_only( start_abs );


    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );


    here.add_item_or_charges( start_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( start_pos, itype_test_apple, std::nullopt ) == 1 );


    const tripoint_bub_ms dest_pos = start_pos + tripoint( 5, 0, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );


    for( int dx = -1; dx <= 1; dx++ ) {
        for( int dy = -1; dy <= 1; dy++ ) {
            if( dx == 0 && dy == 0 ) {
                continue;
            }
            here.ter_set( dest_pos + tripoint( dx, dy, 0 ), ter_t_wall );
        }
    }
    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    item test_food( itype_test_apple );
    zone_manager &zm = zone_manager::get_manager();
    REQUIRE( zm.get_near_zone_type_for_item( test_food, start_abs ) == zone_type_LOOT_FOOD );


    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( start_pos, itype_test_apple, std::nullopt ) == 1 );

    CHECK( dummy.charges_of( itype_test_apple ) == 0 );

    CHECK( !dummy.activity );
}


TEST_CASE( "zone_sorting_skips_source_when_all_destinations_count_full",
           "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();
    dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );

    const tripoint_bub_ms src_pos = start_pos;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );

    std::optional<vpart_reference> dest_vp;

    SECTION( "vehicle cargo destination at MAX_ITEM_IN_VEHICLE_STORAGE" ) {
        vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                          dest_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
        REQUIRE( cart != nullptr );
        cart->set_owner( dummy );
        dest_vp = here.veh_at( dest_pos ).cargo();
        REQUIRE( dest_vp );

        const int cargo_fill_target = MAX_ITEM_IN_VEHICLE_STORAGE;
        int inserted = 0;
        for( int i = 0; i < cargo_fill_target; ++i ) {
            item filler( itype_test_rod_14cm );
            if( dest_vp->vehicle().add_item( here, dest_vp->part(), filler ) ) {
                ++inserted;
            } else {
                break;
            }
        }
        REQUIRE( inserted >= cargo_fill_target - 1 );

        create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs, true );
        REQUIRE( zone_manager::get_manager().has_vehicle( zone_type_LOOT_FOOD, dest_abs ) );
    }

    SECTION( "ground destination at MAX_ITEM_IN_SQUARE" ) {
        int inserted = 0;
        for( int i = 0; i < MAX_ITEM_IN_SQUARE; ++i ) {
            item *added = here.add_item_or_charges_ret_loc( dest_pos, item( itype_test_rod_14cm ),
                          false ).get_item();
            if( added == nullptr ) {
                break;
            }
            ++inserted;
        }
        REQUIRE( inserted >= MAX_ITEM_IN_SQUARE - 1 );

        create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );
    }

    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    REQUIRE( zone_manager::get_manager().get_near_zone_type_for_item(
                 item( itype_test_apple ), src_abs ) == zone_type_LOOT_FOOD );

    bool pickup_failure = false;
    bool spillable_skipped = false;
    zone_sorting::zone_items items = zone_sorting::populate_items( src_pos );
    zone_sorting::unload_sort_options opts = zone_sorting::set_unload_options( dummy, src_abs,
            false );
    std::vector<item_location> other_activity;
    CHECK_FALSE( zone_sorting::has_items_to_sort( dummy, src_abs, opts, other_activity, items,
                 &pickup_failure, &spillable_skipped ) );
}


TEST_CASE( "zone_sorting_activity_terminates_with_count_full_vehicle_destination",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();
    dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );

    const tripoint_bub_ms src_pos = start_pos;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      dest_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    std::optional<vpart_reference> vp = here.veh_at( dest_pos ).cargo();
    REQUIRE( vp );

    int inserted = 0;
    for( int i = 0; i < MAX_ITEM_IN_VEHICLE_STORAGE; ++i ) {
        item filler( itype_test_rod_14cm );
        if( vp->vehicle().add_item( here, vp->part(), filler ) ) {
            ++inserted;
        } else {
            break;
        }
    }
    REQUIRE( inserted >= MAX_ITEM_IN_VEHICLE_STORAGE - 1 );

    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs, true );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );

    const int max_turns = 500;
    int turns = 0;
    while( dummy.activity && turns < max_turns ) {
        dummy.mod_moves( dummy.get_speed() );
        while( dummy.get_moves() > 0 && dummy.activity ) {
            dummy.activity.do_turn( dummy );
        }
        ++turns;
    }

    CHECK( !dummy.activity );
    CHECK( turns < max_turns );
}





TEST_CASE( "zone_sorting_with_grabbed_single_tile_vehicle",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();


    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms cart_pos = start_pos + tripoint::east;
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      cart_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::east );
    REQUIRE( dummy.get_grab_type() == object_type::VEHICLE );


    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );


    here.add_item_or_charges( start_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( start_pos, itype_test_apple, std::nullopt ) == 1 );


    const tripoint_bub_ms dest_pos = start_pos + tripoint( 0, 5, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    zone_manager &zm = zone_manager::get_manager();
    item test_food( itype_test_apple );
    REQUIRE( zm.get_near_zone_type_for_item( test_food, start_abs ) == zone_type_LOOT_FOOD );


    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );





    CHECK( count_items_or_charges( start_pos, itype_test_apple, std::nullopt ) == 0 );
}



TEST_CASE( "zone_sorting_with_grabbed_multi_tile_vehicle",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();


    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms veh_pos = start_pos + tripoint::east;
    vehicle *veh = here.add_vehicle( vehicle_prototype_test_turret_rig,
                                     veh_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );
    REQUIRE( veh->get_points().size() > 1 );
    veh->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::east );
    REQUIRE( dummy.get_grab_type() == object_type::VEHICLE );


    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );
    here.add_item_or_charges( start_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( start_pos, itype_test_apple, std::nullopt ) == 1 );


    const tripoint_bub_ms dest_pos = start_pos + tripoint( 0, 5, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    zone_manager &zm = zone_manager::get_manager();
    item test_food( itype_test_apple );
    REQUIRE( zm.get_near_zone_type_for_item( test_food, start_abs ) == zone_type_LOOT_FOOD );





    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( start_pos, itype_test_apple, std::nullopt ) == 0 );
}



TEST_CASE( "zone_sorting_cart_on_source_full_inventory",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );


    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );
    REQUIRE( dummy.get_grab_type() == object_type::VEHICLE );


    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );

    const tripoint_bub_ms dest_pos = start_pos + tripoint( 5, 0, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );



    for( int i = 0; i < 500 && dummy.can_add( item( itype_test_apple ) ); i++ ) {
        dummy.i_add( item( itype_test_apple ) );
    }
    REQUIRE( !dummy.can_add( item( itype_test_apple ) ) );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    zone_manager &zm = zone_manager::get_manager();
    item test_food( itype_test_apple );
    REQUIRE( zm.get_near_zone_type_for_item( test_food,
             here.get_abs( start_pos ) ) == zone_type_LOOT_FOOD );


    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );




    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( !dummy.activity );
}




TEST_CASE( "zone_sorting_virtual_pickup_from_cart",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );



    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs, true );
    REQUIRE( zone_manager::get_manager().has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );
    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    zone_manager &zm = zone_manager::get_manager();
    item test_food( itype_test_apple );
    REQUIRE( zm.get_near_zone_type_for_item( test_food,
             here.get_abs( start_pos ) ) == zone_type_LOOT_FOOD );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 0 );
    CHECK( !dummy.activity );
}


TEST_CASE( "zone_sorting_virtual_pickup_full_inventory",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );


    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs, true );
    REQUIRE( zone_manager::get_manager().has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );
    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );


    for( int i = 0; i < 500 && dummy.can_add( item( itype_test_apple ) ); i++ ) {
        dummy.i_add( item( itype_test_apple ) );
    }
    REQUIRE( !dummy.can_add( item( itype_test_apple ) ) );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    zone_manager &zm = zone_manager::get_manager();
    item test_food( itype_test_apple );
    REQUIRE( zm.get_near_zone_type_for_item( test_food,
             here.get_abs( start_pos ) ) == zone_type_LOOT_FOOD );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 0 );
    CHECK( !dummy.activity );
}



TEST_CASE( "zone_sorting_direct_delivery_to_cart",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );


    const tripoint_bub_ms cart_pos = start_pos + tripoint::east;
    const tripoint_abs_ms cart_abs = here.get_abs( cart_pos );
    here.ter_set( cart_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      cart_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::east );

    create_tile_zone( "Food", zone_type_LOOT_FOOD, cart_abs );


    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    zone_manager &zm = zone_manager::get_manager();
    item test_food( itype_test_apple );
    REQUIRE( zm.get_near_zone_type_for_item( test_food,
             here.get_abs( start_pos ) ) == zone_type_LOOT_FOOD );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    REQUIRE( cargo.has_value() );
    CHECK( count_items_or_charges( cart_pos, itype_test_apple, cargo ) == 1 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( !dummy.activity );
}



TEST_CASE( "zone_sorting_cart_source_with_dest_zone",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );



    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs, true );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, src_abs, true );


    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );
    CHECK( !dummy.activity );
}


TEST_CASE( "zone_sorting_direct_delivery_cart_full",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );


    const tripoint_bub_ms cart_pos = start_pos + tripoint::east;
    const tripoint_abs_ms cart_abs = here.get_abs( cart_pos );
    here.ter_set( cart_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      cart_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::east );

    create_tile_zone( "Food", zone_type_LOOT_FOOD, cart_abs );


    std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    REQUIRE( cargo.has_value() );
    for( int i = 0; i < 700; i++ ) {
        item filler( itype_test_apple );
        if( !cargo->vehicle().add_item( here, cargo->part(), filler ) ) {
            break;
        }
    }
    REQUIRE( cargo->items().free_volume() < item( itype_test_apple ).volume() );


    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );


    CHECK( count_items_or_charges( cart_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( !dummy.activity );
}



TEST_CASE( "zone_sorting_virtual_pickup_adjacent_dest",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );


    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs, true );
    REQUIRE( zone_manager::get_manager().has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );
    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 3 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 3 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 0 );
    CHECK( !dummy.activity );
}



TEST_CASE( "zone_sorting_virtual_pickup_unreachable_dest",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );


    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs, true );
    REQUIRE( zone_manager::get_manager().has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );


    const tripoint_bub_ms dest_pos = start_pos + tripoint( 5, 0, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    for( const tripoint &offset : {
             tripoint( 4, -1, 0 ), tripoint( 5, -1, 0 ), tripoint( 6, -1, 0 ),
             tripoint( 4, 0, 0 ), tripoint( 6, 0, 0 ),
             tripoint( 4, 1, 0 ), tripoint( 5, 1, 0 ), tripoint( 6, 1, 0 )
         } ) {
        here.ter_set( start_pos + offset, ter_t_wall );
    }


    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );
    CHECK( !dummy.activity );
}


TEST_CASE( "zone_sorting_adjacent_ground_delivery",
           "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );


    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( !dummy.has_item_with( []( const item & it ) {
        return it.typeId() == itype_test_apple;
    } ) );
    CHECK( !dummy.activity );
}



TEST_CASE( "zone_sorting_vehicle_on_terrain_unsorted_both_items",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );


    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );


    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );

    zone_manager &mgr = zone_manager::get_manager();
    mgr.cache_vzones();
    REQUIRE( mgr.has_terrain( zone_type_LOOT_UNSORTED, src_abs ) );
    REQUIRE( !mgr.has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );


    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );

    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );
    CHECK( !dummy.activity );
}




TEST_CASE( "zone_sorting_vehicle_unsorted_zone_ground_items_ignored",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );


    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );


    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs, true );

    zone_manager &mgr = zone_manager::get_manager();
    REQUIRE( !mgr.has_terrain( zone_type_LOOT_UNSORTED, src_abs ) );
    REQUIRE( mgr.has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );


    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );

    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );



    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 2 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 0 );
    CHECK( !dummy.activity );
}


TEST_CASE( "zone_sorting_both_zones_at_same_position",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );


    create_tile_zone( "Unsorted Ground", zone_type_LOOT_UNSORTED, src_abs );


    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );


    create_tile_zone( "Unsorted Vehicle", zone_type_LOOT_UNSORTED, src_abs, true );

    zone_manager &mgr = zone_manager::get_manager();
    mgr.cache_vzones();
    REQUIRE( mgr.has_terrain( zone_type_LOOT_UNSORTED, src_abs ) );
    REQUIRE( mgr.has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );


    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 2 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 0 );
    CHECK( !dummy.activity );
}



TEST_CASE( "zone_sorting_terrain_zone_sorts_non_grabbed_vehicle_cargo",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );


    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );


    vehicle *van = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                     src_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( van != nullptr );
    van->set_owner( dummy );


    zone_manager &mgr = zone_manager::get_manager();
    mgr.cache_vzones();
    REQUIRE( mgr.has_terrain( zone_type_LOOT_UNSORTED, src_abs ) );
    REQUIRE( !mgr.has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );


    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );

    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );



    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 2 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 0 );
    CHECK( !dummy.activity );
}


TEST_CASE( "zone_sorting_has_terrain_has_vehicle_helpers",
           "[zones][activities][sorting]" )
{
    map &here = get_map();

    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms pos_a_bub( 60, 60, 0 );
    const tripoint_abs_ms pos_a = here.get_abs( pos_a_bub );
    here.ter_set( pos_a_bub, ter_t_floor );

    const tripoint_bub_ms pos_b_bub( 62, 60, 0 );
    const tripoint_abs_ms pos_b = here.get_abs( pos_b_bub );
    here.ter_set( pos_b_bub, ter_t_floor );


    create_tile_zone( "Food Ground A", zone_type_LOOT_FOOD, pos_a );


    avatar &dummy = get_avatar();
    clear_avatar();
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      pos_a_bub, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    create_tile_zone( "Food Vehicle A", zone_type_LOOT_FOOD, pos_a, true );


    create_tile_zone( "Food Ground B", zone_type_LOOT_FOOD, pos_b );

    zone_manager &mgr = zone_manager::get_manager();
    mgr.cache_vzones();


    CHECK( mgr.has_terrain( zone_type_LOOT_FOOD, pos_a ) );
    CHECK( mgr.has_vehicle( zone_type_LOOT_FOOD, pos_a ) );
    CHECK( mgr.has( zone_type_LOOT_FOOD, pos_a ) );


    CHECK( mgr.has_terrain( zone_type_LOOT_FOOD, pos_b ) );
    CHECK( !mgr.has_vehicle( zone_type_LOOT_FOOD, pos_b ) );
    CHECK( mgr.has( zone_type_LOOT_FOOD, pos_b ) );
}






TEST_CASE( "zone_sorting_batches_nearby_sources",
           "[zones][items][activities][sorting][batching]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms s1_pos( 60, 60, 0 );
    dummy.setpos( here, s1_pos );
    dummy.clear_destination();

    dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );


    const tripoint_abs_ms s1_abs = here.get_abs( s1_pos );
    here.ter_set( s1_pos, ter_t_floor );
    create_tile_zone( "Unsorted S1", zone_type_LOOT_UNSORTED, s1_abs );
    for( int i = 0; i < 10; i++ ) {
        here.add_item_or_charges( s1_pos, item( itype_test_apple ) );
    }


    const tripoint_bub_ms s2_pos = s1_pos + tripoint::east;
    const tripoint_abs_ms s2_abs = here.get_abs( s2_pos );
    here.ter_set( s2_pos, ter_t_floor );
    create_tile_zone( "Unsorted S2", zone_type_LOOT_UNSORTED, s2_abs );
    for( int i = 0; i < 10; i++ ) {
        here.add_item_or_charges( s2_pos, item( itype_test_apple ) );
    }


    const tripoint_bub_ms dest_pos = s1_pos + tripoint( 0, 10, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    for( int y = s1_pos.y() + 1; y < dest_pos.y(); ++y ) {
        here.ter_set( tripoint_bub_ms( s1_pos.x(), y, 0 ), ter_t_floor );
    }

    REQUIRE( count_items_or_charges( s1_pos, itype_test_apple, std::nullopt ) == 10 );
    REQUIRE( count_items_or_charges( s2_pos, itype_test_apple, std::nullopt ) == 10 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( s1_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( s2_pos, itype_test_apple, std::nullopt ) == 0 );



    int carried = 0;
    dummy.visit_items( [&carried]( const item_location & it ) {
        if( it->typeId() == itype_test_apple ) {
            carried++;
        }
        return VisitResponse::NEXT;
    } );
    CHECK( carried == 20 );
}




TEST_CASE( "zone_sorting_batches_into_grabbed_vehicle",
           "[zones][items][activities][sorting][batching][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms s1_pos( 60, 60, 0 );
    dummy.setpos( here, s1_pos );
    dummy.clear_destination();


    const tripoint_bub_ms cart_pos = s1_pos + tripoint::south;
    here.ter_set( cart_pos, ter_t_floor );
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      cart_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );
    REQUIRE( dummy.get_grab_type() == object_type::VEHICLE );


    const tripoint_abs_ms s1_abs = here.get_abs( s1_pos );
    here.ter_set( s1_pos, ter_t_floor );
    create_tile_zone( "Unsorted S1", zone_type_LOOT_UNSORTED, s1_abs );
    for( int i = 0; i < 10; i++ ) {
        here.add_item_or_charges( s1_pos, item( itype_test_apple ) );
    }


    const tripoint_bub_ms s2_pos = s1_pos + tripoint::east;
    const tripoint_abs_ms s2_abs = here.get_abs( s2_pos );
    here.ter_set( s2_pos, ter_t_floor );
    create_tile_zone( "Unsorted S2", zone_type_LOOT_UNSORTED, s2_abs );
    for( int i = 0; i < 10; i++ ) {
        here.add_item_or_charges( s2_pos, item( itype_test_apple ) );
    }


    const tripoint_bub_ms dest_pos = s1_pos + tripoint( -5, 10, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    REQUIRE( count_items_or_charges( s1_pos, itype_test_apple, std::nullopt ) == 10 );
    REQUIRE( count_items_or_charges( s2_pos, itype_test_apple, std::nullopt ) == 10 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( s1_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( s2_pos, itype_test_apple, std::nullopt ) == 0 );

    int total = 0;
    dummy.visit_items( [&total]( const item_location & it ) {
        if( it->typeId() == itype_test_apple ) {
            total++;
        }
        return VisitResponse::NEXT;
    } );
    std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    if( cargo.has_value() ) {
        total += count_items_or_charges( cart_pos, itype_test_apple, cargo );
    }
    CHECK( total == 20 );
}



TEST_CASE( "zone_sorting_adjacent_dest_both_full",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );


    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      dest_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::east );

    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    std::optional<vpart_reference> cargo = here.veh_at( dest_pos ).cargo();
    REQUIRE( cargo.has_value() );
    for( int i = 0; i < 700; i++ ) {
        item filler( itype_test_apple );
        if( !cargo->vehicle().add_item( here, cargo->part(), filler ) ) {
            break;
        }
    }
    REQUIRE( cargo->items().free_volume() < item( itype_test_apple ).volume() );


    const item filler_item( itype_test_apple );
    while( here.free_volume( dest_pos ) >= filler_item.volume() ) {
        here.add_item( dest_pos, filler_item );
    }
    REQUIRE( here.free_volume( dest_pos ) < filler_item.volume() );


    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( !dummy.activity );


    for( int dx = -1; dx <= 1; dx++ ) {
        for( int dy = -1; dy <= 1; dy++ ) {
            if( dx == 0 && dy == 0 ) {
                continue;
            }
            const tripoint_bub_ms neighbor = dest_pos + tripoint( dx, dy, 0 );
            if( neighbor == src_pos || neighbor == start_pos ) {
                continue;
            }
            CHECK( here.i_at( neighbor ).empty() );
        }
    }
}


TEST_CASE( "zone_sorting_adjacent_ground_dest_full",
           "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );


    const tripoint_bub_ms dest_pos = start_pos + tripoint::north;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );


    const item filler_item( itype_test_apple );
    while( here.free_volume( dest_pos ) >= filler_item.volume() ) {
        here.add_item( dest_pos, filler_item );
    }
    REQUIRE( here.free_volume( dest_pos ) < filler_item.volume() );


    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( !dummy.activity );


    for( int dx = -1; dx <= 1; dx++ ) {
        for( int dy = -1; dy <= 1; dy++ ) {
            if( dx == 0 && dy == 0 ) {
                continue;
            }
            const tripoint_bub_ms neighbor = dest_pos + tripoint( dx, dy, 0 );
            if( neighbor == src_pos || neighbor == start_pos ) {
                continue;
            }
            CHECK( here.i_at( neighbor ).empty() );
        }
    }
}


TEST_CASE( "zone_sorting_vehicle_dest_cargo_full_ground_fallback",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );


    const tripoint_bub_ms cart_pos = start_pos + tripoint::east;
    const tripoint_abs_ms cart_abs = here.get_abs( cart_pos );
    here.ter_set( cart_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      cart_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::east );


    create_tile_zone( "Food", zone_type_LOOT_FOOD, cart_abs, true );


    std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    REQUIRE( cargo.has_value() );
    for( int i = 0; i < 700; i++ ) {
        item filler( itype_test_apple );
        if( !cargo->vehicle().add_item( here, cargo->part(), filler ) ) {
            break;
        }
    }
    REQUIRE( cargo->items().free_volume() < item( itype_test_apple ).volume() );


    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( count_items_or_charges( cart_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( !dummy.activity );
}






TEST_CASE( "zone_sorting_terrain_zone_vehicle_cargo_no_loop",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();


    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );



    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, src_abs );


    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );


    int turns = 0;
    const int max_turns = 20;
    do {
        dummy.mod_moves( dummy.get_speed() );
        while( dummy.get_moves() > 0 && dummy.activity ) {
            dummy.activity.do_turn( dummy );
        }
        turns++;
    } while( dummy.activity && turns < max_turns );

    REQUIRE( turns < max_turns );
    CHECK( !dummy.activity );


    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
}








TEST_CASE( "zone_sorting_multi_dest_processes_all_items",
           "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();
















    const tripoint_bub_ms start_pos( 60, 61, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    const tripoint_bub_ms src_pos( 60, 60, 0 );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, here.get_abs( src_pos ) );




    const tripoint_bub_ms pfood_pos( 60, 59, 0 );
    here.ter_set( pfood_pos, ter_t_floor );
    create_tile_zone( "PFood", zone_type_LOOT_PFOOD, here.get_abs( pfood_pos ) );


    const tripoint_bub_ms food_pos( 55, 60, 0 );
    here.ter_set( food_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, here.get_abs( food_pos ) );


    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    here.add_item_or_charges( src_pos, item( itype_test_bitter_almond ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    REQUIRE( count_items_or_charges( src_pos, itype_test_bitter_almond, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );



    int teleports = 0;
    const int max_teleports = 30;
    do {
        int turns = 0;
        while( dummy.activity && turns < 200 ) {
            dummy.mod_moves( dummy.get_speed() );
            while( dummy.get_moves() > 0 && dummy.activity ) {
                dummy.activity.do_turn( dummy );
            }
            turns++;
        }
        if( dummy.destination_point ) {
            dummy.setpos( here, here.get_bub( *dummy.destination_point ) );
            if( dummy.has_destination_activity() ) {
                dummy.start_destination_activity();
            } else {
                dummy.clear_destination();
            }
            teleports++;
        }
    } while( ( dummy.activity || dummy.destination_point ) && teleports < max_teleports );

    REQUIRE( teleports < max_teleports );


    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( src_pos, itype_test_bitter_almond, std::nullopt ) == 0 );
}




TEST_CASE( "zone_sorting_no_grab_weight_gate",
           "[zones][items][activities][sorting][weight]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );






    dummy.set_str_base( 6 );
    dummy.set_str_bonus( 0 );
    const int num_boulders = 5;


    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    here.ter_set( start_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );
    for( int i = 0; i < num_boulders; i++ ) {
        here.add_item_or_charges( start_pos, item( itype_test_heavy_boulder ) );
    }


    const tripoint_bub_ms dest_pos = start_pos + tripoint( 0, 8, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    for( int y = start_pos.y(); y <= dest_pos.y(); ++y ) {
        here.ter_set( tripoint_bub_ms( start_pos.x(), y, 0 ), ter_t_floor );
    }

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    const int remaining = count_items_or_charges( start_pos, itype_test_heavy_boulder, std::nullopt );
    int carried = 0;
    dummy.visit_items( [&carried]( const item_location & it ) {
        if( it->typeId() == itype_test_heavy_boulder ) {
            carried++;
        }
        return VisitResponse::NEXT;
    } );

    CAPTURE( remaining );
    CAPTURE( carried );
    CAPTURE( dummy.weight_carried().value() );
    CAPTURE( dummy.weight_capacity().value() );

    CHECK( carried > 0 );
    CHECK( carried < num_boulders );
    CHECK( remaining > 0 );

    CHECK( dummy.weight_carried() <= dummy.weight_capacity() );
}




TEST_CASE( "zone_sorting_drag_weight_gate",
           "[zones][items][activities][sorting][weight][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();



    dummy.set_str_base( 4 );
    dummy.set_str_bonus( 0 );

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );


    const tripoint_bub_ms cart_pos = start_pos + tripoint::south;


    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      cart_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );
    REQUIRE( dummy.get_grab_type() == object_type::VEHICLE );


    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    here.ter_set( start_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );
    const int num_boulders = 20;
    for( int i = 0; i < num_boulders; i++ ) {
        here.add_item_or_charges( start_pos, item( itype_test_heavy_boulder ) );
    }



    const tripoint_bub_ms dest_pos = start_pos + tripoint( 0, 8, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    const int arm_str = dummy.get_arm_str();
    CAPTURE( arm_str );
    CAPTURE( cart->total_mass( here ).value() );


    {

        const int easy_req = cart->drag_str_req_at( here, dest_pos );
        CAPTURE( easy_req );
        CHECK( easy_req <= arm_str );


        int threshold_kg = -1;
        for( int extra_kg = 10; extra_kg <= 300; extra_kg += 10 ) {
            units::mass proj = cart->total_mass( here ) + extra_kg * 1_kilogram;
            int req = cart->drag_str_req_at( here, dest_pos, proj );
            if( req > arm_str ) {
                threshold_kg = extra_kg;
                break;
            }
        }
        CAPTURE( threshold_kg );
        REQUIRE( threshold_kg > 0 );

        units::mass heavy = cart->total_mass( here ) + 200_kilogram;
        const int hard_req = cart->drag_str_req_at( here, dest_pos, heavy );
        CAPTURE( hard_req );
        REQUIRE( hard_req > arm_str );
    }


    {
        std::vector<tripoint_abs_ms> test_dropoffs = { here.get_abs( dest_pos ) };
        auto worst = zone_sorting::worst_drag_tile_on_route( dummy, test_dropoffs );
        REQUIRE( worst.has_value() );
        if( worst ) {
            int req_at_worst = cart->drag_str_req_at( here, *worst,
                               cart->total_mass( here ) + 200_kilogram );
            CAPTURE( req_at_worst );
            CAPTURE( worst->x() );
            CAPTURE( worst->y() );
            REQUIRE( req_at_worst > arm_str );
        }
    }


    {
        units::mass after_10 = cart->total_mass( here ) + 100_kilogram;
        std::vector<tripoint_abs_ms> test_drops = { here.get_abs( dest_pos ) };
        auto wt = zone_sorting::worst_drag_tile_on_route( dummy, test_drops );
        if( wt ) {
            int req10 = cart->drag_str_req_at( here, *wt, after_10 );
            CAPTURE( req10 );

            REQUIRE( req10 > arm_str );
        }
    }

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    int in_cart = 0;
    std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    if( cargo.has_value() ) {
        in_cart = count_items_or_charges( cart_pos, itype_test_heavy_boulder, cargo );
    }
    int at_source = count_items_or_charges( start_pos, itype_test_heavy_boulder, std::nullopt );
    int carried = 0;
    dummy.visit_items( [&carried]( const item_location & it ) {
        if( it->typeId() == itype_test_heavy_boulder ) {
            carried++;
        }
        return VisitResponse::NEXT;
    } );

    CAPTURE( in_cart );
    CAPTURE( carried );
    CAPTURE( at_source );



    CHECK( in_cart + carried + at_source == num_boulders );
    CHECK( at_source > 0 );
}

static void build_open_area( map &here, const tripoint_bub_ms &center, int radius = 15 )
{
    for( int x = center.x() - radius; x <= center.x() + radius; x++ ) {
        for( int y = center.y() - radius; y <= center.y() + radius; y++ ) {
            here.ter_set( tripoint_bub_ms( x, y, 0 ), ter_t_floor );
        }
    }
}

static vehicle *setup_grabbed_cart( avatar &dummy, map &here,
                                    const tripoint_bub_ms &player_pos,
                                    const tripoint_rel_ms &grab_dir )
{
    dummy.setpos( here, player_pos );
    dummy.clear_destination();

    const tripoint_bub_ms cart_pos = player_pos + grab_dir;
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      cart_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, grab_dir );
    REQUIRE( dummy.get_grab_type() == object_type::VEHICLE );
    REQUIRE( cart->get_points().size() == 1 );
    return cart;
}

TEST_CASE( "route_with_grab_strength_gating",
           "[zones][pathfinding][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_bub_ms dest_pos( 60, 70, 0 );

    build_open_area( here, tripoint_bub_ms( 60, 65, 0 ) );

    vehicle *cart = setup_grabbed_cart( dummy, here, start_pos, tripoint_rel_ms::south );

    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );
    here.add_item_or_charges( start_pos, item( itype_test_apple ) );

    const int low_str = 4;
    dummy.set_str_base( low_str );
    dummy.set_str_bonus( 0 );

    const tripoint_bub_ms cart_pos = start_pos + tripoint::south;
    std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    REQUIRE( cargo.has_value() );
    for( int i = 0; i < 200; i++ ) {
        if( !cargo->vehicle().add_item( here, cargo->part(),
                                        item( itype_test_heavy_boulder ) ) ) {
            break;
        }
    }

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    const int drag_req = cart->drag_str_req_at( here, start_pos + tripoint::south );
    CAPTURE( drag_req );
    CAPTURE( dummy.get_arm_str() );
    REQUIRE( drag_req > dummy.get_arm_str() );

    dummy.setpos( here, start_pos + tripoint::north );
    dummy.setpos( here, start_pos );
    CHECK( zone_sorting::route_length( dummy, dest_pos ) == INT_MAX );

    dummy.set_str_base( 20 );
    dummy.set_str_bonus( 0 );
    REQUIRE( dummy.get_arm_str() >= drag_req );

    dummy.setpos( here, start_pos + tripoint::north );
    dummy.setpos( here, start_pos );
    CHECK( zone_sorting::route_length( dummy, dest_pos ) != INT_MAX );

    dummy.grab( object_type::NONE );
    clear_map_without_vision();
}

TEST_CASE( "route_cache_invalidation_on_mass_change",
           "[zones][pathfinding][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_bub_ms dest_pos( 60, 70, 0 );

    build_open_area( here, tripoint_bub_ms( 60, 65, 0 ) );

    vehicle *cart = setup_grabbed_cart( dummy, here, start_pos, tripoint_rel_ms::south );

    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );
    here.add_item_or_charges( start_pos, item( itype_test_apple ) );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    const int len_before = zone_sorting::route_length( dummy, dest_pos );
    CHECK( len_before != INT_MAX );

    const tripoint_bub_ms cart_pos = start_pos + tripoint::south;
    std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    REQUIRE( cargo.has_value() );
    for( int i = 0; i < 200; i++ ) {
        if( !cargo->vehicle().add_item( here, cargo->part(),
                                        item( itype_test_heavy_boulder ) ) ) {
            break;
        }
    }

    dummy.set_str_base( 4 );
    dummy.set_str_bonus( 0 );

    const int drag_req = cart->drag_str_req_at( here, cart_pos );
    CAPTURE( drag_req );
    CAPTURE( dummy.get_arm_str() );
    REQUIRE( drag_req > dummy.get_arm_str() );

    const int len_after = zone_sorting::route_length( dummy, dest_pos );
    CHECK( len_after == INT_MAX );

    dummy.grab( object_type::NONE );
    zone_manager::get_manager().clear();
    clear_map_without_vision();
}





TEST_CASE( "zone_sort_unload_liquid_container_hang", "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms start_pos = tripoint_bub_ms::zero + tripoint::east;
    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    dummy.set_pos_abs_only( start_abs );



    const tripoint_bub_ms chem_pos = start_pos + tripoint::east;
    here.ter_set( chem_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );
    create_tile_zone( "Unload All", zone_type_UNLOAD_ALL, start_abs );
    create_tile_zone( "Chemical", zone_type_LOOT_CHEMICAL, here.get_abs( chem_pos ) );




    item bottle( itype_bottle_glass );
    item milk( itype_test_milk );
    REQUIRE( milk.made_of( phase_id::LIQUID ) );
    REQUIRE( bottle.put_in( milk, pocket_type::CONTAINER ).success() );
    here.add_item_or_charges( start_pos, bottle );
    here.add_item_or_charges( start_pos, item( itype_chem_washing_soda ) );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    CHECK( !dummy.activity );
}




TEST_CASE( "zone_sort_think_prunes_empty_sources", "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );


    const tripoint_abs_ms zone_start = here.get_abs( start_pos - point( 10, 10 ) );
    const tripoint_abs_ms zone_end = here.get_abs( start_pos + point( 9, 9 ) );
    zone_manager &zm = zone_manager::get_manager();
    zm.add( "Unsorted", zone_type_LOOT_UNSORTED, faction_your_followers,
            false, true, zone_start, zone_end );


    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, here.get_abs( dest_pos ) );



    here.add_item_or_charges( start_pos, item( itype_test_apple ) );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    CHECK( !dummy.activity );

    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 1 );
}






TEST_CASE( "cancel_activity_clears_auto_resume_for_multi_type",
           "[zones][activities][cancel]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();
    clear_map_without_vision();



    dummy.assign_activity( unload_loot_activity_actor() );
    REQUIRE( dummy.activity );
    REQUIRE( dummy.activity.is_multi_type() );
    REQUIRE( dummy.activity.auto_resume );



    dummy.cancel_activity();
    dummy.resume_backlog_activity();


    CHECK( !dummy.activity );
    CHECK( ( dummy.backlog.empty() ||
             !dummy.backlog.front().auto_resume ) );
}




TEST_CASE( "cancel_non_resumable_sub_blocks_parent_auto_resume",
           "[zones][activities][cancel]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();
    clear_map_without_vision();



    dummy.assign_activity( multi_chop_planks_activity_actor() );
    REQUIRE( dummy.activity.is_multi_type() );
    REQUIRE( dummy.activity.auto_resume );
    player_activity parent = dummy.activity;
    dummy.backlog.push_front( parent );


    dummy.activity = player_activity(
                         chop_planks_activity_actor( 100 ) );
    REQUIRE( !dummy.activity.can_resume() );
    REQUIRE( dummy.backlog.front().auto_resume );


    dummy.cancel_activity();
    dummy.resume_backlog_activity();

    CHECK( !dummy.activity );
}




TEST_CASE( "multi_zone_vehicle_repair_large_zone_no_hang",
           "[zones][activities][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    dummy.activity = player_activity();
    dummy.backlog.clear();
    dummy.clear_destination();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );




    const tripoint_bub_ms veh_pos = start_pos + tripoint( 2, 0, 0 );
    here.ter_set( veh_pos, ter_t_floor );
    vehicle *veh = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                     veh_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );
    for( const vpart_reference &vpr : veh->get_all_parts() ) {
        veh->set_hp( vpr.part(), vpr.part().info().durability / 2, false );
    }


    const tripoint_abs_ms zone_start = here.get_abs( start_pos - point( 10, 10 ) );
    const tripoint_abs_ms zone_end = here.get_abs( start_pos + point( 9, 9 ) );
    zone_manager &zm = zone_manager::get_manager();
    zm.add( "Vehicle Repair", zone_type_VEHICLE_REPAIR, faction_your_followers,
            false, true, zone_start, zone_end );


    dummy.assign_activity( multi_vehicle_repair_activity_actor() );




    int turns = 0;
    const int max_turns = 50;
    while( ( !dummy.activity.is_null() || dummy.is_auto_moving() ) && turns < max_turns ) {
        dummy.set_moves( dummy.get_speed() );
        if( dummy.is_auto_moving() ) {
            dummy.setpos( here, here.get_bub( *dummy.destination_point ) );
            here.build_map_cache( dummy.posz() );
            dummy.start_destination_activity();
        }
        if( dummy.activity ) {
            dummy.activity.do_turn( dummy );
        }
        turns++;
    }

    CHECK( !dummy.activity );

    CHECK( turns < max_turns );
}



TEST_CASE( "zone_sort_viewport_bbox", "[zones][viewport]" )
{
    using zone_sorting::viewport_bbox;
    using zone_sorting::calc_zone_bbox;
    using zone_sorting::expand_bbox;

    SECTION( "single tile" ) {
        std::unordered_set<tripoint_abs_ms> tiles;
        tiles.emplace( 10, 20, 0 );
        viewport_bbox bbox = calc_zone_bbox( tiles );
        CHECK( bbox.min_corner == tripoint_abs_ms( 10, 20, 0 ) );
        CHECK( bbox.max_corner == tripoint_abs_ms( 10, 20, 0 ) );
        CHECK( bbox.centroid == tripoint_abs_ms( 10, 20, 0 ) );
        CHECK( bbox.width() == 0 );
        CHECK( bbox.height() == 0 );
    }

    SECTION( "spread tiles" ) {
        std::unordered_set<tripoint_abs_ms> tiles;
        tiles.emplace( 5, 10, 0 );
        tiles.emplace( 25, 10, 0 );
        tiles.emplace( 15, 30, 0 );
        viewport_bbox bbox = calc_zone_bbox( tiles );
        CHECK( bbox.min_corner == tripoint_abs_ms( 5, 10, 0 ) );
        CHECK( bbox.max_corner == tripoint_abs_ms( 25, 30, 0 ) );
        CHECK( bbox.centroid == tripoint_abs_ms( 15, 20, 0 ) );
        CHECK( bbox.width() == 20 );
        CHECK( bbox.height() == 20 );
    }

    SECTION( "empty set" ) {
        std::unordered_set<tripoint_abs_ms> tiles;
        viewport_bbox bbox = calc_zone_bbox( tiles );
        CHECK( bbox.width() == 0 );
        CHECK( bbox.height() == 0 );
    }

    SECTION( "expand_bbox grows when tile outside" ) {
        std::unordered_set<tripoint_abs_ms> tiles;
        tiles.emplace( 10, 10, 0 );
        tiles.emplace( 20, 20, 0 );
        viewport_bbox bbox = calc_zone_bbox( tiles );
        CHECK( bbox.width() == 10 );
        CHECK( bbox.height() == 10 );

        bool grew = expand_bbox( bbox, tripoint_abs_ms( 30, 20, 0 ) );
        CHECK( grew );
        CHECK( bbox.max_corner.x() == 30 );
        CHECK( bbox.width() == 20 );
        CHECK( bbox.centroid.x() == 20 );
    }

    SECTION( "expand_bbox no-op when tile inside" ) {
        std::unordered_set<tripoint_abs_ms> tiles;
        tiles.emplace( 10, 10, 0 );
        tiles.emplace( 20, 20, 0 );
        viewport_bbox bbox = calc_zone_bbox( tiles );

        bool grew = expand_bbox( bbox, tripoint_abs_ms( 15, 15, 0 ) );
        CHECK_FALSE( grew );
        CHECK( bbox.width() == 10 );
        CHECK( bbox.height() == 10 );
    }
}

TEST_CASE( "zone_sort_viewport_zoom", "[zones][viewport]" )
{
    using zone_sorting::calc_target_zoom;



    const int vis_w = 60;
    const int vis_h = 40;
    const int cur_zoom = 16;
    const int saved = 16;

    SECTION( "bbox fits at current zoom" ) {

        int z = calc_target_zoom( 20, 20, vis_w, vis_h, cur_zoom, saved );
        CHECK( z == 16 );
    }

    SECTION( "bbox needs one step zoom out" ) {




        int z = calc_target_zoom( 100, 80, vis_w, vis_h, cur_zoom, saved );
        CHECK( z == 4 );
    }

    SECTION( "bbox needs moderate zoom out" ) {



        int z = calc_target_zoom( 80, 30, vis_w, vis_h, cur_zoom, saved );
        CHECK( z == 8 );
    }

    SECTION( "clamp at minimum zoom 4" ) {

        int z = calc_target_zoom( 1000, 1000, vis_w, vis_h, cur_zoom, saved );
        CHECK( z == 4 );
    }

    SECTION( "never zoom in beyond saved_zoom" ) {


        int z = calc_target_zoom( 5, 5, vis_w, vis_h, cur_zoom, saved );
        CHECK( z == 16 );
    }

    SECTION( "respects higher saved_zoom" ) {


        int z = calc_target_zoom( 5, 5, vis_w, vis_h, cur_zoom, 32 );
        CHECK( z == 32 );
    }
}

TEST_CASE( "zone_sort_viewport_lifecycle", "[zones][viewport][activities]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map();
    zone_manager::get_manager().clear();





    const tripoint_bub_ms start_pos( 60, 61, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();
    dummy.zone_sort_viewport = avatar::zone_sort_viewport_t{};

    const tripoint_bub_ms src_pos( 60, 60, 0 );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, here.get_abs( src_pos ) );

    const tripoint_bub_ms dest_pos( 60, 55, 0 );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "PFood", zone_type_LOOT_PFOOD, here.get_abs( dest_pos ) );

    here.add_item_or_charges( src_pos, item( itype_test_apple ) );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    SECTION( "activates on sort start and deactivates on completion" ) {
        CHECK_FALSE( dummy.zone_sort_viewport.active );

        dummy.assign_activity( zone_sort_activity_actor() );


        dummy.mod_moves( dummy.get_speed() );
        while( dummy.get_moves() > 0 && dummy.activity ) {
            dummy.activity.do_turn( dummy );
        }


        CHECK( dummy.zone_sort_viewport.active );


        int teleports = 0;
        const int max_teleports = 30;
        do {
            int turns = 0;
            while( dummy.activity && turns < 200 ) {
                dummy.mod_moves( dummy.get_speed() );
                while( dummy.get_moves() > 0 && dummy.activity ) {
                    dummy.activity.do_turn( dummy );
                }
                turns++;
            }
            if( dummy.destination_point ) {
                dummy.setpos( here, here.get_bub( *dummy.destination_point ) );
                if( dummy.has_destination_activity() ) {
                    dummy.start_destination_activity();
                } else {
                    dummy.clear_destination();
                }
                teleports++;
            }
        } while( ( dummy.activity || dummy.destination_point ) && teleports < max_teleports );

        REQUIRE( teleports < max_teleports );


        CHECK_FALSE( dummy.zone_sort_viewport.active );
    }

    SECTION( "stays active during auto-move leg" ) {
        dummy.assign_activity( zone_sort_activity_actor() );


        int turns = 0;
        while( dummy.activity && turns < 200 ) {
            dummy.mod_moves( dummy.get_speed() );
            while( dummy.get_moves() > 0 && dummy.activity ) {
                dummy.activity.do_turn( dummy );
            }
            turns++;
        }


        if( dummy.destination_point ) {

            CHECK( dummy.zone_sort_viewport.active );
        }
    }

    SECTION( "abort_automove restores viewport" ) {
        dummy.assign_activity( zone_sort_activity_actor() );


        int turns = 0;
        while( dummy.activity && turns < 200 ) {
            dummy.mod_moves( dummy.get_speed() );
            while( dummy.get_moves() > 0 && dummy.activity ) {
                dummy.activity.do_turn( dummy );
            }
            turns++;
        }

        if( dummy.destination_point ) {
            CHECK( dummy.zone_sort_viewport.active );
            dummy.abort_automove();
            CHECK_FALSE( dummy.zone_sort_viewport.active );
        }
    }

    SECTION( "deactivates when nothing to sort" ) {

        map_stack items_at_src = here.i_at( src_pos );
        items_at_src.clear();

        CHECK_FALSE( dummy.zone_sort_viewport.active );

        dummy.assign_activity( zone_sort_activity_actor() );



        dummy.mod_moves( dummy.get_speed() );
        while( dummy.get_moves() > 0 && dummy.activity ) {
            dummy.activity.do_turn( dummy );
        }

        CHECK_FALSE( dummy.activity );
        CHECK_FALSE( dummy.zone_sort_viewport.active );
    }

    SECTION( "deactivates when no matching destination zone" ) {

        map_stack items_at_src = here.i_at( src_pos );
        items_at_src.clear();
        here.add_item_or_charges( src_pos, item( itype_test_heavy_boulder ) );

        CHECK_FALSE( dummy.zone_sort_viewport.active );

        dummy.assign_activity( zone_sort_activity_actor() );

        dummy.mod_moves( dummy.get_speed() );
        while( dummy.get_moves() > 0 && dummy.activity ) {
            dummy.activity.do_turn( dummy );
        }

        CHECK_FALSE( dummy.activity );
        CHECK_FALSE( dummy.zone_sort_viewport.active );
    }

    SECTION( "viewport_saved_zoom survives serialize roundtrip" ) {
        dummy.assign_activity( zone_sort_activity_actor() );


        dummy.mod_moves( dummy.get_speed() );
        while( dummy.get_moves() > 0 && dummy.activity ) {
            dummy.activity.do_turn( dummy );
        }

        REQUIRE( dummy.zone_sort_viewport.active );


        const zone_sort_activity_actor *actor =
            dynamic_cast<const zone_sort_activity_actor *>( dummy.activity.actor.get() );
        if( actor ) {
            CHECK( actor->viewport_was_active );
            CHECK( actor->viewport_saved_zoom == 16 );
        }
    }

    SECTION( "viewport reactivated after load keeps actor's saved zoom" ) {
        dummy.assign_activity( zone_sort_activity_actor() );


        dummy.mod_moves( dummy.get_speed() );
        while( dummy.get_moves() > 0 && dummy.activity ) {
            dummy.activity.do_turn( dummy );
        }
        REQUIRE( dummy.zone_sort_viewport.active );

        zone_sort_activity_actor *actor =
            dynamic_cast<zone_sort_activity_actor *>( dummy.activity.actor.get() );
        REQUIRE( actor != nullptr );
        REQUIRE( actor->viewport_was_active );

        actor->viewport_saved_zoom = 32;


        dummy.zone_sort_viewport = avatar::zone_sort_viewport_t{};

        g->try_activate_zone_sort_viewport();

        CHECK( dummy.zone_sort_viewport.active );
        CHECK( dummy.zone_sort_viewport.saved_zoom == 32 );
    }
}




TEST_CASE( "zone_sort_skips_spillable_container_in_mixed_source",
           "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    dummy.clear_destination();

    restore_on_out_of_scope<test_mode_spilling_action_t> restore_spill( test_mode_spilling_action );
    test_mode_spilling_action = test_mode_spilling_action_t::spill_all;

    const tripoint_bub_ms start_pos = tripoint_bub_ms::zero + tripoint::east;
    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    dummy.set_pos_abs_only( start_abs );
    dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );

    const tripoint_bub_ms dest_pos = start_pos + tripoint( 3, 0, 0 );
    here.ter_set( dest_pos, ter_t_floor );

    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, here.get_abs( dest_pos ) );


    create_tile_zone( "Default", zone_type_LOOT_DEFAULT, here.get_abs( dest_pos ) );


    item container( itype_test_watertight_open_sealed_container_250ml );
    item liquid( itype_test_liquid_1ml );
    liquid.charges = 5;
    REQUIRE( container.put_in( liquid, pocket_type::CONTAINER ).success() );
    REQUIRE( container.is_bucket_nonempty() );
    here.add_item_or_charges( start_pos, container );


    here.add_item_or_charges( start_pos, item( itype_test_apple ) );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );



    int teleports = 0;
    const int max_teleports = 30;
    do {
        int turns = 0;
        while( dummy.activity && turns < 200 ) {
            dummy.mod_moves( dummy.get_speed() );
            while( dummy.get_moves() > 0 && dummy.activity ) {
                dummy.activity.do_turn( dummy );
            }
            turns++;
        }
        if( dummy.destination_point ) {
            dummy.setpos( here, here.get_bub( *dummy.destination_point ) );
            if( dummy.has_destination_activity() ) {
                dummy.start_destination_activity();
            } else {
                dummy.clear_destination();
            }
            teleports++;
        }
    } while( ( dummy.activity || dummy.destination_point ) && teleports < max_teleports );

    REQUIRE( teleports < max_teleports );


    bool found_container_with_liquid = find_liquid_container( start_pos );

    CHECK( found_container_with_liquid );


    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 1 );
}

TEST_CASE( "zone_sort_completes_when_only_spillable_items_at_source",
           "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    dummy.clear_destination();

    restore_on_out_of_scope<test_mode_spilling_action_t> restore_spill( test_mode_spilling_action );
    test_mode_spilling_action = test_mode_spilling_action_t::spill_all;

    const tripoint_bub_ms start_pos = tripoint_bub_ms::zero + tripoint::east;
    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    dummy.set_pos_abs_only( start_abs );
    dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );

    const tripoint_bub_ms dest_pos = start_pos + tripoint( 3, 0, 0 );
    here.ter_set( dest_pos, ter_t_floor );

    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );
    create_tile_zone( "Default", zone_type_LOOT_DEFAULT, here.get_abs( dest_pos ) );


    item container( itype_test_watertight_open_sealed_container_250ml );
    item liquid( itype_test_liquid_1ml );
    liquid.charges = 5;
    REQUIRE( container.put_in( liquid, pocket_type::CONTAINER ).success() );
    REQUIRE( container.is_bucket_nonempty() );
    here.add_item_or_charges( start_pos, container );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );


    bool found_container_with_liquid = find_liquid_container( start_pos );

    CHECK( found_container_with_liquid );

    CHECK( !dummy.activity );
}



TEST_CASE( "has_items_to_sort_returns_false_for_spillable_only_source",
           "[zones][items][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    dummy.clear_destination();

    const tripoint_bub_ms start_pos = tripoint_bub_ms::zero + tripoint::east;
    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    dummy.set_pos_abs_only( start_abs );

    const tripoint_bub_ms dest_pos = start_pos + tripoint( 3, 0, 0 );
    here.ter_set( dest_pos, ter_t_floor );

    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );
    create_tile_zone( "Default", zone_type_LOOT_DEFAULT, here.get_abs( dest_pos ) );

    item container( itype_test_watertight_open_sealed_container_250ml );
    item liquid( itype_test_liquid_1ml );
    liquid.charges = 5;
    REQUIRE( container.put_in( liquid, pocket_type::CONTAINER ).success() );
    REQUIRE( container.is_bucket_nonempty() );
    here.add_item_or_charges( start_pos, container );

    zone_sorting::zone_items items = zone_sorting::populate_items( start_pos );
    REQUIRE( !items.empty() );

    zone_sorting::unload_sort_options zone_unload_options =
        zone_sorting::set_unload_options( dummy, start_abs, true );

    std::vector<item_location> other_activity_items;
    bool pickup_failure = false;

    bool result = zone_sorting::has_items_to_sort( dummy, start_abs, zone_unload_options,
                  other_activity_items, items, &pickup_failure );

    CHECK_FALSE( result );
}

TEST_CASE( "zone_sort_sealed_liquid_container_is_sorted_normally",
           "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    dummy.clear_destination();

    const tripoint_bub_ms start_pos = tripoint_bub_ms::zero + tripoint::east;
    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    dummy.set_pos_abs_only( start_abs );
    dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );

    const tripoint_bub_ms dest_pos = start_pos + tripoint( 3, 0, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );

    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );
    create_tile_zone( "Default", zone_type_LOOT_DEFAULT, dest_abs );

    item container( itype_test_watertight_open_sealed_container_250ml );
    item liquid( itype_test_liquid_1ml );
    liquid.charges = 5;
    REQUIRE( container.put_in( liquid, pocket_type::CONTAINER ).success() );
    REQUIRE( container.seal() );
    REQUIRE( !container.is_bucket_nonempty() );


    const zone_manager &mgr = zone_manager::get_manager();
    REQUIRE( mgr.get_near_zone_type_for_item( container, start_abs ) == zone_type_LOOT_DEFAULT );

    here.add_item_or_charges( start_pos, container );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );

    int teleports = 0;
    const int max_teleports = 30;
    do {
        int turns = 0;
        while( dummy.activity && turns < 200 ) {
            dummy.mod_moves( dummy.get_speed() );
            while( dummy.get_moves() > 0 && dummy.activity ) {
                dummy.activity.do_turn( dummy );
            }
            turns++;
        }
        if( dummy.destination_point ) {
            dummy.setpos( here, here.get_bub( *dummy.destination_point ) );
            if( dummy.has_destination_activity() ) {
                dummy.start_destination_activity();
            } else {
                dummy.clear_destination();
            }
            teleports++;
        }
    } while( ( dummy.activity || dummy.destination_point ) && teleports < max_teleports );

    REQUIRE( teleports < max_teleports );


    bool found_at_dest = find_liquid_container( dest_pos );

    CHECK( found_at_dest );

    CHECK( count_items_or_charges( start_pos, itype_test_watertight_open_sealed_container_250ml,
                                   std::nullopt ) == 0 );
}





TEST_CASE( "npc_zone_sorting_ignores_personal_zones", "[zones][items][activities][sorting][npc]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    zone_manager &zm = zone_manager::get_manager();
    zm.clear();


    const tripoint_bub_ms camp_pos = tripoint_bub_ms::zero + tripoint::east;
    const tripoint_abs_ms camp_abs = here.get_abs( camp_pos );

    const tripoint_bub_ms camp_dest_pos = camp_pos + tripoint::east;
    const tripoint_abs_ms camp_dest_abs = here.get_abs( camp_dest_pos );
    here.ter_set( camp_dest_pos, ter_t_floor );

    create_tile_zone( "Camp Unsorted", zone_type_LOOT_UNSORTED, camp_abs );
    create_tile_zone( "Camp Food", zone_type_LOOT_FOOD, camp_dest_abs );


    const tripoint_bub_ms avatar_pos = camp_pos + tripoint( 0, 3, 0 );
    const tripoint_abs_ms avatar_abs = here.get_abs( avatar_pos );
    dummy.set_pos_abs_only( avatar_abs );


    const tripoint_rel_ms personal_dest_rel( 1, 0, 0 );
    zm.add( "Personal Food", zone_type_LOOT_FOOD, faction_your_followers,
            false, true, personal_dest_rel, personal_dest_rel, nullptr );
    zm.cache_avatar_location();

    const tripoint_bub_ms personal_dest_pos = avatar_pos + tripoint::east;
    here.ter_set( personal_dest_pos, ter_t_floor );


    standard_npc worker( "Worker", camp_pos, {}, 0, 8, 8, 8, 8 );
    worker.set_fac( faction_your_followers );
    worker.set_pos_abs_only( camp_abs );
    worker.wear_item( item( itype_backpack ) );


    here.add_item_or_charges( camp_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( camp_pos, itype_test_apple, std::nullopt ) == 1 );


    worker.assign_activity( zone_sort_activity_actor() );
    process_activity( worker );


    CHECK( count_items_or_charges( camp_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( camp_dest_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( count_items_or_charges( personal_dest_pos, itype_test_apple, std::nullopt ) == 0 );
}



TEST_CASE( "npc_zone_sorting_works_when_personal_overlaps_static",
           "[zones][items][activities][sorting][npc]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    zone_manager &zm = zone_manager::get_manager();
    zm.clear();


    const tripoint_bub_ms camp_pos = tripoint_bub_ms::zero + tripoint::east;
    const tripoint_abs_ms camp_abs = here.get_abs( camp_pos );


    const tripoint_bub_ms camp_dest_pos = camp_pos + tripoint::east;
    const tripoint_abs_ms camp_dest_abs = here.get_abs( camp_dest_pos );
    here.ter_set( camp_dest_pos, ter_t_floor );

    create_tile_zone( "Camp Unsorted", zone_type_LOOT_UNSORTED, camp_abs );
    create_tile_zone( "Camp Food", zone_type_LOOT_FOOD, camp_dest_abs );



    const tripoint_bub_ms avatar_pos = camp_dest_pos + tripoint::south;
    const tripoint_abs_ms avatar_abs = here.get_abs( avatar_pos );
    dummy.set_pos_abs_only( avatar_abs );


    const tripoint_rel_ms personal_dest_rel( 0, -1, 0 );
    zm.add( "Personal Food", zone_type_LOOT_FOOD, faction_your_followers,
            false, true, personal_dest_rel, personal_dest_rel, nullptr );
    zm.cache_avatar_location();


    standard_npc worker( "Worker", camp_pos, {}, 0, 8, 8, 8, 8 );
    worker.set_fac( faction_your_followers );
    worker.set_pos_abs_only( camp_abs );
    worker.wear_item( item( itype_backpack ) );


    here.add_item_or_charges( camp_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( camp_pos, itype_test_apple, std::nullopt ) == 1 );



    worker.assign_activity( zone_sort_activity_actor() );
    process_activity( worker );

    CHECK( count_items_or_charges( camp_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( camp_dest_pos, itype_test_apple, std::nullopt ) == 1 );
}



TEST_CASE( "npc_zone_sorting_ignores_personal_unsorted_source",
           "[zones][items][activities][sorting][npc]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    zone_manager &zm = zone_manager::get_manager();
    zm.clear();


    const tripoint_bub_ms avatar_pos = tripoint_bub_ms::zero + tripoint::east;
    const tripoint_abs_ms avatar_abs = here.get_abs( avatar_pos );
    dummy.set_pos_abs_only( avatar_abs );

    zm.add( "Personal Unsorted", zone_type_LOOT_UNSORTED, faction_your_followers,
            false, true, tripoint_rel_ms::zero, tripoint_rel_ms::zero, nullptr );
    zm.cache_avatar_location();


    const tripoint_bub_ms dest_pos = avatar_pos + tripoint( 3, 0, 0 );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food Dest", zone_type_LOOT_FOOD, here.get_abs( dest_pos ) );


    standard_npc worker( "Worker", avatar_pos, {}, 0, 8, 8, 8, 8 );
    worker.set_fac( faction_your_followers );
    worker.set_pos_abs_only( avatar_abs );
    worker.wear_item( item( itype_backpack ) );


    here.add_item_or_charges( avatar_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( avatar_pos, itype_test_apple, std::nullopt ) == 1 );


    worker.assign_activity( zone_sort_activity_actor() );
    process_activity( worker );

    CHECK( count_items_or_charges( avatar_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 0 );
}


TEST_CASE( "vehicle_zone_refresh_preserves_personal_zone_positions",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager &zm = zone_manager::get_manager();
    zm.clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();
    here.ter_set( start_pos, ter_t_floor );

    const tripoint_abs_ms start_abs = here.get_abs( start_pos );


    zm.add( "Unsorted", zone_type_LOOT_UNSORTED, faction_your_followers,
            false, true,
            tripoint_rel_ms( -2, -2, 0 ), tripoint_rel_ms( 2, 2, 0 ) );


    zm.add( "Food", zone_type_LOOT_FOOD, faction_your_followers,
            false, true,
            tripoint_rel_ms( 2, 0, 0 ), tripoint_rel_ms( 2, 0, 0 ) );



    const tripoint_bub_ms veh_pos = start_pos + tripoint( 0, -3, 0 );
    here.ter_set( veh_pos, ter_t_floor );
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      veh_pos, 0_degrees, 0, veh_spawn_status::UNDAMAGED );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    const tripoint_abs_ms veh_abs = here.get_abs( veh_pos );
    create_tile_zone( "VehUnsorted", zone_type_LOOT_UNSORTED, veh_abs, true );
    REQUIRE( zm.has_vehicle( zone_type_LOOT_UNSORTED, veh_abs, faction_your_followers ) );

    zm.cache_data();
    zm.cache_vzones();

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    const tripoint_abs_ms food_abs = start_abs + tripoint( 2, 0, 0 );
    REQUIRE( zm.has( zone_type_LOOT_FOOD, food_abs, faction_your_followers ) );



    const tripoint_bub_ms moved_pos = start_pos + tripoint( 0, 5, 0 );
    here.ter_set( moved_pos, ter_t_floor );
    dummy.setpos( here, moved_pos );
    zm.cache_avatar_location();

    cart->zones_dirty = true;
    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );
    if( here.check_vehicle_zones( 0 ) ) {
        zm.cache_vzones();
    }


    CHECK( zm.has( zone_type_LOOT_FOOD, food_abs, faction_your_followers ) );


    const tripoint_abs_ms shifted_food = here.get_abs( moved_pos ) + tripoint( 2, 0, 0 );
    if( shifted_food != food_abs ) {
        CHECK_FALSE( zm.has( zone_type_LOOT_FOOD, shifted_food, faction_your_followers ) );
    }
}




static bool load_cart_to_drag_edge( avatar &dummy, map &here, vehicle &cart,
                                    const tripoint_bub_ms &cart_pos,
                                    const tripoint_abs_ms &dest_abs )
{
    const std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    CAPTURE( cargo.has_value() );
    if( !cargo ) {
        return false;
    }
    const std::vector<tripoint_abs_ms> dropoffs = { dest_abs };
    const int arm_str = dummy.get_arm_str();
    for( int loaded = 0; loaded < 200; loaded++ ) {


        const std::optional<tripoint_bub_ms> worst =
            zone_sorting::worst_drag_tile_on_route( dummy, dropoffs );
        if( !worst ) {
            CAPTURE( loaded );
            return false;
        }
        const units::mass mass = cart.total_mass( here );
        const int req_one = cart.drag_str_req_at( here, *worst, mass + 10_kilogram );
        const int req_two = cart.drag_str_req_at( here, *worst, mass + 20_kilogram );
        if( req_one <= arm_str && req_two > arm_str ) {
            return true;
        }
        if( req_one > arm_str ) {
            CAPTURE( loaded );
            CAPTURE( req_one );
            CAPTURE( mass.value() );
            return false;
        }
        if( !cart.add_item( here, cargo->part(), item( itype_test_heavy_boulder ) ) ) {
            CAPTURE( loaded );
            CAPTURE( mass.value() );
            return false;
        }
    }
    return false;
}

TEST_CASE( "zone_sorting_adjacent_sources_do_not_oscillate",
           "[zones][items][activities][sorting][batching][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();



    dummy.set_str_base( 4 );
    dummy.set_str_bonus( 0 );

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();
    dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );

    const tripoint_bub_ms cart_pos = start_pos + tripoint::east;
    vehicle *cart = setup_grabbed_cart( dummy, here, start_pos, tripoint_rel_ms::east );



    const tripoint_bub_ms src_a = start_pos + tripoint::south;
    const tripoint_bub_ms src_b = src_a + tripoint::west;
    const tripoint_bub_ms dest_pos = start_pos + tripoint( 0, 8, 0 );
    for( const tripoint_bub_ms &p : {
             start_pos, src_a, src_b, dest_pos
         } ) {
        here.ter_set( p, ter_t_floor );
    }
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    create_tile_zone( "Unsorted A", zone_type_LOOT_UNSORTED, here.get_abs( src_a ) );
    create_tile_zone( "Unsorted B", zone_type_LOOT_UNSORTED, here.get_abs( src_b ) );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    const int per_tile = 4;
    for( int i = 0; i < per_tile; i++ ) {
        here.add_item_or_charges( src_a, item( itype_test_heavy_boulder ) );
        here.add_item_or_charges( src_b, item( itype_test_heavy_boulder ) );
    }

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    CAPTURE( dummy.get_arm_str() );
    REQUIRE( load_cart_to_drag_edge( dummy, here, *cart, cart_pos, dest_abs ) );
    CAPTURE( cart->total_mass( here ).value() );

    dummy.assign_activity( zone_sort_activity_actor() );



    REQUIRE( process_activity_bounded( dummy, 100, 2000 ) );
}

TEST_CASE( "zone_custom_filter_query_spans_the_whole_zone",
           "[zones][items][sorting]" )
{
    clear_map_without_vision();
    map &here = get_map();
    zone_manager &zmgr = zone_manager::get_manager();
    zmgr.clear();

    const int zone_width = 4;
    const tripoint_abs_ms zone_start = here.get_abs( tripoint_bub_ms{ 5, 5, 0 } );
    const tripoint_abs_ms zone_end = zone_start + tripoint( zone_width - 1, 0, 0 );
    const tripoint_abs_ms where = here.get_abs( tripoint_bub_ms::zero );

    const item hammer( itype_hammer );
    const item apple( itype_test_apple );

    auto tiles_for = [&zmgr, &where]( const item & it ) {
        return zmgr.get_near( zone_type_LOOT_CUSTOM, where, MAX_VIEW_DISTANCE, &it );
    };

    mapgen_place_zone( zone_start, zone_end, zone_type_LOOT_CUSTOM, faction_your_followers, {},
                       "hammer" );

    const std::unordered_set<tripoint_abs_ms> hammer_tiles = tiles_for( hammer );
    CHECK( hammer_tiles.size() == static_cast<size_t>( zone_width ) );
    for( int i = 0; i < zone_width; i++ ) {
        CAPTURE( i );
        CHECK( hammer_tiles.count( zone_start + tripoint( i, 0, 0 ) ) == 1 );
    }
    CHECK( tiles_for( apple ).empty() );


    CHECK( tiles_for( hammer ) == hammer_tiles );
    CHECK( tiles_for( apple ).empty() );


    zmgr.clear();
    mapgen_place_zone( zone_start, zone_end, zone_type_LOOT_CUSTOM, faction_your_followers, {},
                       "hammer" );
    CHECK( tiles_for( hammer ).size() == static_cast<size_t>( zone_width ) );


    zmgr.clear();
    mapgen_place_zone( zone_start, zone_end, zone_type_LOOT_CUSTOM, faction_your_followers, {},
                       "apple" );
    CHECK( tiles_for( hammer ).empty() );
    CHECK( tiles_for( apple ).size() == static_cast<size_t>( zone_width ) );
}




static vehicle *setup_drag_limited_sort( avatar &dummy, map &here,
        const tripoint_bub_ms &start_pos, int boulders )
{
    dummy.set_str_base( 4 );
    dummy.set_str_bonus( 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    const tripoint_bub_ms cart_pos = start_pos + tripoint::east;
    vehicle *cart = setup_grabbed_cart( dummy, here, start_pos, tripoint_rel_ms::east );

    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_bub_ms dest_pos = start_pos + tripoint( 0, 8, 0 );
    here.ter_set( start_pos, ter_t_floor );
    here.ter_set( src_pos, ter_t_floor );
    here.ter_set( dest_pos, ter_t_floor );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, here.get_abs( src_pos ) );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );
    for( int i = 0; i < boulders; i++ ) {
        here.add_item_or_charges( src_pos, item( itype_test_heavy_boulder ) );
    }

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    REQUIRE( load_cart_to_drag_edge( dummy, here, *cart, cart_pos, dest_abs ) );
    return cart;
}

static int count_carried( const Character &who, const itype_id &id )
{
    int carried = 0;
    who.visit_items( [&carried, &id]( const item_location & it ) {
        if( it->typeId() == id ) {
            carried++;
        }
        return VisitResponse::NEXT;
    } );
    return carried;
}

TEST_CASE( "zone_sorting_drag_refused_items_reach_the_inventory",
           "[zones][items][activities][sorting][vehicle][weight]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const int boulders = 3;

    SECTION( "into a worn container" ) {
        dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );
        setup_drag_limited_sort( dummy, here, start_pos, boulders );

        dummy.assign_activity( zone_sort_activity_actor() );
        REQUIRE( process_activity_bounded( dummy, 100, 2000 ) );

        const int at_source = count_items_or_charges( src_pos, itype_test_heavy_boulder,
                              std::nullopt );
        const int carried = count_carried( dummy, itype_test_heavy_boulder );
        CAPTURE( at_source );
        CAPTURE( carried );


        CHECK( carried > 0 );
        CHECK( at_source < boulders - 1 );
    }

    SECTION( "by wielding when there is no container" ) {
        REQUIRE( dummy.worn.empty() );
        setup_drag_limited_sort( dummy, here, start_pos, boulders );
        REQUIRE_FALSE( dummy.can_stash( item( itype_test_heavy_boulder ) ) );

        dummy.assign_activity( zone_sort_activity_actor() );
        REQUIRE( process_activity_bounded( dummy, 100, 2000 ) );

        const int at_source = count_items_or_charges( src_pos, itype_test_heavy_boulder,
                              std::nullopt );
        CAPTURE( at_source );
        CHECK( at_source < boulders - 1 );
    }

    SECTION( "but not past the carry limit" ) {
        dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );
        setup_drag_limited_sort( dummy, here, start_pos, boulders );


        for( int i = 0; i < 100 && dummy.weight_carried() <= dummy.weight_capacity(); i++ ) {
            const units::mass before = dummy.weight_carried();
            dummy.i_add( item( itype_test_heavy_boulder ) );
            if( dummy.weight_carried() <= before ) {
                break;
            }
        }
        REQUIRE( dummy.weight_carried() > dummy.weight_capacity() );
        const int carried_before = count_carried( dummy, itype_test_heavy_boulder );

        dummy.assign_activity( zone_sort_activity_actor() );
        REQUIRE( process_activity_bounded( dummy, 100, 2000 ) );

        CHECK( count_carried( dummy, itype_test_heavy_boulder ) == carried_before );
        CHECK( count_items_or_charges( src_pos, itype_test_heavy_boulder,
                                       std::nullopt ) == boulders - 1 );
    }
}

namespace
{



class stalling_zone_actor : public zone_activity_actor
{
    public:
        stalling_zone_actor() : zone_activity_actor( 0 ) {}

        const activity_id &get_type() const override {
            return ACT_MOVE_LOOT;
        }
        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<stalling_zone_actor>( *this );
        }

        void stage_init( player_activity &, Character & ) override {
            stage = DO;
        }
        bool stage_think( player_activity &, Character & ) override {
            stage = DO;
            return true;
        }
        void stage_do( player_activity &, Character & ) override {

        }
        void on_no_progress( Character & ) override {
            recovered = true;
            recovery_stage = stage;
        }

        bool recovered = false;
        zone_activity_stage recovery_stage = UNINIT;
};

stalling_zone_actor *stalling_actor_of( player_activity &act )
{
    return dynamic_cast<stalling_zone_actor *>( act.actor.get() );
}




class replacing_zone_actor : public zone_activity_actor
{
    public:
        replacing_zone_actor() : zone_activity_actor( 0 ) {}

        const activity_id &get_type() const override {
            return ACT_MOVE_LOOT;
        }
        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<replacing_zone_actor>( *this );
        }

        void stage_init( player_activity &, Character & ) override {
            stage = DO;
        }
        bool stage_think( player_activity &, Character & ) override {
            stage = DO;
            return true;
        }
        void stage_do( player_activity &, Character &you ) override {
            you.assign_activity( stalling_zone_actor() );
        }
};




class stall_count_probe_actor : public zone_sort_activity_actor
{
    public:
        static constexpr int preset_count = 5;

        const activity_id &get_type() const override {
            return ACT_MOVE_LOOT;
        }
        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<stall_count_probe_actor>( *this );
        }

        void stage_do( player_activity &act, Character &you ) override {
            zero_move_dispatches = preset_count;
            zone_sort_activity_actor::stage_do( act, you );
        }

        int stall_count() const {
            return zero_move_dispatches;
        }
};
}

TEST_CASE( "zone_activity_zero_move_passes_are_bounded",
           "[zones][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();


    const int budget = 8;

    SECTION( "recovery runs while the stage is still DO" ) {
        player_activity act{ stalling_zone_actor{} };
        REQUIRE( stalling_actor_of( act ) != nullptr );
        dummy.set_moves( 100 );
        const int moves_before = dummy.get_moves();

        for( int i = 0; i < budget + 2; i++ ) {
            act.do_turn( dummy );
        }

        const stalling_zone_actor *actor = stalling_actor_of( act );
        REQUIRE( actor != nullptr );
        CHECK( actor->recovered );
        CHECK( actor->recovery_stage == DO );
        CHECK( dummy.get_moves() < moves_before );
    }

    SECTION( "a staged batch goes back to its source" ) {
        map &here = get_map();
        const tripoint_bub_ms start_pos( 60, 60, 0 );
        dummy.setpos( here, start_pos );
        dummy.clear_destination();
        dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );

        const tripoint_bub_ms src_pos = start_pos + tripoint::south;
        const tripoint_bub_ms dest_pos = start_pos + tripoint( 0, 8, 0 );
        build_open_area( here, start_pos, 12 );
        create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, here.get_abs( src_pos ) );
        create_tile_zone( "Food", zone_type_LOOT_FOOD, here.get_abs( dest_pos ) );
        here.add_item_or_charges( src_pos, item( itype_test_apple ) );
        here.invalidate_map_cache( 0 );
        here.build_map_cache( 0, true );

        dummy.assign_activity( zone_sort_activity_actor() );


        zone_sort_activity_actor *actor = nullptr;
        for( int i = 0; i < 20 && dummy.activity; i++ ) {
            dummy.set_moves( 1 );
            dummy.activity.do_turn( dummy );
            actor = dynamic_cast<zone_sort_activity_actor *>( dummy.activity.actor.get() );
            if( actor != nullptr && !actor->get_dropoff_coords().empty() &&
                dummy.has_amount( itype_test_apple, 1 ) ) {
                break;
            }
        }
        REQUIRE( actor != nullptr );
        REQUIRE( !actor->get_dropoff_coords().empty() );
        REQUIRE( dummy.has_amount( itype_test_apple, 1 ) );
        REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );

        actor->on_no_progress( dummy );

        CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
        CHECK_FALSE( dummy.has_amount( itype_test_apple, 1 ) );
    }

    SECTION( "a pass that swaps in another activity is not followed up" ) {
        dummy.set_moves( 100 );
        dummy.assign_activity( replacing_zone_actor() );
        REQUIRE( dummy.activity.id() == ACT_MOVE_LOOT );
        const activity_actor *const dispatched = dummy.activity.actor.get();


        dummy.activity.do_turn( dummy );

        CHECK( dummy.activity.actor.get() != dispatched );
        CHECK( dynamic_cast<stalling_zone_actor *>( dummy.activity.actor.get() ) != nullptr );
    }

    SECTION( "routing hands over a cleared count" ) {
        map &here = get_map();
        const tripoint_bub_ms start_pos( 60, 60, 0 );
        dummy.setpos( here, start_pos );
        dummy.clear_destination();
        dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );

        const tripoint_bub_ms src_pos = start_pos + tripoint::south;
        const tripoint_bub_ms dest_pos = start_pos + tripoint( 0, 8, 0 );
        build_open_area( here, start_pos, 12 );
        create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, here.get_abs( src_pos ) );
        create_tile_zone( "Food", zone_type_LOOT_FOOD, here.get_abs( dest_pos ) );
        here.add_item_or_charges( src_pos, item( itype_test_apple ) );
        here.invalidate_map_cache( 0 );
        here.build_map_cache( 0, true );

        dummy.assign_activity( stall_count_probe_actor() );
        for( int i = 0; i < 40 && dummy.activity && !dummy.has_destination(); i++ ) {
            dummy.mod_moves( dummy.get_speed() );
            while( dummy.get_moves() > 0 && dummy.activity ) {
                dummy.activity.do_turn( dummy );
            }
        }


        REQUIRE( dummy.has_destination() );
        REQUIRE_FALSE( dummy.peek_destination_activity().is_null() );
        const stall_count_probe_actor *saved =
            dynamic_cast<const stall_count_probe_actor *>(
                dummy.peek_destination_activity().actor.get() );
        REQUIRE( saved != nullptr );
        CHECK( saved->stall_count() == 0 );
    }

    SECTION( "count does not persist between turns" ) {
        player_activity act{ stalling_zone_actor{} };
        dummy.set_moves( 100 );

        for( int i = 0; i < budget - 2; i++ ) {
            act.do_turn( dummy );
        }
        REQUIRE( stalling_actor_of( act ) != nullptr );
        REQUIRE_FALSE( stalling_actor_of( act )->recovered );

        calendar::turn += 1_turns;
        for( int i = 0; i < budget - 2; i++ ) {
            act.do_turn( dummy );
        }

        const stalling_zone_actor *actor = stalling_actor_of( act );
        REQUIRE( actor != nullptr );
        CHECK_FALSE( actor->recovered );
    }
}
