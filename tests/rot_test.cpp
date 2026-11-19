#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "enums.h"
#include "item.h"
#include "map.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"

static const flag_id json_flag_FROZEN( "FROZEN" );

static const itype_id itype_meat_cooked( "meat_cooked" );
static const itype_id itype_offal_canned( "offal_canned" );

static void set_map_temperature( units::temperature new_temperature )
{
    get_weather().temperature = new_temperature;
    get_weather().clear_temp_cache();
}

TEST_CASE( "Rate_of_rotting", "[rot]" )
{
    SECTION( "Passage of time" ) {








        if( calendar::turn <= calendar::start_of_cataclysm ) {
            calendar::turn = calendar::start_of_cataclysm + 1_minutes;
        }

        item normal_item( itype_meat_cooked );

        item freeze_item( itype_offal_canned );

        item sealed_item( itype_offal_canned );
        sealed_item = sealed_item.in_its_container();

        set_map_temperature( units::from_fahrenheit( 65 ) );

        normal_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::NORMAL );
        sealed_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::NORMAL );
        freeze_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::NORMAL );


        CHECK( normal_item.get_rot() == 0_turns );
        CHECK( sealed_item.get_rot() == 0_turns );
        CHECK( freeze_item.get_rot() == 0_turns );

        INFO( "Initial turn: " << to_turn<int>( calendar::turn ) );

        calendar::turn += 20_minutes;
        sealed_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::NORMAL );
        normal_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::NORMAL );
        freeze_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::FREEZER );


        CHECK( to_turns<int>( normal_item.get_rot() )
               == Approx( to_turns<int>( 20_minutes ) ).epsilon( 0.01 ) );

        CHECK( sealed_item.get_rot() == 0_turns );
        CHECK( freeze_item.get_rot() == 0_turns );


        calendar::turn += 110_minutes;
        sealed_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::NORMAL );
        freeze_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::FREEZER );

        CHECK( sealed_item.get_rot() == 0_turns );
        CHECK( freeze_item.get_rot() == 0_turns );


        CHECK( !freeze_item.has_own_flag( json_flag_FROZEN ) );
    }
}

TEST_CASE( "Items_rot_away", "[rot]" )
{
    SECTION( "Item in reality bubble rots away" ) {


        if( calendar::turn <= calendar::start_of_cataclysm ) {
            calendar::turn = calendar::start_of_cataclysm + 1_minutes;
        }

        item test_item( itype_meat_cooked );


        test_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::HEATER );


        calendar::turn += 20_minutes;
        test_item.mod_rot( 2_days );

        CHECK( test_item.process_temperature_rot( 1, tripoint_bub_ms::zero, get_map(), nullptr,
                temperature_flag::HEATER ) );
        INFO( "Rot: " << to_turns<int>( test_item.get_rot() ) );
    }
}

TEST_CASE( "Hourly_rotpoints", "[rot]" )
{
    item normal_item( itype_meat_cooked );


    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_celsius( 0 ) ) == 0 );


    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_celsius( 63 ) ) == Approx(
               20364.67 ) );


    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_celsius( 62 ) ) == Approx(
               20364.67 ) );


    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_fahrenheit( 65 ) ) == Approx(
               3600 ) );


    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_fahrenheit( 65 + 16 ) ) == Approx(
               3600.0 * 2 ) );


    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_fahrenheit( 65 - 16 ) ) == Approx(
               3600.0 / 2 ) );


    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_fahrenheit( 35 ) ) == Approx(
               1117.672 / 2 ) );


    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_fahrenheit( 107 ) ) == Approx(
               20364.67 ) );
}
