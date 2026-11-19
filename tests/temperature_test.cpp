#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "enums.h"
#include "flag.h"
#include "game_constants.h"
#include "item.h"
#include "map.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"

static const itype_id itype_meat_cooked( "meat_cooked" );
static const itype_id itype_water( "water" );

static void set_map_temperature( units::temperature new_temperature )
{
    get_weather().temperature = new_temperature;
    get_weather().clear_temp_cache();
}

TEST_CASE( "Item_spawns_with_right_thermal_attributes", "[temperature]" )
{
    item D( itype_meat_cooked );

    CHECK( D.get_specific_heat_liquid() == 3.7f );
    CHECK( D.get_specific_heat_solid() == 2.15f );
    CHECK( D.get_latent_heat() == 260 );

    CHECK( units::to_kelvin( D.temperature ) == 0 );
    CHECK( units::to_joule_per_gram( D.specific_energy ) == -10 );

    set_map_temperature( units::from_fahrenheit( 122 ) );
    D.process_temperature_rot( 1, tripoint_bub_ms::zero, get_map(), nullptr );

    CHECK( units::to_kelvin( D.temperature ) == Approx( 323.15 ) );
}

TEST_CASE( "Rate_of_temperature_change", "[temperature]" )
{













    map &here = get_map();
    SECTION( "Water bottle test (ralisticity)" ) {










        item water1( itype_water );
        item water2( itype_water );

        set_map_temperature( units::from_fahrenheit( 131 ) );

        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        water2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );


        CHECK( units::to_kelvin( water1.temperature ) == Approx( 328.15 ) );

        set_map_temperature( units::from_fahrenheit( 68 ) );

        calendar::turn += 11_minutes;
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 20_minutes;
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 29_minutes;
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        water2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 15_minutes;
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        water2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );


        CHECK( units::to_kelvin( water1.temperature ) == Approx( 302.71802 ) );
        CHECK( units::to_kelvin( water1.temperature ) == Approx( units::to_kelvin( water2.temperature ) ) );
    }

    SECTION( "Hot liquid to frozen" ) {







        item meat1( itype_meat_cooked );
        item meat2( itype_meat_cooked );

        set_map_temperature( units::from_fahrenheit( 122 ) );

        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );


        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 323.15 ) );
        CHECK( meat1.has_own_flag( flag_HOT ) );

        set_map_temperature( units::from_fahrenheit( -4 ) );

        calendar::turn += 15_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );


        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 307.78338 ) );
        CHECK( !meat1.has_own_flag( flag_HOT ) );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 30_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );



        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 273.15 ) );
        CHECK( units::to_kelvin( meat2.temperature ) == Approx( 273.15 ) );
        CHECK( !meat1.has_own_flag( flag_FROZEN ) );
        CHECK( !meat2.has_own_flag( flag_FROZEN ) );

        calendar::turn += 60_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 60_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );




        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 273.15 ) );;
        CHECK( meat1.has_own_flag( flag_FROZEN ) );
        CHECK( meat2.has_own_flag( flag_FROZEN ) );
        CHECK( units::to_joule_per_gram( meat1.specific_energy ) == Approx( units::to_joule_per_gram(
                    meat2.specific_energy ) ) );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 50_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );




        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 267.98050 ) );
        CHECK( meat1.has_own_flag( flag_FROZEN ) );
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( units::to_kelvin( meat2.temperature ) ) );
    }

    SECTION( "Cold solid to liquid" ) {










        item meat1( itype_meat_cooked );
        item meat2( itype_meat_cooked );

        set_map_temperature( units::from_fahrenheit( -4 ) );

        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );


        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 253.15 ) );
        CHECK( meat1.has_own_flag( flag_FROZEN ) );

        set_map_temperature( units::from_fahrenheit( 68 ) );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 263.89390 ) );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );




        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 273.15 ) );
        CHECK( units::to_kelvin( meat2.temperature ) == units::to_kelvin( meat1.temperature ) );
        CHECK( meat1.has_own_flag( flag_FROZEN ) );
        CHECK( meat2.has_own_flag( flag_FROZEN ) );

        calendar::turn += 45_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 45_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );




        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 273.15 ) );
        CHECK( units::to_kelvin( meat2.temperature ) == units::to_kelvin( meat1.temperature ) );
        CHECK( !meat1.has_own_flag( flag_FROZEN ) );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 50_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );


        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 275.32468 ) );
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( units::to_kelvin( meat2.temperature ) ) );
    }
}

TEST_CASE( "Temperature_controlled_location", "[temperature]" )
{
    SECTION( "Heater test" ) {




        item water1( itype_water );

        set_map_temperature( units::from_fahrenheit( 0 ) );

        map &here = get_map();
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr,
                                        temperature_flag::HEATER );

        CHECK( units::to_kelvin( water1.temperature ) == Approx( units::to_kelvin(
                    temperatures::normal ) ) );

        calendar::turn += 15_minutes;
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr,
                                        temperature_flag::HEATER );

        CHECK( units::to_kelvin( water1.temperature ) == Approx( units::to_kelvin(
                    temperatures::normal ) ) );

        calendar::turn += 2_hours + 3_minutes;
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr,
                                        temperature_flag::HEATER );

        CHECK( units::to_kelvin( water1.temperature ) == Approx( units::to_kelvin(
                    temperatures::normal ) ) );
    }
}
