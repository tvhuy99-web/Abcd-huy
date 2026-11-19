#include <climits>
#include <map>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "type_id.h"

static const itype_id itype_UPS_ON( "UPS_ON" );
static const itype_id itype_battery_ups( "battery_ups" );
static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_heavy_battery_cell( "heavy_battery_cell" );
static const itype_id itype_medium_battery_cell( "medium_battery_cell" );
static const itype_id itype_test_can_food( "test_can_food" );
static const itype_id itype_test_cordless_drill( "test_cordless_drill" );
static const itype_id itype_test_liquid( "test_liquid" );
static const itype_id itype_test_sonic_screwdriver( "test_sonic_screwdriver" );

static const quality_id qual_BOIL( "BOIL" );
static const quality_id qual_DRILL( "DRILL" );
static const quality_id qual_LOCKPICK( "LOCKPICK" );
static const quality_id qual_PRY( "PRY" );
static const quality_id qual_SCREW( "SCREW" );
static const quality_id qual_SCREW_FINE( "SCREW_FINE" );
static const quality_id qual_WRENCH( "WRENCH" );











TEST_CASE( "get_quality", "[tool][quality]" )
{
    SECTION( "get_quality returns numeric level of given quality" ) {
        item sonic( itype_test_sonic_screwdriver );
        CHECK( sonic.get_quality( qual_LOCKPICK ) == 30 );
        CHECK( sonic.get_quality( qual_PRY ) == 2 );
        CHECK( sonic.get_quality( qual_SCREW ) == 2 );
        CHECK( sonic.get_quality( qual_SCREW_FINE ) == 1 );
        CHECK( sonic.get_quality( qual_WRENCH ) == 1 );
    }

    SECTION( "charged_qualities" ) {

        SECTION( "with 0 charge, get_quality returns 0" ) {
            CHECK( tool_with_ammo( itype_test_cordless_drill, 0 ).get_quality( qual_DRILL ) == 0 );
        }
        SECTION( "with sufficient charge, get_quality returns quality level" ) {
            CHECK( tool_with_ammo( itype_test_cordless_drill, 20 ).get_quality( qual_DRILL ) == 3 );
        }
    }

    SECTION( "BOIL quality" ) {

        item tin_can( itype_test_can_food );
        SECTION( "get_quality returns BOIL quality if container is empty" ) {
            REQUIRE( tin_can.empty_container() );

            CHECK( tin_can.get_quality( qual_BOIL ) == 1 );
        }
        SECTION( "get_quality returns INT_MIN for BOIL quality if container is not empty" ) {
            item broth( itype_test_liquid );
            tin_can.put_in( broth, pocket_type::CONTAINER );
            REQUIRE_FALSE( tin_can.empty_container() );

            CHECK( tin_can.get_quality( qual_BOIL ) == INT_MIN );

            CHECK( tin_can.get_quality( qual_BOIL, false ) == 1 );
        }
    }
}



