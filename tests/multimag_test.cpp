#include <algorithm>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "avatar_action.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_utility.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "debug.h"
#include "flag.h"
#include "flat_set.h"
#include "flexbuffer_json.h"
#include "gun_mode.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "item_pocket.h"
#include "inventory_ui.h"
#include "iteminfo_query.h"
#include "itype.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "messages.h"
#include "options_helpers.h"
#include "output.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "temp_crafting_inventory.h"
#include "type_id.h"
#include "units.h"
#include "visitable.h"

static const ammotype ammo_9mm( "9mm" );
static const ammotype ammo_battery( "battery" );

static const flag_id json_flag_CASING( "CASING" );

static const gun_mode_id gun_mode_AUTO( "AUTO" );
static const gun_mode_id gun_mode_BURST( "BURST" );
static const gun_mode_id gun_mode_DEFAULT( "DEFAULT" );
static const gun_mode_id gun_mode_EXTRA( "EXTRA" );
static const gun_mode_id gun_mode_REGULATED( "REGULATED" );
static const gun_mode_id gun_mode_TGMODE( "TGMODE" );

static const item_group_id Item_spawn_data_test_multimag_full_load( "test_multimag_full_load" );

static const itype_id itype_38_special( "38_special" );
static const itype_id itype_556( "556" );
static const itype_id itype_9mm( "9mm" );
static const itype_id itype_9mm_casing( "9mm_casing" );
static const itype_id itype_UPS_ON( "UPS_ON" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_book_binder( "book_binder" );
static const itype_id itype_flashlight( "flashlight" );
static const itype_id itype_glock_19( "glock_19" );
static const itype_id itype_glockmag( "glockmag" );
static const itype_id itype_heavy_battery_cell( "heavy_battery_cell" );
static const itype_id itype_medium_battery_cell( "medium_battery_cell" );
static const itype_id itype_oxygen( "oxygen" );
static const itype_id itype_paper( "paper" );
static const itype_id itype_robofac_gun( "robofac_gun" );
static const itype_id itype_stanag30( "stanag30" );
static const itype_id itype_sw_619( "sw_619" );
static const itype_id itype_test_multimag_add_extra_mode_mod( "test_multimag_add_extra_mode_mod" );
static const itype_id itype_test_multimag_aux_gun( "test_multimag_aux_gun" );
static const itype_id itype_test_multimag_bad_pocket_gunmod( "test_multimag_bad_pocket_gunmod" );
static const itype_id itype_test_multimag_capacity_gunmod( "test_multimag_capacity_gunmod" );
static const itype_id
itype_test_multimag_capacity_well_gunmod( "test_multimag_capacity_well_gunmod" );
static const itype_id itype_test_multimag_consume_gunmod( "test_multimag_consume_gunmod" );
static const itype_id
itype_test_multimag_consume_gunmod_mult( "test_multimag_consume_gunmod_mult" );
static const itype_id itype_test_multimag_consume_toolmod( "test_multimag_consume_toolmod" );
static const itype_id itype_test_multimag_gun( "test_multimag_gun" );
static const itype_id itype_test_multimag_gun_consume( "test_multimag_gun_consume" );
static const itype_id itype_test_multimag_gun_integral_ammo( "test_multimag_gun_integral_ammo" );
static const itype_id itype_test_multimag_gun_same_type( "test_multimag_gun_same_type" );
static const itype_id itype_test_multimag_hide_all_mod( "test_multimag_hide_all_mod" );
static const itype_id
itype_test_multimag_hide_extra_mode_mod( "test_multimag_hide_extra_mode_mod" );
static const itype_id itype_test_multimag_regulated_mod( "test_multimag_regulated_mod" );
static const itype_id itype_test_multimag_tool_consume( "test_multimag_tool_consume" );
static const itype_id itype_test_multimag_tool_factor( "test_multimag_tool_factor" );
static const itype_id itype_test_multimag_tool_gunmod( "test_multimag_tool_gunmod" );
static const itype_id itype_test_multimag_turret_gun( "test_multimag_turret_gun" );
static const itype_id itype_test_multimag_vehicle_welder( "test_multimag_vehicle_welder" );
static const itype_id itype_welding_wire_steel_tests_dupe( "welding_wire_steel_tests_dupe" );

static item make_loaded_glock()
{
    item mag( itype_glockmag );
    mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
    item gun( itype_glock_19 );
    gun.put_in( mag, pocket_type::MAGAZINE_WELL );
    return gun;
}

static item make_loaded_stanag()
{
    item mag( itype_stanag30 );
    mag.put_in( item( itype_556, calendar::turn, 30 ), pocket_type::MAGAZINE );
    return mag;
}

static item make_dual_well_gun()
{
    item gun( itype_test_multimag_gun );
    item glock_mag( itype_glockmag );
    glock_mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
    gun.put_in( glock_mag, pocket_type::MAGAZINE_WELL );
    gun.put_in( make_loaded_stanag(), pocket_type::MAGAZINE_WELL );
    return gun;
}

TEST_CASE( "single_well_magazine_operations", "[multimag]" )
{
    SECTION( "ammo_remaining and magazine_current" ) {
        item gun = make_loaded_glock();

        CHECK( gun.ammo_remaining() == 15 );
        REQUIRE( gun.magazine_current() != nullptr );
        CHECK( gun.magazine_current()->typeId() == itype_glockmag );
    }

    SECTION( "ammo_consume drains loaded magazine" ) {
        clear_map();
        map &here = get_map();
        tripoint_bub_ms pos( tripoint_bub_ms::zero );
        item gun = make_loaded_glock();

        CHECK( gun.ammo_consume( 3, here, pos, nullptr ) == 3 );
        CHECK( gun.ammo_remaining() == 12 );
        CHECK( gun.ammo_consume( 12, here, pos, nullptr ) == 12 );
        CHECK( gun.ammo_remaining() == 0 );
    }

    SECTION( "first_ammo returns loaded ammo type" ) {
        item gun = make_loaded_glock();
        CHECK( gun.first_ammo().typeId() == itype_9mm );
    }

    SECTION( "first_ammo on empty gun" ) {
        item gun( itype_glock_19 );
        CHECK( gun.first_ammo().is_null() );
    }

    SECTION( "magazines_current" ) {
        item gun = make_loaded_glock();
        std::vector<item *> mags = gun.magazines_current();
        REQUIRE( mags.size() == 1 );
        CHECK( mags[0] == gun.magazine_current() );

        item empty_gun( itype_glock_19 );
        CHECK( empty_gun.magazines_current().empty() );
    }
}

TEST_CASE( "integral_magazine_ammo_consume", "[multimag]" )
{
    clear_map();
    map &here = get_map();
    tripoint_bub_ms pos( tripoint_bub_ms::zero );

    item gun( itype_sw_619 );
    gun.put_in( item( itype_38_special, calendar::turn, 7 ), pocket_type::MAGAZINE );

    CHECK( gun.ammo_remaining() == 7 );
    CHECK( gun.ammo_consume( 3, here, pos, nullptr ) == 3 );
    CHECK( gun.ammo_remaining() == 4 );
    CHECK( gun.first_ammo().typeId() == itype_38_special );
}

TEST_CASE( "dual_well_magazine_operations", "[multimag]" )
{
    SECTION( "magazines_current with both loaded" ) {
        item gun = make_dual_well_gun();
        CHECK( gun.magazines_current().size() == 2 );
    }

    SECTION( "magazines_current with only second loaded" ) {
        item gun( itype_test_multimag_gun );
        gun.put_in( make_loaded_stanag(), pocket_type::MAGAZINE_WELL );

        std::vector<item *> mags = gun.magazines_current();
        REQUIRE( mags.size() == 1 );
        REQUIRE( gun.magazine_current() != nullptr );
        CHECK( mags[0] == gun.magazine_current() );
    }

    SECTION( "ammo_consume with first well empty" ) {
        clear_map();
        map &here = get_map();
        tripoint_bub_ms pos( tripoint_bub_ms::zero );

        item gun( itype_test_multimag_gun );
        gun.put_in( make_loaded_stanag(), pocket_type::MAGAZINE_WELL );

        CHECK( gun.ammo_remaining() == 30 );
        CHECK( gun.ammo_consume( 10, here, pos, nullptr ) == 10 );
        CHECK( gun.ammo_remaining() == 20 );
    }

    SECTION( "ammo_consume drains both wells" ) {
        clear_map();
        map &here = get_map();
        tripoint_bub_ms pos( tripoint_bub_ms::zero );

        item gun = make_dual_well_gun();

        CHECK( gun.ammo_consume( 20, here, pos, nullptr ) == 20 );
        std::vector<item *> remaining = gun.magazines_current();
        REQUIRE( remaining.size() == 2 );
        CHECK( remaining[0]->ammo_remaining() == 0 );
        CHECK( remaining[1]->ammo_remaining() == 25 );
    }

    SECTION( "first_ammo with first well empty" ) {
        item gun( itype_test_multimag_gun );
        gun.put_in( make_loaded_stanag(), pocket_type::MAGAZINE_WELL );
        CHECK( gun.first_ammo().typeId() == itype_556 );
    }

    SECTION( "first_ammo with both wells empty" ) {
        item gun( itype_test_multimag_gun );
        CHECK( gun.first_ammo().is_null() );
    }
}

TEST_CASE( "rate_action_unload_across_wells", "[multimag][hint]" )
{
    clear_avatar();
    Character &you = get_player_character();

    SECTION( "good when only second well has a mag" ) {
        item gun( itype_test_multimag_gun );

        gun.put_in( make_loaded_stanag(), pocket_type::MAGAZINE_WELL );
        REQUIRE( gun.magazines_current().size() == 1 );
        CHECK( you.rate_action_unload( gun ) == hint_rating::good );
    }

    SECTION( "good when both wells loaded" ) {
        item gun = make_dual_well_gun();
        CHECK( you.rate_action_unload( gun ) == hint_rating::good );
    }

    SECTION( "iffy when no wells loaded but item has ammo capacity" ) {
        item gun( itype_test_multimag_gun );
        CHECK( you.rate_action_unload( gun ) == hint_rating::iffy );
    }
}

TEST_CASE( "display_name_multi_well_per_well_counts", "[multimag][display]" )
{
    SECTION( "single loaded well prints (amount/max ammo) once" ) {
        item gun = make_loaded_glock();
        const std::string name = remove_color_tags( gun.display_name() );
        CAPTURE( name );
        CHECK( name.find( "(15/15" ) != std::string::npos );
    }

    SECTION( "dual loaded wells: per-well counts in single paren" ) {
        item gun = make_dual_well_gun();
        const std::string name = remove_color_tags( gun.display_name() );
        CAPTURE( name );

        const size_t open = name.find( '(' );
        const size_t close = name.find( ')', open );
        REQUIRE( open != std::string::npos );
        REQUIRE( close != std::string::npos );
        const std::string inner = name.substr( open + 1, close - open - 1 );
        CAPTURE( inner );
        CHECK( inner.find( "15/15" ) != std::string::npos );
        CHECK( inner.find( "30/30" ) != std::string::npos );
        CHECK( inner.find( ',' ) != std::string::npos );
    }

    SECTION( "one well loaded out of two still emits per-well segments" ) {
        override_option opt( "AMMO_IN_NAMES", "false" );
        item gun( itype_test_multimag_gun );
        gun.put_in( make_loaded_stanag(), pocket_type::MAGAZINE_WELL );
        const std::string name = remove_color_tags( gun.display_name() );
        CAPTURE( name );
        const size_t open = name.find( '(' );
        const size_t close = name.find( ')', open );
        REQUIRE( open != std::string::npos );
        REQUIRE( close != std::string::npos );
        const std::string inner = name.substr( open + 1, close - open - 1 );
        CHECK( inner.find( "0/15" ) != std::string::npos );
        CHECK( inner.find( "30/30" ) != std::string::npos );
        CHECK( inner.find( ',' ) != std::string::npos );
    }

    SECTION( "dual loaded wells use variant ammo names, not generic class names" ) {
        override_option opt( "AMMO_IN_NAMES", "true" );
        item gun = make_dual_well_gun();
        const std::string name = remove_color_tags( gun.display_name() );
        CAPTURE( name );

        const std::string variant_9mm = item::find_type( itype_9mm )->nname( 1 );
        const std::string variant_556 = item::find_type( itype_556 )->nname( 1 );
        CHECK( name.find( variant_9mm ) != std::string::npos );
        CHECK( name.find( variant_556 ) != std::string::npos );
    }

    SECTION( "AMMO_IN_NAMES off: dual wells still label per-well ammo" ) {



        override_option opt( "AMMO_IN_NAMES", "false" );
        item gun = make_dual_well_gun();
        const std::string name = remove_color_tags( gun.display_name() );
        CAPTURE( name );
        const std::string variant_9mm = item::find_type( itype_9mm )->nname( 1 );
        const std::string variant_556 = item::find_type( itype_556 )->nname( 1 );
        CHECK( name.find( variant_9mm ) != std::string::npos );
        CHECK( name.find( variant_556 ) != std::string::npos );
        CHECK( name.find( "15/15" ) != std::string::npos );
        CHECK( name.find( "30/30" ) != std::string::npos );
    }

    SECTION( "bare HWP with null gun ammotype and no barrel mod does not crash" ) {


        item gun( itype_robofac_gun );
        REQUIRE( gun.is_gun() );
        REQUIRE( !gun.ammo_types().empty() );
        REQUIRE( gun.ammo_data() == nullptr );
        std::string name;
        name = remove_color_tags( gun.display_name() );
        CAPTURE( name );
        CHECK_FALSE( name.empty() );
    }
}

TEST_CASE( "iteminfo_magazine_label_pluralization", "[multimag][display]" )
{
    auto iteminfo_for = []( const item & it ) {
        std::vector<iteminfo> info_v;
        const std::vector<iteminfo_parts> parts = {
            iteminfo_parts::GUN_MAGAZINE,
            iteminfo_parts::TOOL_MAGAZINE_CURRENT
        };
        const iteminfo_query query_v( parts );
        it.info( info_v, &query_v, 1 );
        return format_item_info( info_v, {} );
    };

    SECTION( "single loaded: singular 'Magazine:' label" ) {
        item gun = make_loaded_glock();
        const std::string info = iteminfo_for( gun );
        CAPTURE( info );
        CHECK( info.find( "Magazine: " ) != std::string::npos );
        CHECK( info.find( "Magazines: " ) == std::string::npos );

        CHECK( info.find( item( itype_glockmag ).tname() ) != std::string::npos );
    }

    SECTION( "dual loaded: plural 'Magazines:' label, both names listed" ) {
        item gun = make_dual_well_gun();
        const std::string info = iteminfo_for( gun );
        CAPTURE( info );
        CHECK( info.find( "Magazines: " ) != std::string::npos );
        const std::string glock_name = item( itype_glockmag ).tname();
        const std::string stanag_name = item( itype_stanag30 ).tname();
        CHECK( info.find( glock_name ) != std::string::npos );
        CHECK( info.find( stanag_name ) != std::string::npos );

        const size_t glock_pos = info.find( glock_name );
        const size_t stanag_pos = info.find( stanag_name );
        if( glock_pos != std::string::npos && stanag_pos != std::string::npos ) {
            const size_t lo = std::min( glock_pos, stanag_pos );
            const size_t hi = std::max( glock_pos, stanag_pos );
            CHECK( info.substr( lo, hi - lo ).find( ',' ) != std::string::npos );
        }
    }

    SECTION( "single loaded tool: tool_info path uses singular 'Magazine:'" ) {
        item flashlight( itype_flashlight );
        flashlight.put_in( item( itype_medium_battery_cell ), pocket_type::MAGAZINE_WELL );
        const std::string info = iteminfo_for( flashlight );
        CAPTURE( info );
        CHECK( info.find( "Magazine: " ) != std::string::npos );
        CHECK( info.find( "Magazines: " ) == std::string::npos );
        CHECK( info.find( item( itype_medium_battery_cell ).tname() ) != std::string::npos );
    }
}

TEST_CASE( "per_well_ammo_remaining_capacity_overloads",
           "[multimag][capacity]" )
{
    item gun = make_dual_well_gun();
    REQUIRE( gun.magazines_current().size() == 2 );


    {
        clear_map();
        map &here = get_map();
        const tripoint_bub_ms pos( tripoint_bub_ms::zero );
        REQUIRE( gun.ammo_consume( 5, here, pos, nullptr ) == 5 );
    }


    int glock_idx = -1;
    int stanag_idx = -1;
    {
        int idx = 0;
        for( const item_pocket *p : gun.get_pockets( []( const item_pocket & ) {
        return true;
    } ) ) {
            if( p->is_type( pocket_type::MAGAZINE_WELL ) ) {
                if( const item *mag = p->magazine_current() ) {
                    if( mag->typeId() == itype_glockmag ) {
                        glock_idx = idx;
                    } else if( mag->typeId() == itype_stanag30 ) {
                        stanag_idx = idx;
                    }
                }
            }
            ++idx;
        }
    }
    REQUIRE( glock_idx >= 0 );
    REQUIRE( stanag_idx >= 0 );

    SECTION( "per-well ammo_remaining returns each well's loaded count" ) {
        CHECK( gun.ammo_remaining( glock_idx ) == 10 );
        CHECK( gun.ammo_remaining( stanag_idx ) == 30 );

        CHECK( gun.ammo_remaining() == 40 );
    }

    SECTION( "per-well ammo_capacity returns each loaded magazine's capacity" ) {
        CHECK( gun.ammo_capacity( glock_idx ) == 15 );
        CHECK( gun.ammo_capacity( stanag_idx ) == 30 );
    }

    SECTION( "per-well remaining_ammo_capacity is per-mag, not item-aggregate" ) {
        CHECK( gun.remaining_ammo_capacity( glock_idx ) == 5 );
        CHECK( gun.remaining_ammo_capacity( stanag_idx ) == 0 );
    }

    SECTION( "out-of-range or non-well indices return 0" ) {
        CHECK( gun.ammo_remaining( -1 ) == 0 );
        CHECK( gun.ammo_remaining( 999 ) == 0 );
        CHECK( gun.ammo_capacity( -1 ) == 0 );
        CHECK( gun.remaining_ammo_capacity( -1 ) == 0 );
    }
}

TEST_CASE( "character_list_ammo_per_well_targets_flag", "[multimag][reload]" )
{
    clear_avatar();
    Character &you = get_player_character();


    you.wear_item( item( itype_backpack ) );

    SECTION( "default flag emits one option per item plus one per loaded magazine" ) {
        item gun = make_dual_well_gun();
        item_location gun_loc = you.i_add( gun );
        REQUIRE( gun_loc );

        you.i_add( item( itype_9mm, calendar::turn, 15 ) );
        you.i_add( item( itype_556, calendar::turn, 30 ) );

        std::vector<item::reload_option> ammo_list;
        you.list_ammo( gun_loc, ammo_list, true );


        for( const item::reload_option &opt : ammo_list ) {
            CHECK( opt.pocket_index < 0 );
        }
    }

    SECTION( "per_well_targets=true emits per-well options with stable pocket_index" ) {



        item gun( itype_test_multimag_gun );
        item_location gun_loc = you.i_add( gun );
        REQUIRE( gun_loc );
        item glock_extra( itype_glockmag );
        glock_extra.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
        item stanag_extra( itype_stanag30 );
        stanag_extra.put_in( item( itype_556, calendar::turn, 30 ), pocket_type::MAGAZINE );
        you.i_add( glock_extra );
        you.i_add( stanag_extra );


        std::vector<int> well_indices;
        {
            int idx = 0;
            for( const item_pocket *p : gun_loc->get_pockets(
            []( const item_pocket & ) {
            return true;
        } ) ) {
                if( p->is_type( pocket_type::MAGAZINE_WELL ) ) {
                    well_indices.push_back( idx );
                }
                ++idx;
            }
        }
        REQUIRE( well_indices.size() == 2 );

        std::vector<item::reload_option> ammo_list;
        you.list_ammo( gun_loc, ammo_list, true, true );

        std::set<int> emitted_pocket_indices;
        for( const item::reload_option &opt : ammo_list ) {
            emitted_pocket_indices.insert( opt.pocket_index );
        }

        for( int idx : well_indices ) {
            CHECK( emitted_pocket_indices.count( idx ) == 1 );
        }


        const item_pocket *first_well = nullptr;
        const item_pocket *second_well = nullptr;
        {
            int idx = 0;
            for( const item_pocket *p : gun_loc->get_pockets(
            []( const item_pocket & ) {
            return true;
        } ) ) {
                if( p->is_type( pocket_type::MAGAZINE_WELL ) ) {
                    if( idx == well_indices[0] ) {
                        first_well = p;
                    } else if( idx == well_indices[1] ) {
                        second_well = p;
                    }
                }
                ++idx;
            }
        }
        REQUIRE( first_well != nullptr );
        REQUIRE( second_well != nullptr );
        for( const item::reload_option &opt : ammo_list ) {
            if( opt.pocket_index < 0 ) {
                continue;
            }
            const item_pocket *picked = opt.pocket_index == well_indices[0] ? first_well : second_well;
            CAPTURE( opt.pocket_index );
            CAPTURE( opt.ammo->tname() );
            CHECK( picked->can_reload_with( *opt.ammo, true ) );
        }


        for( const item::reload_option &opt : ammo_list ) {
            if( opt.pocket_index >= 0 ) {
                CHECK( opt.qty() == 1 );
            }
        }
    }

    SECTION( "empty-well pocket_index is the position in contents, not a "
             "magazines_current() position" ) {

        item gun( itype_test_multimag_gun );
        gun.put_in( make_loaded_stanag(), pocket_type::MAGAZINE_WELL );
        item_location gun_loc = you.i_add( gun );
        REQUIRE( gun_loc );

        item glock_extra( itype_glockmag );
        glock_extra.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
        item stanag_extra( itype_stanag30 );
        stanag_extra.put_in( item( itype_556, calendar::turn, 30 ), pocket_type::MAGAZINE );
        you.i_add( glock_extra );
        you.i_add( stanag_extra );


        int empty_well_idx = -1;
        int loaded_well_idx = -1;
        {
            int idx = 0;
            for( const item_pocket *p : gun_loc->get_pockets(
            []( const item_pocket & ) {
            return true;
        } ) ) {
                if( p->is_type( pocket_type::MAGAZINE_WELL ) ) {
                    if( p->magazine_current() == nullptr ) {
                        empty_well_idx = idx;
                    } else {
                        loaded_well_idx = idx;
                    }
                }
                ++idx;
            }
        }
        REQUIRE( empty_well_idx >= 0 );
        REQUIRE( loaded_well_idx >= 0 );
        REQUIRE( empty_well_idx != loaded_well_idx );

        std::vector<item::reload_option> ammo_list;
        you.list_ammo( gun_loc, ammo_list, true, true );


        bool saw_empty = false;
        for( const item::reload_option &opt : ammo_list ) {
            if( opt.pocket_index == empty_well_idx ) {
                saw_empty = true;
            }
        }
        CHECK( saw_empty );
    }
}

TEST_CASE( "item_reload_with_pocket_index_targets_specific_well", "[multimag][reload]" )
{
    clear_avatar();
    Character &you = get_player_character();
    you.wear_item( item( itype_backpack ) );



    item gun( itype_test_multimag_gun );
    item glock_mag( itype_glockmag );
    glock_mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
    gun.put_in( glock_mag, pocket_type::MAGAZINE_WELL );

    item_location gun_loc = you.i_add( gun );
    REQUIRE( gun_loc );



    int loaded_well_idx = -1;
    int empty_well_idx = -1;
    {
        int idx = 0;
        for( const item_pocket *p : gun_loc->get_pockets(
        []( const item_pocket & ) {
        return true;
    } ) ) {
            if( p->is_type( pocket_type::MAGAZINE_WELL ) ) {
                if( p->magazine_current() != nullptr ) {
                    loaded_well_idx = idx;
                } else {
                    empty_well_idx = idx;
                }
            }
            ++idx;
        }
    }
    REQUIRE( loaded_well_idx >= 0 );
    REQUIRE( empty_well_idx >= 0 );

    SECTION( "pocket_index = empty stanag well inserts there, leaves glock untouched" ) {
        item replacement_stanag( itype_stanag30 );
        replacement_stanag.put_in( item( itype_556, calendar::turn, 10 ), pocket_type::MAGAZINE );
        item_location replacement_loc = you.i_add( replacement_stanag );
        REQUIRE( replacement_loc );

        const int glock_count_before = gun_loc->ammo_remaining( loaded_well_idx );
        REQUIRE( gun_loc->reload( you, replacement_loc, 1, empty_well_idx ) );
        CHECK( gun_loc->ammo_remaining( empty_well_idx ) == 10 );
        CHECK( gun_loc->ammo_remaining( loaded_well_idx ) == glock_count_before );
    }

    SECTION( "invalid pocket_index aborts reload" ) {
        item replacement_stanag( itype_stanag30 );
        replacement_stanag.put_in( item( itype_556, calendar::turn, 10 ), pocket_type::MAGAZINE );
        item_location replacement_loc = you.i_add( replacement_stanag );
        REQUIRE( replacement_loc );

        const int glock_count_before = gun_loc->ammo_remaining( loaded_well_idx );
        bool reload_result = true;
        const std::string dmsg = capture_debugmsg_during( [&]() {


            reload_result = gun_loc->reload( you, replacement_loc, 1, 99 );
        } );
        CHECK( !reload_result );
        CHECK( dmsg.find( "pocket_index" ) != std::string::npos );
        CHECK( gun_loc->ammo_remaining( loaded_well_idx ) == glock_count_before );
        CHECK( gun_loc->ammo_remaining( empty_well_idx ) == 0 );
    }
}

TEST_CASE( "reload_option_qty_forces_1_for_pocket_targeted_reloads",
           "[multimag][reload]" )
{
    clear_avatar();
    Character &you = get_player_character();
    you.wear_item( item( itype_backpack ) );

    item gun = make_dual_well_gun();
    item_location gun_loc = you.i_add( gun );
    REQUIRE( gun_loc );


    item glock_extra( itype_glockmag );
    glock_extra.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
    item_location mag_loc = you.i_add( glock_extra );
    REQUIRE( mag_loc );
    you.i_add( glock_extra );
    you.i_add( glock_extra );

    int glock_idx = -1;
    {
        int idx = 0;
        for( const item_pocket *p : gun_loc->get_pockets( []( const item_pocket & ) {
        return true;
    } ) ) {
            if( p->is_type( pocket_type::MAGAZINE_WELL ) ) {
                if( const item *mag = p->magazine_current() ) {
                    if( mag->typeId() == itype_glockmag ) {
                        glock_idx = idx;
                        break;
                    }
                }
            }
            ++idx;
        }
    }
    REQUIRE( glock_idx >= 0 );

    item::reload_option opt( &you, gun_loc, mag_loc, item::reload_option::POCKET_FALLBACK );
    opt.pocket_index = glock_idx;
    opt.qty( 1000 );
    CHECK( opt.qty() == 1 );
}

TEST_CASE( "get_possible_reload_targets_emits_per_well_entries",
           "[multimag][reload]" )
{
    clear_avatar();
    Character &you = get_player_character();
    you.wear_item( item( itype_backpack ) );

    SECTION( "multi-well gun: one well entry per MAGAZINE_WELL plus loaded-mag entry" ) {
        item gun = make_dual_well_gun();
        item_location gun_loc = you.i_add( gun );
        REQUIRE( gun_loc );

        const std::vector<reload_target> targets = get_possible_reload_targets( gun_loc );
        std::set<int> well_indices;
        int loaded_mag_count = 0;
        for( const reload_target &rt : targets ) {
            if( rt.kind == reload_target::kind::well && rt.owner == gun_loc ) {
                well_indices.insert( rt.pocket_index );
            } else if( rt.kind == reload_target::kind::loaded_mag && rt.owner == gun_loc ) {
                loaded_mag_count++;
            }
        }


        CHECK( well_indices.size() == 2 );
        CHECK( loaded_mag_count == 2 );
    }

    SECTION( "integral-mag gun emits a fallback loaded-mag entry on the gun itself" ) {
        item revolver( itype_sw_619 );
        item_location revolver_loc = you.i_add( revolver );
        REQUIRE( revolver_loc );

        const std::vector<reload_target> targets = get_possible_reload_targets( revolver_loc );
        bool found_fallback = false;
        for( const reload_target &rt : targets ) {
            if( rt.kind == reload_target::kind::loaded_mag && rt.target == revolver_loc &&
                rt.pocket_index < 0 ) {
                found_fallback = true;
            }
        }
        CHECK( found_fallback );
    }
}

TEST_CASE( "find_matching_reload_target_routes_ammo_to_correct_well",
           "[multimag][reload]" )
{
    clear_avatar();
    Character &you = get_player_character();
    you.wear_item( item( itype_backpack ) );

    item gun( itype_test_multimag_gun );
    item_location gun_loc = you.i_add( gun );
    REQUIRE( gun_loc );

    const std::vector<reload_target> targets = get_possible_reload_targets( gun_loc );


    int glock_well_idx = -1;
    int stanag_well_idx = -1;
    {
        int idx = 0;
        for( const item_pocket *p : gun_loc->get_pockets( []( const item_pocket & ) {
        return true;
    } ) ) {
            if( p->is_type( pocket_type::MAGAZINE_WELL ) ) {
                if( glock_well_idx < 0 ) {
                    glock_well_idx = idx;
                } else if( stanag_well_idx < 0 ) {
                    stanag_well_idx = idx;
                }
            }
            ++idx;
        }
    }
    REQUIRE( glock_well_idx >= 0 );
    REQUIRE( stanag_well_idx >= 0 );

    SECTION( "glock magazine routes to the glockmag-restricted well" ) {
        item glock_mag( itype_glockmag );
        glock_mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
        item_location glock_loc = you.i_add( glock_mag );
        REQUIRE( glock_loc );

        const reload_target *match = find_matching_reload_target( targets, glock_loc );
        REQUIRE( match != nullptr );
        CHECK( match->kind == reload_target::kind::well );
        CHECK( match->pocket_index == glock_well_idx );
    }

    SECTION( "stanag magazine routes to the stanag30-restricted well" ) {
        item stanag_mag( itype_stanag30 );
        stanag_mag.put_in( item( itype_556, calendar::turn, 30 ), pocket_type::MAGAZINE );
        item_location stanag_loc = you.i_add( stanag_mag );
        REQUIRE( stanag_loc );

        const reload_target *match = find_matching_reload_target( targets, stanag_loc );
        REQUIRE( match != nullptr );
        CHECK( match->kind == reload_target::kind::well );
        CHECK( match->pocket_index == stanag_well_idx );
    }

    SECTION( "same-type dual-well: find_all_matching emits one entry per accepting well" ) {
        item same_type_gun( itype_test_multimag_gun_same_type );
        item_location same_loc = you.i_add( same_type_gun );
        REQUIRE( same_loc );
        const std::vector<reload_target> same_targets = get_possible_reload_targets( same_loc );

        item glock_mag( itype_glockmag );
        glock_mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
        item_location glock_loc = you.i_add( glock_mag );
        REQUIRE( glock_loc );

        const std::vector<const reload_target *> matches =
            find_all_matching_reload_targets( same_targets, glock_loc );
        REQUIRE( matches.size() == 2 );

        std::set<int> seen_pocket_indices;
        for( const reload_target *rt : matches ) {
            CHECK( rt->kind == reload_target::kind::well );
            seen_pocket_indices.insert( rt->pocket_index );
        }
        CHECK( seen_pocket_indices.size() == 2 );
    }

    SECTION( "same-type loaded mags: loose ammo matches every loaded mag for class-(b) top-up" ) {




        item gun_two( itype_test_multimag_gun_same_type );
        item glock_a( itype_glockmag );
        glock_a.put_in( item( itype_9mm, calendar::turn, 5 ), pocket_type::MAGAZINE );
        item glock_b( itype_glockmag );
        glock_b.put_in( item( itype_9mm, calendar::turn, 1 ), pocket_type::MAGAZINE );
        std::vector<item_pocket *> wells;
        for( item_pocket *p : gun_two.get_pockets( []( const item_pocket & pp ) {
        return pp.is_type( pocket_type::MAGAZINE_WELL );
        } ) ) {
            wells.push_back( p );
        }
        REQUIRE( wells.size() == 2 );
        REQUIRE( wells[0]->insert_item( glock_a ).success() );
        REQUIRE( wells[1]->insert_item( glock_b ).success() );
        REQUIRE( gun_two.magazines_current().size() == 2 );

        item_location two_loc = you.i_add( gun_two );
        REQUIRE( two_loc );
        REQUIRE( two_loc->magazines_current().size() == 2 );

        const std::vector<reload_target> two_targets = get_possible_reload_targets( two_loc );

        item loose_9mm( itype_9mm, calendar::turn, 30 );
        item_location loose_loc = you.i_add( loose_9mm );
        REQUIRE( loose_loc );

        const std::vector<const reload_target *> matches =
            find_all_matching_reload_targets( two_targets, loose_loc );


        int loaded_mag_matches = 0;
        std::vector<int> loaded_remaining;
        for( const reload_target *rt : matches ) {
            if( rt->kind == reload_target::kind::loaded_mag ) {
                loaded_mag_matches++;
                loaded_remaining.push_back( rt->target->ammo_remaining() );
            }
        }
        CHECK( loaded_mag_matches == 2 );



        const reload_target *small_mag_target = nullptr;
        for( const reload_target *rt : matches ) {
            if( rt->kind == reload_target::kind::loaded_mag &&
                rt->target->ammo_remaining() == 1 ) {
                small_mag_target = rt;
                break;
            }
        }
        REQUIRE( small_mag_target != nullptr );
        item::reload_option opt( &you, small_mag_target->target, loose_loc,
                                 small_mag_target->pocket_index );

        CHECK( opt.qty() == 14 );



        item::reload_option opt_partial( &you, small_mag_target->target, loose_loc,
                                         small_mag_target->pocket_index );
        opt_partial.qty( 3 );
        CHECK( opt_partial.qty() == 3 );
    }

    SECTION( "magazine compatible with no well returns nullptr" ) {



        item loose( itype_38_special, calendar::turn, 7 );
        item_location loose_loc = you.i_add( loose );
        REQUIRE( loose_loc );

        const reload_target *match = find_matching_reload_target( targets, loose_loc );
        CHECK( match == nullptr );
    }
}

TEST_CASE( "character_unload_ejects_every_loaded_magazine", "[multimag][unload]" )
{
    clear_avatar();
    clear_map();
    Character &you = get_player_character();
    you.wear_item( item( itype_backpack ) );

    item gun = make_dual_well_gun();
    REQUIRE( gun.magazines_current().size() == 2 );

    item_location gun_loc = you.i_add( gun );
    REQUIRE( gun_loc );

    const bool unloaded = you.unload( gun_loc, true );
    CHECK( unloaded );

    const std::vector<item *> remaining = gun_loc->magazines_current();
    CHECK( remaining.empty() );

    bool found_glockmag = false;
    bool found_stanag = false;
    you.visit_items( [&]( const item_location & it ) {
        if( it->typeId() == itype_glockmag ) {
            found_glockmag = true;
        }
        if( it->typeId() == itype_stanag30 ) {
            found_stanag = true;
        }
        return VisitResponse::NEXT;
    } );
    CHECK( found_glockmag );
    CHECK( found_stanag );
}

TEST_CASE( "same_type_dual_well_targeted_reload_replaces_specific_well",
           "[multimag][reload]" )
{
    clear_avatar();
    Character &you = get_player_character();
    you.wear_item( item( itype_backpack ) );

    item gun( itype_test_multimag_gun_same_type );
    item glock_a( itype_glockmag );
    glock_a.ammo_set( itype_9mm, 15 );
    item glock_b( itype_glockmag );
    glock_b.ammo_set( itype_9mm, 5 );

    std::vector<int> well_indices;
    {
        int idx = 0;
        for( const item_pocket *p : gun.get_pockets( []( const item_pocket & ) {
        return true;
    } ) ) {
            if( p->is_type( pocket_type::MAGAZINE_WELL ) ) {
                well_indices.push_back( idx );
            }
            ++idx;
        }
    }
    REQUIRE( well_indices.size() == 2 );

    std::vector<item_pocket *> wells = gun.get_pockets( []( const item_pocket & p ) {
        return p.is_type( pocket_type::MAGAZINE_WELL );
    } );
    REQUIRE( wells.size() == 2 );
    REQUIRE( wells[0]->insert_item( glock_a ).success() );
    REQUIRE( wells[1]->insert_item( glock_b ).success() );

    item_location gun_loc = you.i_add( gun );
    REQUIRE( gun_loc );

    item replacement( itype_glockmag );
    replacement.ammo_set( itype_9mm, 15 );
    item_location replacement_loc = you.i_add( replacement );
    REQUIRE( replacement_loc );

    const int target_well = well_indices[1];
    const bool ok = gun_loc->reload( you, replacement_loc, 1, target_well );
    REQUIRE( ok );

    std::vector<const item_pocket *> wells_after = std::as_const( *gun_loc ).get_pockets(
    []( const item_pocket & p ) {
        return p.is_type( pocket_type::MAGAZINE_WELL );
    } );
    REQUIRE( wells_after.size() == 2 );
    REQUIRE( wells_after[0]->magazine_current() != nullptr );
    REQUIRE( wells_after[1]->magazine_current() != nullptr );
    CHECK( wells_after[0]->magazine_current()->ammo_remaining() == 15 );
    CHECK( wells_after[1]->magazine_current()->ammo_remaining() == 15 );
}

TEST_CASE( "reload_activity_actor_serializes_pocket_index",
           "[multimag][reload][serialize]" )
{
    clear_avatar();
    Character &you = get_player_character();
    you.wear_item( item( itype_backpack ) );

    item gun = make_dual_well_gun();
    item_location gun_loc = you.i_add( gun );
    REQUIRE( gun_loc );
    item ammo( itype_glockmag );
    ammo.ammo_set( itype_9mm, 15 );
    item_location ammo_loc = you.i_add( ammo );
    REQUIRE( ammo_loc );

    item::reload_option opt( &you, gun_loc, ammo_loc, 0 );
    reload_activity_actor actor( std::move( opt ) );

    std::ostringstream oss;
    JsonOut jsout( oss );
    actor.serialize( jsout );
    const std::string serialized = oss.str();
    CAPTURE( serialized );
    CHECK( serialized.find( "\"pocket_index\":0" ) != std::string::npos );
}



TEST_CASE( "multimag_reload_chains_to_sibling_well",
           "[multimag][reload][activity]" )
{
    clear_avatar();
    Character &you = get_player_character();
    you.wear_item( item( itype_backpack ) );

    item_location gun_loc = you.i_add( item( itype_test_multimag_turret_gun ) );
    REQUIRE( gun_loc );
    REQUIRE( gun_loc->uses_firing_requirements() );
    REQUIRE( gun_loc->magazines_current().empty() );

    item glock_mag( itype_glockmag );
    glock_mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
    item_location glock_loc = you.i_add( glock_mag );
    REQUIRE( glock_loc );
    item batt( itype_heavy_battery_cell );
    batt.ammo_set( itype_battery, 100 );
    you.i_add( batt );

    int glock_idx = -1;
    int idx = 0;
    for( const item_pocket *p : gun_loc->get_pockets( []( const item_pocket & ) {
    return true;
} ) ) {
        if( p->is_type( pocket_type::MAGAZINE_WELL ) ) {
            for( const itype_id &id : p->item_type_restrictions() ) {
                if( id == itype_glockmag ) {
                    glock_idx = idx;
                    break;
                }
            }
            if( glock_idx >= 0 ) {
                break;
            }
        }
        ++idx;
    }
    REQUIRE( glock_idx >= 0 );

    item::reload_option opt( &you, gun_loc, glock_loc, glock_idx );
    you.assign_activity( reload_activity_actor( std::move( opt ) ) );
    REQUIRE( you.activity );

    process_activity( you );

    const std::vector<item *> mags = gun_loc->magazines_current();
    CHECK( mags.size() == 2 );
}



TEST_CASE( "multimag_unload_drains_integral_and_well",
           "[multimag][unload]" )
{
    clear_avatar();
    Character &you = get_player_character();
    you.wear_item( item( itype_backpack ) );

    item gun( itype_test_multimag_gun_integral_ammo );
    REQUIRE( gun.uses_firing_requirements() );

    item battery( itype_heavy_battery_cell );
    battery.ammo_set( itype_battery, 100 );
    REQUIRE( gun.put_in( battery, pocket_type::MAGAZINE_WELL ).success() );
    REQUIRE( gun.put_in( item( itype_9mm, calendar::turn, 1 ),
                         pocket_type::MAGAZINE ).success() );

    item_location gun_loc = you.i_add( gun );
    REQUIRE( gun_loc );
    REQUIRE_FALSE( gun_loc->magazines_current().empty() );

    const bool unloaded = you.unload( gun_loc, true );
    REQUIRE( unloaded );

    CHECK( gun_loc->magazines_current().empty() );
    const item &const_gun = *gun_loc;
    const std::vector<const item_pocket *> integral_pockets = const_gun.get_pockets(
    []( const item_pocket & p ) {
        return p.is_type( pocket_type::MAGAZINE );
    } );
    for( const item_pocket *p : integral_pockets ) {
        for( const item *e : p->all_items_top() ) {
            CHECK( e->has_flag( flag_CASING ) );
        }
    }
}

TEST_CASE( "wield_collision_sees_mags_from_every_well",
           "[multimag][wield]" )
{
    item gun = make_dual_well_gun();
    const std::vector<item *> mags = gun.magazines_current();
    REQUIRE( mags.size() == 2 );
    std::set<itype_id> mag_types;
    for( const item *m : mags ) {
        mag_types.insert( m->typeId() );
    }
    CHECK( mag_types.count( itype_glockmag ) == 1 );
    CHECK( mag_types.count( itype_stanag30 ) == 1 );
}

TEST_CASE( "item_group_spawn_fills_every_well",
           "[multimag][spawn]" )
{
    const item_group::ItemList items = item_group::items_from(
                                           Item_spawn_data_test_multimag_full_load );
    REQUIRE( items.size() == 1 );
    const item &gun = items[0];
    REQUIRE( gun.typeId() == itype_test_multimag_gun );

    const std::vector<const item *> mags = gun.magazines_current();
    CHECK( mags.size() == 2 );

    std::set<itype_id> mag_types;
    for( const item *m : mags ) {
        mag_types.insert( m->typeId() );
    }
    CHECK( mag_types.count( itype_glockmag ) == 1 );
    CHECK( mag_types.count( itype_stanag30 ) == 1 );

    for( const item *m : mags ) {
        CHECK( m->ammo_remaining() > 0 );
    }
}

TEST_CASE( "item_wide_charges_on_multi_well_rejected",
           "[multimag][spawn]" )
{
    Item_modifier modifier;
    modifier.charges = { 20, 20 };
    item gun( itype_test_multimag_gun );

    const std::string dmsg = capture_debugmsg_during( [&modifier, &gun]() {
        modifier.modify( gun, "test_multimag_charges_ambiguous" );
    } );

    CHECK( dmsg.find( "ambiguous" ) != std::string::npos );
    CHECK( gun.typeId() == itype_test_multimag_gun );
    CHECK( gun.magazines_current().empty() );
    CHECK( gun.ammo_remaining() == 0 );
}

TEST_CASE( "item_magazines_default_one_entry_per_well",
           "[multimag][spawn]" )
{
    item gun( itype_test_multimag_gun );
    const std::vector<itype_id> defaults = gun.magazines_default();
    REQUIRE( defaults.size() == 2 );

    std::set<itype_id> uniq( defaults.begin(), defaults.end() );
    CHECK( uniq.count( itype_glockmag ) == 1 );
    CHECK( uniq.count( itype_stanag30 ) == 1 );
}

TEST_CASE( "dress_magazine_wells_fills_empty_siblings",
           "[multimag][spawn]" )
{
    item gun( itype_test_multimag_gun );

    item glock_mag( itype_glockmag );
    glock_mag.ammo_set( itype_9mm, 15 );
    REQUIRE( gun.put_in( glock_mag, pocket_type::MAGAZINE_WELL ).success() );
    REQUIRE( gun.magazines_current().size() == 1 );

    gun.dress_magazine_wells( true, true );

    const std::vector<item *> mags = gun.magazines_current();
    REQUIRE( mags.size() == 2 );
    std::set<itype_id> mag_types;
    for( const item *m : mags ) {
        mag_types.insert( m->typeId() );
    }
    CHECK( mag_types.count( itype_glockmag ) == 1 );
    CHECK( mag_types.count( itype_stanag30 ) == 1 );
    for( const item *m : mags ) {
        CHECK( m->ammo_remaining() > 0 );
    }
}

TEST_CASE( "dress_magazine_wells_tops_up_empty_mag",
           "[multimag][spawn]" )
{
    item gun( itype_test_multimag_gun );
    item empty_mag( itype_glockmag );
    REQUIRE( empty_mag.ammo_remaining() == 0 );
    REQUIRE( gun.put_in( empty_mag, pocket_type::MAGAZINE_WELL ).success() );

    gun.dress_magazine_wells( false, true );

    const std::vector<item *> mags = gun.magazines_current();
    REQUIRE( mags.size() == 1 );
    CHECK( mags[0]->typeId() == itype_glockmag );
    CHECK( mags[0]->ammo_remaining() > 0 );
}




static item make_multimag_consume_gun( int bb_count = 15, int batt = 100 )
{
    item gun( itype_test_multimag_gun_consume );
    item bb_mag( itype_glockmag );
    bb_mag.put_in( item( itype_9mm, calendar::turn, bb_count ), pocket_type::MAGAZINE );
    item batt_mag( itype_heavy_battery_cell );
    batt_mag.put_in( item( itype_battery, calendar::turn, batt ), pocket_type::MAGAZINE );
    REQUIRE( gun.put_in( bb_mag, pocket_type::MAGAZINE_WELL ).success() );
    REQUIRE( gun.put_in( batt_mag, pocket_type::MAGAZINE_WELL ).success() );
    return gun;
}

static item make_multimag_consume_tool( int well_a_charges = 12, int well_b_charges = 30 )
{
    item tool( itype_test_multimag_tool_consume );
    item mag_a( itype_glockmag );
    mag_a.put_in( item( itype_9mm, calendar::turn, well_a_charges ), pocket_type::MAGAZINE );
    item mag_b( itype_stanag30 );
    mag_b.put_in( item( itype_556, calendar::turn, well_b_charges ), pocket_type::MAGAZINE );
    REQUIRE( tool.put_in( mag_a, pocket_type::MAGAZINE_WELL ).success() );
    REQUIRE( tool.put_in( mag_b, pocket_type::MAGAZINE_WELL ).success() );
    return tool;
}




static item make_multimag_welder( int batt_charges, int oxygen_qty, int rod_qty )
{
    item tool( itype_test_multimag_vehicle_welder );
    item batt_mag( itype_heavy_battery_cell );
    batt_mag.put_in( item( itype_battery, calendar::turn, batt_charges ), pocket_type::MAGAZINE );
    REQUIRE( tool.put_in( batt_mag, pocket_type::MAGAZINE_WELL ).success() );
    if( oxygen_qty > 0 ) {
        REQUIRE( tool.put_in( item( itype_oxygen, calendar::turn, oxygen_qty ),
                              pocket_type::MAGAZINE ).success() );
    }
    if( rod_qty > 0 ) {
        REQUIRE( tool.put_in( item( itype_welding_wire_steel_tests_dupe, calendar::turn, rod_qty ),
                              pocket_type::MAGAZINE ).success() );
    }
    return tool;
}

TEST_CASE( "multimag_predicates",
           "[multimag][consume]" )
{
    SECTION( "uses_firing_requirements" ) {
        item legacy( itype_glock_19 );
        CHECK_FALSE( legacy.uses_firing_requirements() );

        item gun = make_multimag_consume_gun();
        CHECK( gun.uses_firing_requirements() );
    }

    SECTION( "needs_charges_to_use covers multimag" ) {
        item gun = make_multimag_consume_gun();
        CHECK( gun.needs_charges_to_use() );
    }

    SECTION( "pocket_by_id" ) {
        item gun = make_multimag_consume_gun();
        CHECK( gun.pocket_by_id( "ammo" ) != nullptr );
        CHECK( gun.pocket_by_id( "power" ) != nullptr );
        CHECK( gun.pocket_by_id( "missing" ) == nullptr );
    }

    SECTION( "ammo_remaining_in_pocket" ) {
        item gun = make_multimag_consume_gun( 12, 80 );
        CHECK( gun.ammo_remaining_in_pocket( "ammo" ) == 12 );
        CHECK( gun.ammo_remaining_in_pocket( "power" ) == 80 );
        CHECK( gun.ammo_remaining_in_pocket( "missing" ) == 0 );
    }
}

TEST_CASE( "primary_ammo_pocket_selection_order",
           "[multimag][consume]" )
{
    SECTION( "loaded primary-ammo well preferred over earlier non-matching well" ) {


        item gun = make_multimag_consume_gun();
        const item_pocket *primary = gun.primary_ammo_pocket();
        REQUIRE( primary != nullptr );
        const item *mag = primary->magazine_current();
        REQUIRE( mag != nullptr );
        CHECK( mag->typeId() == itype_glockmag );
    }

    SECTION( "ammo_data routes through primary_ammo_pocket" ) {
        item gun = make_multimag_consume_gun();
        const itype *adata = gun.ammo_data();
        REQUIRE( adata != nullptr );
        CHECK( adata->get_id() == itype_9mm );
    }
}

TEST_CASE( "tool_uses_remaining_feasibility",
           "[multimag][consume]" )
{
    SECTION( "non-fungible local stock; pocket B empty caps capacity" ) {

        item tool = make_multimag_consume_tool( 12, 0 );
        CHECK( tool.tool_uses_remaining_local() == 0 );
    }

    SECTION( "abundant local in both wells supports min-uses computation" ) {


        item tool = make_multimag_consume_tool( 12, 30 );
        CHECK( tool.tool_uses_remaining_local() == 6 );
    }
}

TEST_CASE( "consume_shots_and_consume_tool_uses_drain_per_pocket",
           "[multimag][consume]" )
{
    clear_map();
    map &here = get_map();
    tripoint_bub_ms pos( tripoint_bub_ms::zero );

    SECTION( "gun DEFAULT mode drains 1 ammo + 5 power per shot" ) {
        item gun = make_multimag_consume_gun( 15, 100 );
        const int got = gun.consume_shots( gun_mode_DEFAULT, 3, here, pos, nullptr );
        CHECK( got == 3 );
        CHECK( gun.ammo_remaining_in_pocket( "ammo" ) == 12 );
        CHECK( gun.ammo_remaining_in_pocket( "power" ) == 85 );
    }

    SECTION( "gun BURST mode drains 3 ammo + 15 power per shot" ) {
        item gun = make_multimag_consume_gun( 15, 100 );
        const int got = gun.consume_shots( gun_mode_BURST, 2, here, pos, nullptr );
        CHECK( got == 2 );
        CHECK( gun.ammo_remaining_in_pocket( "ammo" ) == 9 );
        CHECK( gun.ammo_remaining_in_pocket( "power" ) == 70 );
    }

    SECTION( "tool consume drains both wells linearly" ) {
        item tool = make_multimag_consume_tool( 12, 30 );
        const int got = tool.consume_tool_uses( 3, here, pos, nullptr );
        CHECK( got == 3 );
        CHECK( tool.ammo_remaining_in_pocket( "well_a" ) == 6 );
        CHECK( tool.ammo_remaining_in_pocket( "well_b" ) == 27 );
    }

    SECTION( "shortfall in one well caps consumption" ) {

        item tool = make_multimag_consume_tool( 12, 1 );
        const int got = tool.consume_tool_uses( 5, here, pos, nullptr );
        CHECK( got == 1 );
        CHECK( tool.ammo_remaining_in_pocket( "well_a" ) == 10 );
        CHECK( tool.ammo_remaining_in_pocket( "well_b" ) == 0 );
    }

    SECTION( "direct MAGAZINE oxygen pocket drains alongside MAGAZINE_WELL battery" ) {

        item welder = make_multimag_welder( 100, 20, 5 );
        REQUIRE( welder.ammo_remaining_in_pocket( "power" ) == 100 );
        REQUIRE( welder.ammo_remaining_in_pocket( "oxygen" ) == 20 );
        REQUIRE( welder.ammo_remaining_in_pocket( "rod" ) == 5 );
        const int got = welder.consume_tool_uses( 1, here, pos, nullptr );
        CHECK( got == 1 );
        CHECK( welder.ammo_remaining_in_pocket( "power" ) == 95 );
        CHECK( welder.ammo_remaining_in_pocket( "oxygen" ) == 18 );
        CHECK( welder.ammo_remaining_in_pocket( "rod" ) == 4 );
    }
}

TEST_CASE( "gunmod_supplies_per_mode_firing_requirements", "[multimag][consume][mod]" )
{
    clear_map();
    map &here = get_map();
    const tripoint_bub_ms pos = tripoint_bub_ms::zero;

    item gun = make_multimag_consume_gun( 15, 100 );
    REQUIRE( gun.put_in( item( itype_test_multimag_regulated_mod ),
                         pocket_type::MOD ).success() );

    GIVEN( "a gun mod that adds a REGULATED mode with its own per-pocket cost" ) {
        THEN( "the added mode appears and DEFAULT is hidden" ) {
            const std::map<gun_mode_id, gun_mode> modes = gun.gun_all_modes();
            CHECK( modes.count( gun_mode_REGULATED ) == 1 );
            CHECK( modes.count( gun_mode_DEFAULT ) == 0 );
        }
        WHEN( "fired in REGULATED mode" ) {
            const int got = gun.consume_shots( gun_mode_REGULATED, 3, here, pos, nullptr );
            THEN( "the mod's firing_requirements drive consumption, not DEFAULT" ) {
                CHECK( got == 3 );

                CHECK( gun.ammo_remaining_in_pocket( "ammo" ) == 12 );

                CHECK( gun.ammo_remaining_in_pocket( "power" ) == 97 );
            }
        }
        WHEN( "the hidden DEFAULT mode id is fired directly" ) {
            const int got = gun.consume_shots( gun_mode_DEFAULT, 3, here, pos, nullptr );
            THEN( "it is blocked and bills nothing, not the host DEFAULT cost" ) {
                CHECK( got == 0 );
                CHECK( gun.ammo_remaining_in_pocket( "ammo" ) == 15 );
                CHECK( gun.ammo_remaining_in_pocket( "power" ) == 100 );
            }
        }
    }
}

TEST_CASE( "gunmod_hide_modes_global_kill_switch", "[multimag][consume][mod]" )
{
    clear_map();
    map &here = get_map();
    const tripoint_bub_ms pos = tripoint_bub_ms::zero;

    SECTION( "hiding every mode leaves the gun unfireable, not crashing" ) {
        item gun = make_multimag_consume_gun( 15, 100 );
        REQUIRE( gun.put_in( item( itype_test_multimag_hide_all_mod ),
                             pocket_type::MOD ).success() );
        CHECK( gun.gun_all_modes().empty() );

        CHECK( gun.shots_remaining( here, nullptr ) == 0 );
        CHECK( gun.consume_shots( gun_mode_DEFAULT, 1, here, pos, nullptr ) == 0 );
        CHECK( gun.ammo_remaining_in_pocket( "ammo" ) == 15 );
        CHECK( gun.ammo_remaining_in_pocket( "power" ) == 100 );
    }

    SECTION( "one mod can hide a mode another mod added" ) {
        item gun = make_multimag_consume_gun( 15, 100 );
        REQUIRE( gun.put_in( item( itype_test_multimag_add_extra_mode_mod ),
                             pocket_type::MOD ).success() );
        REQUIRE( gun.gun_all_modes().count( gun_mode_EXTRA ) == 1 );
        REQUIRE( gun.put_in( item( itype_test_multimag_hide_extra_mode_mod ),
                             pocket_type::MOD ).success() );
        CHECK( gun.gun_all_modes().count( gun_mode_EXTRA ) == 0 );
    }
}

TEST_CASE( "gunmod_scales_integral_pocket_capacity", "[multimag][mod]" )
{
    clear_map();
    item gun( itype_test_multimag_gun_integral_ammo );
    const item_pocket *ammo_p = gun.pocket_by_id( "ammo" );
    REQUIRE( ammo_p != nullptr );
    REQUIRE_FALSE( ammo_p->ammo_types().empty() );
    const ammotype at = *ammo_p->ammo_types().begin();
    REQUIRE( ammo_p->ammo_capacity( at ) == 1 );

    REQUIRE( gun.put_in( item( itype_test_multimag_capacity_gunmod ),
                         pocket_type::MOD ).success() );

    THEN( "the integral pocket capacity scales, including the reload-facing remaining" ) {
        const item_pocket *scaled = gun.pocket_by_id( "ammo" );
        REQUIRE( scaled != nullptr );
        CHECK( scaled->ammo_capacity( at ) == 2 );
        CHECK( scaled->remaining_ammo_capacity( at ) == 2 );

        CHECK( gun.ammo_capacity( at ) == 2 );
    }
}

TEST_CASE( "capacity_mods_reflected_in_item_name", "[multimag][mod]" )
{
    clear_map();
    item gun( itype_test_multimag_gun_integral_ammo );
    REQUIRE( gun.put_in( item( itype_test_multimag_capacity_gunmod ),
                         pocket_type::MOD ).success() );
    const std::string name = remove_color_tags( gun.display_name() );
    INFO( name );

    CHECK( name.find( "0/2" ) != std::string::npos );
}

TEST_CASE( "capacity_mods_survive_save_load_roundtrip", "[multimag][mod]" )
{
    clear_map();
    item gun( itype_test_multimag_gun_integral_ammo );
    REQUIRE( gun.put_in( item( itype_test_multimag_capacity_gunmod ),
                         pocket_type::MOD ).success() );
    const item_pocket *ammo_p = gun.pocket_by_id( "ammo" );
    REQUIRE( ammo_p != nullptr );
    const ammotype at = *ammo_p->ammo_types().begin();
    REQUIRE( ammo_p->ammo_capacity( at ) == 2 );

    std::ostringstream os;
    JsonOut jsout( os );
    gun.serialize( jsout );
    JsonValue jv = json_loader::from_string( os.str() );
    item loaded;
    loaded.deserialize( jv.get_object() );


    const item_pocket *ammo2 = loaded.pocket_by_id( "ammo" );
    REQUIRE( ammo2 != nullptr );
    CHECK( ammo2->ammo_capacity( at ) == 2 );
}

TEST_CASE( "capacity_mod_overflow_grandfathered_on_removal", "[multimag][mod]" )
{
    clear_map();
    Character &guy = get_player_character();

    item_location gun = guy.i_add( item( itype_test_multimag_gun_integral_ammo ) );
    REQUIRE( gun->put_in( item( itype_test_multimag_capacity_gunmod ),
                          pocket_type::MOD ).success() );
    item_pocket *ammo = gun->pocket_by_id( "ammo" );
    REQUIRE( ammo != nullptr );
    const ammotype at = *ammo->ammo_types().begin();
    REQUIRE( ammo->ammo_capacity( at ) == 2 );
    REQUIRE( ammo->insert_item( item( itype_9mm, calendar::turn, 1 ) ).success() );
    REQUIRE( ammo->insert_item( item( itype_9mm, calendar::turn, 1 ) ).success() );
    REQUIRE( ammo->remaining_ammo_capacity( at ) == 0 );

    WHEN( "the boost mod is removed while the pocket is at the boosted fill" ) {
        gun.remove_items_with( [&]( const item & it ) {
            return it.typeId() == itype_test_multimag_capacity_gunmod;
        } );
        THEN( "capacity drops but contents are grandfathered and remaining clamps to 0" ) {
            const item_pocket *ammo2 = gun->pocket_by_id( "ammo" );
            REQUIRE( ammo2 != nullptr );
            CHECK( ammo2->ammo_capacity( at ) == 1 );
            CHECK( ammo2->remaining_ammo_capacity( at ) == 0 );

            CHECK( gun->ammo_capacity( at ) == 1 );
            CHECK( gun->remaining_ammo_capacity() == 0 );
            int loaded = 0;
            for( const item *it : ammo2->all_items_top() ) {
                loaded += it->count();
            }
            CHECK( loaded == 2 );
        }
    }
}

TEST_CASE( "capacity_mods_on_magazine_well_diagnosed", "[multimag][mod]" )
{
    clear_map();
    item gun( itype_test_multimag_gun_integral_ammo );
    const std::string dmsg = capture_debugmsg_during( [&]() {
        gun.put_in( item( itype_test_multimag_capacity_well_gunmod ), pocket_type::MOD );
    } );
    CHECK( dmsg.find( "not an integral MAGAZINE" ) != std::string::npos );
}

TEST_CASE( "mod_consumption_mods_override_per_pocket", "[multimag][consume][mod]" )
{
    clear_map();
    map &here = get_map();
    const tripoint_bub_ms pos = tripoint_bub_ms::zero;

    SECTION( "gunmod set overrides a pocket's per-shot cost" ) {
        item gun = make_multimag_consume_gun( 15, 100 );
        REQUIRE( gun.put_in( item( itype_test_multimag_consume_gunmod ),
                             pocket_type::MOD ).success() );
        const int got = gun.consume_shots( gun_mode_DEFAULT, 2, here, pos, nullptr );
        CHECK( got == 2 );
        CHECK( gun.ammo_remaining_in_pocket( "ammo" ) == 13 );
        CHECK( gun.ammo_remaining_in_pocket( "power" ) == 96 );
    }

    SECTION( "toolmod multiply scales a pocket's per-use cost" ) {
        item tool = make_multimag_consume_tool( 12, 30 );
        REQUIRE( tool.put_in( item( itype_test_multimag_consume_toolmod ),
                              pocket_type::MOD ).success() );
        const int got = tool.consume_tool_uses( 3, here, pos, nullptr );
        CHECK( got == 3 );

        CHECK( tool.ammo_remaining_in_pocket( "well_a" ) == 9 );
        CHECK( tool.ammo_remaining_in_pocket( "well_b" ) == 27 );
    }
}

TEST_CASE( "mod_unknown_pocket_id_diagnosed_at_install", "[multimag][mod]" )
{
    clear_map();
    item gun = make_multimag_consume_gun( 15, 100 );
    const std::string dmsg = capture_debugmsg_during( [&]() {
        gun.put_in( item( itype_test_multimag_bad_pocket_gunmod ), pocket_type::MOD );
    } );
    CHECK( dmsg.find( "unknown pocket id" ) != std::string::npos );
    CHECK( dmsg.find( "nonexistent" ) != std::string::npos );
}

TEST_CASE( "consumption_mods_combine_order_independent", "[multimag][consume][mod]" )
{
    clear_map();
    map &here = get_map();
    const tripoint_bub_ms pos = tripoint_bub_ms::zero;



    auto power_drained = [&]( bool set_first ) -> int {
        item gun = make_multimag_consume_gun( 15, 100 );
        if( set_first )
        {
            REQUIRE( gun.put_in( item( itype_test_multimag_consume_gunmod ), pocket_type::MOD ).success() );
            REQUIRE( gun.put_in( item( itype_test_multimag_consume_gunmod_mult ),
                                 pocket_type::MOD ).success() );
        } else
        {
            REQUIRE( gun.put_in( item( itype_test_multimag_consume_gunmod_mult ),
                                 pocket_type::MOD ).success() );
            REQUIRE( gun.put_in( item( itype_test_multimag_consume_gunmod ), pocket_type::MOD ).success() );
        }
        REQUIRE( gun.consume_shots( gun_mode_DEFAULT, 1, here, pos, nullptr ) == 1 );
        return 100 - gun.ammo_remaining_in_pocket( "power" );
    };

    const int set_first = power_drained( true );
    const int mult_first = power_drained( false );
    CAPTURE( set_first, mult_first );
    CHECK( set_first == 1 );
    CHECK( mult_first == 1 );
}

TEST_CASE( "tool_gunmod_firing_requirements_is_not_own_consumption", "[multimag][consume][mod]" )
{
    clear_map();
    map &here = get_map();
    const tripoint_bub_ms pos = tripoint_bub_ms::zero;



    item tg( itype_test_multimag_tool_gunmod );
    REQUIRE( tg.is_tool() );
    REQUIRE( tg.is_gunmod() );
    CHECK_FALSE( tg.uses_firing_requirements() );

    SECTION( "installed on a gun, its mode draws the mod's per-pocket cost" ) {
        item gun = make_multimag_consume_gun( 15, 100 );
        REQUIRE( gun.put_in( item( itype_test_multimag_tool_gunmod ),
                             pocket_type::MOD ).success() );
        REQUIRE( gun.gun_all_modes().count( gun_mode_TGMODE ) == 1 );
        const int got = gun.consume_shots( gun_mode_TGMODE, 2, here, pos, nullptr );
        CHECK( got == 2 );
        CHECK( gun.ammo_remaining_in_pocket( "ammo" ) == 13 );
        CHECK( gun.ammo_remaining_in_pocket( "power" ) == 98 );
    }
}

TEST_CASE( "aux_gunmod_mode_blocked_on_multimag_host", "[multimag][consume][mod]" )
{
    clear_map();
    map &here = get_map();
    const tripoint_bub_ms pos = tripoint_bub_ms::zero;

    item gun = make_multimag_consume_gun( 15, 100 );
    REQUIRE( gun.put_in( item( itype_test_multimag_aux_gun ), pocket_type::MOD ).success() );


    gun_mode_id aux_mode;
    for( const std::pair<const gun_mode_id, gun_mode> &m : gun.gun_all_modes() ) {
        if( m.second.target != &gun ) {
            aux_mode = m.first;
        }
    }
    REQUIRE_FALSE( aux_mode.is_empty() );

    WHEN( "the aux mode is selected on the multimag host" ) {
        THEN( "it is blocked: not fireable, host pockets untouched" ) {
            CHECK( gun.firing_mode_blocked( aux_mode ) );
            int consumed = -1;

            const std::string dmsg = capture_debugmsg_during( [&]() {
                consumed = gun.consume_shots( aux_mode, 1, here, pos, nullptr );
            } );
            CHECK( consumed == 0 );
            CHECK( dmsg.find( "aux-gunmod" ) != std::string::npos );
            CHECK( gun.ammo_remaining_in_pocket( "ammo" ) == 15 );
            CHECK( gun.ammo_remaining_in_pocket( "power" ) == 100 );
        }
    }
}

TEST_CASE( "shots_remaining_mode_aware",
           "[multimag][consume]" )
{
    item gun = make_multimag_consume_gun( 15, 100 );
    const map &here = get_map();

    CHECK( gun.shots_remaining( here, nullptr ) == std::min( 15, 100 / 5 ) );
}

TEST_CASE( "ammo_consume_qty_hard_guard_for_multimag",
           "[multimag][consume]" )
{
    clear_map();
    map &here = get_map();
    tripoint_bub_ms pos( tripoint_bub_ms::zero );

    item gun = make_multimag_consume_gun();
    int consumed = 0;
    const std::string captured = capture_debugmsg_during( [&]() {
        consumed = gun.ammo_consume( 5, here, pos, nullptr );
    } );
    CHECK( consumed == 0 );
    CHECK( captured.find( "multimag" ) != std::string::npos );

    CHECK( gun.ammo_remaining_in_pocket( "ammo" ) == 15 );
    CHECK( gun.ammo_remaining_in_pocket( "power" ) == 100 );
}

TEST_CASE( "legacy_charges_per_use_factor_non_divisible_request",
           "[multimag][consume]" )
{
    clear_map();
    map &here = get_map();
    tripoint_bub_ms pos( tripoint_bub_ms::zero );


    item tool( itype_test_multimag_tool_factor );
    item mag_a( itype_glockmag );
    mag_a.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
    REQUIRE( tool.put_in( mag_a, pocket_type::MAGAZINE_WELL ).success() );

    SECTION( "divisible qty: 10 charges -> 2 uses -> 2 from well" ) {
        const int got = tool.consume_tool_uses( 2, here, pos, nullptr );
        CHECK( got == 2 );
        CHECK( tool.ammo_remaining_in_pocket( "well_a" ) == 13 );
    }
}

TEST_CASE( "format_consumption_requirements_scaling",
           "[multimag][consume]" )
{
    SECTION( "multimag tool produces per-pocket totals scaled by uses" ) {
        item tool = make_multimag_consume_tool();
        const std::string s = tool.format_consumption_requirements(
                                  "", gun_mode_DEFAULT, 3 );

        CHECK( s.find( '6' ) != std::string::npos );
        CHECK( s.find( '3' ) != std::string::npos );
        CHECK( s.find( '+' ) != std::string::npos );
    }
}

TEST_CASE( "expected_cost_per_use_sums_effective_qty",
           "[multimag][consume]" )
{
    item gun = make_multimag_consume_gun();

    CHECK( gun.expected_cost_per_use() == 6 );

    item legacy( itype_glock_19 );

    CHECK( legacy.expected_cost_per_use() == 1 );
}

TEST_CASE( "vehicle_turret_filter_multimag_per_gun_opt_out", "[multimag][consume]" )
{
    SECTION( "NO_TURRET multimag gun keeps the per-gun opt-out" ) {
        item gun( itype_test_multimag_gun_consume );
        REQUIRE( gun.uses_firing_requirements() );
        REQUIRE( gun.has_flag( flag_NO_TURRET ) );
    }
    SECTION( "multimag gun without NO_TURRET passes the filter" ) {
        item gun( itype_test_multimag_turret_gun );
        REQUIRE( gun.uses_firing_requirements() );
        REQUIRE_FALSE( gun.has_flag( flag_NO_TURRET ) );
    }
}

TEST_CASE( "Character_consume_charges_hard_guards_multimag_tools",
           "[multimag][consume]" )
{
    clear_avatar();
    Character &chr = get_avatar();
    item tool = make_multimag_consume_tool( 12, 30 );
    item_location loc = chr.i_add( tool );
    REQUIRE( loc );
    item *carried = loc.get_item();
    REQUIRE( carried != nullptr );
    REQUIRE( carried->uses_firing_requirements() );

    bool destroyed = false;
    const std::string captured = capture_debugmsg_during( [&]() {
        destroyed = chr.consume_charges( *carried, 1 );
    } );
    CHECK_FALSE( destroyed );
    CHECK( captured.find( "multimag" ) != std::string::npos );
    CHECK( carried->ammo_remaining_in_pocket( "well_a" ) == 12 );
    CHECK( carried->ammo_remaining_in_pocket( "well_b" ) == 30 );
}

TEST_CASE( "legacy_charges_per_use_factor_non_divisible_hard_fails",
           "[multimag][consume]" )
{
    clear_avatar();
    Character &chr = get_avatar();
    item tool( itype_test_multimag_tool_factor );
    item mag_a( itype_glockmag );
    mag_a.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
    REQUIRE( tool.put_in( mag_a, pocket_type::MAGAZINE_WELL ).success() );
    item_location loc = chr.i_add( tool );
    REQUIRE( loc );
    item *carried = loc.get_item();
    REQUIRE( carried != nullptr );

    std::list<item> used;
    int qty = 7;
    const std::string captured = capture_debugmsg_during( [&]() {
        carried->use_charges( loc, itype_test_multimag_tool_factor, qty, used,
                              tripoint_bub_ms::zero,
                              return_true<item>, false );
    } );
    CHECK( captured.find( "factor" ) != std::string::npos );
    CHECK( carried->ammo_remaining_in_pocket( "well_a" ) == 15 );
}

TEST_CASE( "uses_energy_and_is_chargeable_across_multiple_wells",
           "[multimag][consume]" )
{
    SECTION( "multimag tool with battery + non-battery wells reports energy use" ) {


        item gun = make_multimag_consume_gun();
        CHECK( gun.uses_energy() );
    }

    SECTION( "is_chargeable considers only the battery-flavored capacity" ) {


        item gun( itype_test_multimag_gun_consume );
        item bb_mag( itype_glockmag );
        bb_mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
        item batt_mag( itype_heavy_battery_cell );
        REQUIRE( gun.put_in( bb_mag, pocket_type::MAGAZINE_WELL ).success() );
        REQUIRE( gun.put_in( batt_mag, pocket_type::MAGAZINE_WELL ).success() );
        CHECK( gun.is_chargeable() );
    }
}

TEST_CASE( "tool_uses_remaining_vs_local",
           "[multimag][consume]" )
{
    clear_map();
    map &here = get_map();
    item tool = make_multimag_consume_tool( 12, 30 );

    CHECK( tool.tool_uses_remaining_local() == tool.tool_uses_remaining( here, nullptr ) );
}

TEST_CASE( "gunmod_added_modes_inherit_DEFAULT_firing_requirements",
           "[multimag][consume]" )
{
    clear_map();
    map &here = get_map();
    tripoint_bub_ms pos( tripoint_bub_ms::zero );

    SECTION( "consume_shots on an unlisted mode falls back to DEFAULT" ) {
        item gun = make_multimag_consume_gun();


        const int got = gun.consume_shots( gun_mode_AUTO, 2, here, pos, nullptr );

        CHECK( got == 2 );
        CHECK( gun.ammo_remaining_in_pocket( "ammo" ) == 13 );
        CHECK( gun.ammo_remaining_in_pocket( "power" ) == 90 );
    }

    SECTION( "format_consumption_requirements falls back to DEFAULT for unlisted mode" ) {
        item gun = make_multimag_consume_gun();
        const std::string s = gun.format_consumption_requirements(
                                  "", gun_mode_AUTO, 1 );
        CHECK_FALSE( s.empty() );

        CHECK( s.find( '1' ) != std::string::npos );
        CHECK( s.find( '5' ) != std::string::npos );
    }

    SECTION( "BURST mode uses its own firing_requirements, not DEFAULT" ) {
        item gun = make_multimag_consume_gun();
        const int got = gun.consume_shots( gun_mode_BURST, 1, here, pos, nullptr );

        CHECK( got == 1 );
        CHECK( gun.ammo_remaining_in_pocket( "ammo" ) == 12 );
        CHECK( gun.ammo_remaining_in_pocket( "power" ) == 85 );
    }
}

TEST_CASE( "format_consumption_requirements_legacy_item",
           "[multimag][consume]" )
{
    item gun = make_loaded_glock();
    REQUIRE_FALSE( gun.uses_firing_requirements() );
    const std::string s_one = gun.format_consumption_requirements(
                                  "", gun_mode_DEFAULT, 1 );

    CHECK( s_one.find( '1' ) != std::string::npos );

    const std::string s_three = gun.format_consumption_requirements(
                                    "", gun_mode_DEFAULT, 3 );
    CHECK( s_three.find( '3' ) != std::string::npos );
}

TEST_CASE( "Character_use_charges_drains_multimag_tool_via_consume_tool_uses",
           "[multimag][consume]" )
{
    clear_avatar();
    Character &chr = get_avatar();
    item tool = make_multimag_consume_tool( 12, 30 );
    item_location loc = chr.i_add( tool );
    REQUIRE( loc );
    REQUIRE( loc->uses_firing_requirements() );

    int qty = 1;
    std::list<item> used;
    loc->use_charges( loc, itype_test_multimag_tool_consume, qty, used,
                      tripoint_bub_ms::zero,
                      return_true<item>, false );
    CHECK( qty == 0 );
    CHECK( loc->ammo_remaining_in_pocket( "well_a" ) == 10 );
    CHECK( loc->ammo_remaining_in_pocket( "well_b" ) == 29 );
}

TEST_CASE( "charges_of_for_multimag_tool_reports_local_uses",
           "[multimag][consume]" )
{
    clear_avatar();
    Character &chr = get_avatar();

    SECTION( "no external pool: local-only count" ) {

        item tool = make_multimag_consume_tool( 12, 30 );
        item_location loc = chr.i_add( tool );
        REQUIRE( loc );

        const int reported = chr.charges_of( itype_test_multimag_tool_consume );
        CHECK( reported == 6 );
    }

    SECTION( "routes external UPS pool when item has USE_UPS + battery pocket" ) {

        chr.worn.wear_item( chr, item( itype_backpack ), false, false );
        item_location welder = chr.i_add( make_multimag_welder( 0, 10, 5 ) );
        REQUIRE( welder );

        item ups( itype_UPS_ON );
        item ups_mag( ups.magazine_default() );
        ups_mag.ammo_set( ups_mag.ammo_default(), 100 );
        REQUIRE( ups.put_in( ups_mag, pocket_type::MAGAZINE_WELL ).success() );
        chr.i_add( ups );
        REQUIRE( units::to_kilojoule( chr.available_ups() ) == 100 );

        const int reported = chr.charges_of( itype_test_multimag_vehicle_welder );
        CHECK( reported == 5 );
    }

    SECTION( "routes external UPS pool from ground via crafting_inventory" ) {



        clear_map();
        chr.worn.wear_item( chr, item( itype_backpack ), false, false );
        item_location welder = chr.i_add( make_multimag_welder( 0, 10, 5 ) );
        REQUIRE( welder );
        REQUIRE( units::to_kilojoule( chr.available_ups() ) == 0 );

        item ups( itype_UPS_ON );
        item ups_mag( ups.magazine_default() );
        ups_mag.ammo_set( ups_mag.ammo_default(), 100 );
        REQUIRE( ups.put_in( ups_mag, pocket_type::MAGAZINE_WELL ).success() );
        get_map().add_item_or_charges( chr.pos_bub(), ups );

        const int reported = chr.crafting_inventory().charges_of(
                                 itype_test_multimag_vehicle_welder );
        CHECK( reported == 5 );
    }

    SECTION( "inventory dedup: two multimag USE_UPS welders share one UPS" ) {

        chr.worn.wear_item( chr, item( itype_backpack ), false, false );
        REQUIRE( chr.i_add( make_multimag_welder( 5, 6, 3 ) ) );
        REQUIRE( chr.i_add( make_multimag_welder( 5, 6, 3 ) ) );

        item ups( itype_UPS_ON );
        item ups_mag( ups.magazine_default() );
        ups_mag.ammo_set( ups_mag.ammo_default(), 10 );
        REQUIRE( ups.put_in( ups_mag, pocket_type::MAGAZINE_WELL ).success() );
        chr.i_add( ups );
        REQUIRE( units::to_kilojoule( chr.available_ups() ) == 10 );

        const int reported = chr.charges_of( itype_test_multimag_vehicle_welder );
        CHECK( reported == 4 );
    }
}

TEST_CASE( "item_action_comparator_ranks_legacy_below_multimag_by_cost",
           "[multimag][consume]" )
{
    item legacy( itype_glock_19 );
    item multimag = make_multimag_consume_gun();


    CHECK( legacy.expected_cost_per_use() < multimag.expected_cost_per_use() );



    CHECK( multimag.needs_charges_to_use() );
}

TEST_CASE( "raw_ammo_tool_drains_paper_count_via_ammo_consume",
           "[multimag][consume]" )
{
    clear_map();
    tripoint_bub_ms pos( tripoint_bub_ms::zero );




    item binder( itype_book_binder );
    item paper_pile( itype_paper, calendar::turn, 50 );
    REQUIRE( binder.put_in( paper_pile, pocket_type::MAGAZINE ).success() );
    REQUIRE( binder.ammo_remaining() == 50 );

    const int consumed = binder.ammo_consume( 7, pos, nullptr );
    CHECK( consumed == 7 );
    CHECK( binder.ammo_remaining() == 43 );
}

TEST_CASE( "ammo_consume_on_item_without_charges_per_use_drains_raw_count",
           "[multimag][consume]" )
{
    clear_map();
    tripoint_bub_ms pos( tripoint_bub_ms::zero );




    item bb_mag( itype_glockmag );
    bb_mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
    item legacy( itype_glock_19 );
    REQUIRE( legacy.put_in( bb_mag, pocket_type::MAGAZINE_WELL ).success() );

    const int consumed = legacy.ammo_consume( 4, pos, nullptr );
    CHECK( consumed == 4 );
    CHECK( legacy.ammo_remaining() == 11 );
}






TEST_CASE( "multimag_fire_skips_incompatible_ammo_gate", "[multimag][fire]" )
{
    clear_map();
    avatar &dummy = get_avatar();
    dummy.set_body();
    dummy.clear_worn();
    dummy.remove_weapon();

    item gun( itype_test_multimag_gun_integral_ammo );
    REQUIRE( gun.uses_firing_requirements() );
    REQUIRE( gun.put_in( item( itype_9mm, calendar::turn, 1 ),
                         pocket_type::MAGAZINE ).success() );
    item battery( itype_heavy_battery_cell );
    battery.ammo_set( itype_battery, 100 );
    REQUIRE( gun.put_in( battery, pocket_type::MAGAZINE_WELL ).success() );

    dummy.set_wielded_item( gun );
    REQUIRE( dummy.get_wielded_item() );

    Messages::clear_messages();
    avatar_action::fire_wielded_weapon( dummy );

    for( const auto &msg : Messages::recent_messages( 0 ) ) {
        INFO( "message: " << msg.second );
        CHECK( msg.second.find( "incompatible ammunition" ) == std::string::npos );
    }
}



TEST_CASE( "display_name_integral_magazine_per_pocket_segments", "[multimag][display]" )
{
    item gun( itype_test_multimag_gun_integral_ammo );
    REQUIRE( gun.uses_firing_requirements() );

    SECTION( "slug only" ) {
        REQUIRE( gun.put_in( item( itype_9mm, calendar::turn, 1 ),
                             pocket_type::MAGAZINE ).success() );
        const std::string name = remove_color_tags( gun.display_name() );
        INFO( name );

        CHECK( name.find( "1/1" ) != std::string::npos );


        CHECK( name.find( "1/0" ) == std::string::npos );
    }

    SECTION( "slug + battery does not aggregate" ) {
        REQUIRE( gun.put_in( item( itype_9mm, calendar::turn, 1 ),
                             pocket_type::MAGAZINE ).success() );
        item battery( itype_heavy_battery_cell );
        battery.ammo_set( itype_battery, 100 );
        REQUIRE( gun.put_in( battery, pocket_type::MAGAZINE_WELL ).success() );

        const std::string name = remove_color_tags( gun.display_name() );
        INFO( name );


        CHECK( name.find( "1/1" ) != std::string::npos );
        CHECK( name.find( "100/" ) != std::string::npos );
    }

    SECTION( "battery only does not borrow slug count" ) {
        item battery( itype_heavy_battery_cell );
        battery.ammo_set( itype_battery, 50 );
        REQUIRE( gun.put_in( battery, pocket_type::MAGAZINE_WELL ).success() );
        const std::string name = remove_color_tags( gun.display_name() );
        INFO( name );
        CHECK( name.find( "0/1" ) != std::string::npos );
        CHECK( name.find( "50/" ) != std::string::npos );
    }
}




TEST_CASE( "multimag_loaded_ammo_does_not_borrow_sibling_pocket",
           "[multimag][ammo_data]" )
{
    item gun( itype_test_multimag_gun_integral_ammo );
    REQUIRE( gun.uses_firing_requirements() );

    item battery( itype_heavy_battery_cell );
    battery.ammo_set( itype_battery, 100 );
    REQUIRE( gun.put_in( battery, pocket_type::MAGAZINE_WELL ).success() );

    CHECK_FALSE( gun.has_ammo() );
    CHECK_FALSE( gun.has_ammo_data() );
    CHECK( gun.ammo_data() == nullptr );
    CHECK( gun.loaded_ammo().is_null() );
    CHECK( gun.ammo_current() == itype_id::NULL_ID() );



    REQUIRE( gun.put_in( item( itype_9mm, calendar::turn, 1 ),
                         pocket_type::MAGAZINE ).success() );
    CHECK( gun.has_ammo() );
    CHECK( gun.has_ammo_data() );
    REQUIRE( gun.ammo_data() != nullptr );
    CHECK( gun.ammo_current() == itype_9mm );
    const item &loaded = gun.loaded_ammo();
    CHECK_FALSE( loaded.is_null() );
    CHECK( loaded.typeId() == itype_9mm );
}



TEST_CASE( "multimag_integral_pocket_skips_casings", "[multimag][ammo_data]" )
{
    item gun( itype_test_multimag_gun_integral_ammo );
    REQUIRE( gun.uses_firing_requirements() );

    item battery( itype_heavy_battery_cell );
    battery.ammo_set( itype_battery, 100 );
    REQUIRE( gun.put_in( battery, pocket_type::MAGAZINE_WELL ).success() );


    gun.force_insert_item( item( itype_9mm_casing, calendar::turn,
                                 1 ).set_flag( json_flag_CASING ),
                           pocket_type::MAGAZINE );
    CHECK_FALSE( gun.has_ammo() );
    CHECK_FALSE( gun.has_ammo_data() );
    CHECK( gun.ammo_data() == nullptr );
    CHECK( gun.ammo_remaining() == 100 );


    REQUIRE( gun.put_in( item( itype_9mm, calendar::turn, 1 ),
                         pocket_type::MAGAZINE ).success() );
    CHECK( gun.has_ammo() );
    CHECK( gun.has_ammo_data() );
    CHECK( gun.ammo_remaining() == 101 );
}



TEST_CASE( "multimag_ammo_capacity_resolves_per_pocket", "[multimag][capacity]" )
{
    item gun( itype_test_multimag_gun_integral_ammo );
    REQUIRE( gun.uses_firing_requirements() );


    CHECK( gun.ammo_capacity( ammo_9mm ) == 1 );

    item battery( itype_heavy_battery_cell );
    battery.ammo_set( itype_battery, 100 );
    REQUIRE( gun.put_in( battery, pocket_type::MAGAZINE_WELL ).success() );


    CHECK( gun.ammo_capacity( ammo_9mm ) == 1 );
    CHECK( gun.ammo_capacity( ammo_battery ) ==
           battery.ammo_capacity( ammo_battery ) );
}




TEST_CASE( "multimag_fire_without_projectile_does_not_crash", "[multimag][fire]" )
{
    clear_map();
    avatar &dummy = get_avatar();
    dummy.set_body();
    dummy.clear_worn();
    dummy.remove_weapon();
    dummy.setpos( get_map(), tripoint_bub_ms( point_bub_ms::zero, 0 ) );

    item gun( itype_test_multimag_gun_integral_ammo );
    item battery( itype_heavy_battery_cell );
    battery.ammo_set( itype_battery, 100 );
    REQUIRE( gun.put_in( battery, pocket_type::MAGAZINE_WELL ).success() );

    dummy.set_wielded_item( gun );
    REQUIRE( dummy.get_wielded_item() );

    map &here = get_map();
    const tripoint_bub_ms target = dummy.pos_bub( here ) + point( 5, 0 );
    const std::string dmsg = capture_debugmsg_during( [&]() {
        dummy.fire_gun( here, target, 1, *dummy.get_wielded_item() );
    } );
    INFO( "debugmsg: " << dmsg );
    CHECK( dmsg.find( "invalid recipe id" ) == std::string::npos );
    CHECK( dmsg.find( "Unexpected shortage of ammo" ) == std::string::npos );
}



TEST_CASE( "multimag_reload_targets_include_integral_pocket", "[multimag][reload]" )
{
    clear_avatar();
    avatar &dummy = get_avatar();
    item &gun = *dummy.i_add( item( itype_test_multimag_gun_integral_ammo ) );
    item_location gun_loc( dummy, &gun );

    const std::vector<reload_target> targets = get_possible_reload_targets( gun_loc );
    bool saw_integral = false;
    bool saw_well = false;
    for( const reload_target &rt : targets ) {
        if( rt.kind == reload_target::kind::integral_magazine ) {
            saw_integral = true;
        }
        if( rt.kind == reload_target::kind::well ) {
            saw_well = true;
        }
    }
    CHECK( saw_integral );
    CHECK( saw_well );


    item slug( itype_9mm, calendar::turn, 1 );
    item_location slug_loc( dummy, &slug );
    const reload_target *match = find_matching_reload_target( targets, slug_loc );
    REQUIRE( match != nullptr );
    CHECK( match->kind == reload_target::kind::integral_magazine );
}



TEST_CASE( "multimag_item_reload_into_integral_magazine_pocket", "[multimag][reload]" )
{
    clear_avatar();
    avatar &dummy = get_avatar();

    item gun( itype_test_multimag_gun_integral_ammo );
    item battery( itype_heavy_battery_cell );
    battery.ammo_set( itype_battery, 80 );
    REQUIRE( gun.put_in( battery, pocket_type::MAGAZINE_WELL ).success() );

    item_location held_loc = dummy.i_add( gun );
    item_location slug_loc = dummy.i_add( item( itype_9mm, calendar::turn, 1 ) );
    REQUIRE( held_loc );
    REQUIRE( slug_loc );
    item &held_gun = *held_loc;

    int slug_pocket_index = -1;
    int idx = 0;
    for( const item_pocket *p : held_gun.get_pockets( []( const item_pocket & ) {
    return true;
} ) ) {
        if( p->is_type( pocket_type::MAGAZINE ) ) {
            slug_pocket_index = idx;
            break;
        }
        ++idx;
    }
    REQUIRE( slug_pocket_index >= 0 );

    const bool reloaded = held_gun.reload( dummy, slug_loc, 1, slug_pocket_index );
    CHECK( reloaded );


    int slug_count = 0;
    int battery_count = 0;
    for( const item_pocket *p : held_gun.get_pockets( []( const item_pocket & ) {
    return true;
} ) ) {
        if( p->is_type( pocket_type::MAGAZINE ) ) {
            for( const item *e : p->all_items_top() ) {
                slug_count += e->charges > 0 ? e->charges : 1;
            }
        } else if( p->is_type( pocket_type::MAGAZINE_WELL ) ) {
            const item *m = p->magazine_current();
            if( m ) {
                battery_count = m->ammo_remaining();
            }
        }
    }
    CHECK( slug_count == 1 );

    CHECK( battery_count == 80 );
}