TEST_CASE( "battery-powered_tool_qualities", "[tool][battery][quality]" )
{
    item drill( itype_test_cordless_drill );
    item battery( itype_medium_battery_cell );


    REQUIRE( drill.is_tool() );
    REQUIRE( drill.type->charges_to_use() > 0 );


    REQUIRE( drill.type->qualities.size() == 1 );

    REQUIRE( drill.type->charged_qualities.size() == 1 );






    WHEN( "tool has no battery" ) {
        REQUIRE_FALSE( drill.magazine_current() );


        THEN( "inherent qualities can be used" ) {
            CHECK( drill.has_quality( qual_SCREW, 1, 1 ) );
        }
        THEN( "charged tool qualities cannot be used" ) {
            CHECK_FALSE( drill.has_quality( qual_DRILL, 3, 1 ) );
        }
    }

    WHEN( "tool has a battery with zero charge" ) {

        battery.ammo_set( battery.ammo_default(), 0 );
        REQUIRE( battery.ammo_remaining( ) == 0 );

        drill.put_in( battery, pocket_type::MAGAZINE_WELL );
        REQUIRE( drill.magazine_current() );
        REQUIRE( drill.ammo_remaining( ) == 0 );


        THEN( "inherent qualities can be used" ) {
            CHECK( drill.has_quality( qual_SCREW, 1, 1 ) );
        }
        THEN( "charged tool qualities cannot be used" ) {
            CHECK_FALSE( drill.has_quality( qual_DRILL, 3, 1 ) );
        }
    }

    WHEN( "tool has a battery with enough charge for one use, equal to charges_per_use" ) {

        int bat_charges = drill.type->charges_to_use();
        battery.ammo_set( battery.ammo_default(), bat_charges );
        REQUIRE( battery.ammo_remaining( ) == bat_charges );

        drill.put_in( battery, pocket_type::MAGAZINE_WELL );
        REQUIRE( drill.magazine_current() );
        REQUIRE( drill.ammo_remaining( ) == bat_charges );


        REQUIRE( drill.type->charges_to_use() <= bat_charges );


        THEN( "both inherent and charged tool qualities can be used" ) {
            CHECK( drill.has_quality( qual_SCREW, 1, 1 ) );
            CHECK( drill.has_quality( qual_DRILL, 3, 1 ) );
        }
    }

    WHEN( "tool has a battery with insufficient charge, less than charges_per_use" ) {

        int bat_charges = drill.type->charges_to_use() - 1;
        battery.ammo_set( battery.ammo_default(), bat_charges );
        REQUIRE( battery.ammo_remaining( ) == bat_charges );

        drill.put_in( battery, pocket_type::MAGAZINE_WELL );
        REQUIRE( drill.magazine_current() );
        REQUIRE( drill.ammo_remaining( ) == bat_charges );


        REQUIRE( drill.type->charges_to_use() > bat_charges );


        THEN( "inherent qualities can be used" ) {
            CHECK( drill.has_quality( qual_SCREW, 1, 1 ) );
        }
        THEN( "charged tool qualities cannot be used" ) {
            CHECK_FALSE( drill.has_quality( qual_DRILL, 3, 1 ) );
        }
    }

    SECTION( "charged tool qualities with UPS conversion mod" ) {

        Character &they = get_player_character();
        clear_character( they );
        they.worn.wear_item( they, item( itype_debug_backpack ), false, false );



        item_location drill = they.i_add( item( itype_test_cordless_drill ) );
        item_location bat_cell = they.i_add( item( itype_heavy_battery_cell ) );
        item_location ups_mod = they.i_add( item( itype_battery_ups ) );
        item_location ups = they.i_add( item( itype_UPS_ON ) );

        GIVEN( "UPS has battery with enough charge, equal to drill's charges_per_use" ) {

            int bat_charges = drill->type->charges_to_use();
            REQUIRE( bat_charges > 0 );
            bat_cell->ammo_set( bat_cell->ammo_default(), bat_charges );
            REQUIRE( bat_cell->ammo_remaining( ) == bat_charges );

            REQUIRE( ups->put_in( *bat_cell, pocket_type::MAGAZINE_WELL ).success() );
            REQUIRE( ups->ammo_remaining( &they ) == bat_charges );

            WHEN( "UPS battery mod is installed into the drill" ) {

                REQUIRE( drill->toolmods().empty() );
                REQUIRE( drill->tname() == "test cordless drill" );

                drill->put_in( *ups_mod, pocket_type::MOD );
                REQUIRE_FALSE( drill->toolmods().empty() );
                REQUIRE( drill->tname() == "test cordless drill+1 (UPS)" );

                CHECK( they.has_item( *drill ) );
                CHECK( they.has_item( *ups ) );

                THEN( "the drill has the same charge as the UPS" ) {
                    CHECK( drill->ammo_remaining( &they ) == bat_charges );
                }
                THEN( "inherent qualities of the drill can be used" ) {
                    CHECK( drill->has_quality( qual_SCREW, 1, 1 ) );
                }

                THEN( "charged qualities of the drill can be used" ) {
                    CHECK( drill->has_quality( qual_DRILL, 3, 1 ) );
                }
            }
        }
    }
}
