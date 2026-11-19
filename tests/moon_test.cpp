#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "enum_conversions.h"

































TEST_CASE( "moon_phase_repeats_once_per_synodic_month", "[calendar][moon][phase][synodic]" )
{
    calendar::set_season_length( 91 );
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );


    REQUIRE( to_days<float>( lunar_month() ) == Approx( 29.53f ).margin( 0.001f ) );


    const time_duration ten_moons = 295_days;
    const time_duration hundred_moons = 2953_days;
    const time_point zero = calendar::turn_zero;


    SECTION( "first moon on day 0" ) {
        CHECK( get_moon_phase( zero ) == MOON_NEW );
        CHECK( get_moon_phase( zero + 14_days ) == MOON_FULL );
    }


    SECTION( "tenth moon on day 295" ) {
        CHECK( get_moon_phase( zero + ten_moons ) == MOON_NEW );
        CHECK( get_moon_phase( zero + ten_moons + 14_days ) == MOON_FULL );
    }


    SECTION( "hundredth moon on day 2953" ) {
        CHECK( get_moon_phase( zero + hundred_moons ) == MOON_NEW );
        CHECK( get_moon_phase( zero + hundred_moons + 14_days ) == MOON_FULL );
    }
}





TEST_CASE( "lunar_month_is_scaled_by_season_default_ratio", "[calendar][moon][month][season]" )
{
    const time_point zero = calendar::turn_zero;


    WHEN( "seasons are 91 days" ) {
        calendar::set_season_length( 91 );
        REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

        THEN( "it takes 14 days from new moon to full moon" ) {
            CHECK( get_moon_phase( zero ) == MOON_NEW );
            CHECK( get_moon_phase( zero + 14_days ) == MOON_FULL );
        }
    }


    WHEN( "seasons are 182 days" ) {
        calendar::set_season_length( 182 );
        REQUIRE( calendar::season_from_default_ratio() == Approx( 2.0f ) );

        THEN( "it takes 28 days from new moon to full moon" ) {
            CHECK( get_moon_phase( zero ) == MOON_NEW );
            CHECK( get_moon_phase( zero + 28_days ) == MOON_FULL );
        }
    }


    WHEN( "seasons are 45 days" ) {
        calendar::set_season_length( 45 );
        REQUIRE( calendar::season_from_default_ratio() == Approx( 0.5f ).margin( 0.01f ) );

        THEN( "it takes 7 days from new moon to full moon" ) {
            CHECK( get_moon_phase( zero ) == MOON_NEW );
            CHECK( get_moon_phase( zero + 7_days ) == MOON_FULL );
        }
    }

    calendar::set_season_length( 91 );
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );
}




TEST_CASE( "moon_phases_each_day_for_a_month", "[calendar][moon][phase]" )
{
    calendar::set_season_length( 91 );
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

    const time_point zero = calendar::turn_zero;

    CHECK( get_moon_phase( zero + 0_days ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 1_days ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 2_days ) == MOON_WAXING_CRESCENT );
    CHECK( get_moon_phase( zero + 3_days ) == MOON_WAXING_CRESCENT );
    CHECK( get_moon_phase( zero + 4_days ) == MOON_WAXING_CRESCENT );
    CHECK( get_moon_phase( zero + 5_days ) == MOON_WAXING_CRESCENT );
    CHECK( get_moon_phase( zero + 6_days ) == MOON_HALF_MOON_WAXING );
    CHECK( get_moon_phase( zero + 7_days ) == MOON_HALF_MOON_WAXING );
    CHECK( get_moon_phase( zero + 8_days ) == MOON_HALF_MOON_WAXING );
    CHECK( get_moon_phase( zero + 9_days ) == MOON_HALF_MOON_WAXING );
    CHECK( get_moon_phase( zero + 10_days ) == MOON_WAXING_GIBBOUS );
    CHECK( get_moon_phase( zero + 11_days ) == MOON_WAXING_GIBBOUS );
    CHECK( get_moon_phase( zero + 12_days ) == MOON_WAXING_GIBBOUS );
    CHECK( get_moon_phase( zero + 13_days ) == MOON_FULL );
    CHECK( get_moon_phase( zero + 14_days ) == MOON_FULL );
    CHECK( get_moon_phase( zero + 15_days ) == MOON_FULL );
    CHECK( get_moon_phase( zero + 16_days ) == MOON_FULL );
    CHECK( get_moon_phase( zero + 17_days ) == MOON_WANING_GIBBOUS );
    CHECK( get_moon_phase( zero + 18_days ) == MOON_WANING_GIBBOUS );
    CHECK( get_moon_phase( zero + 19_days ) == MOON_WANING_GIBBOUS );
    CHECK( get_moon_phase( zero + 20_days ) == MOON_WANING_GIBBOUS );
    CHECK( get_moon_phase( zero + 21_days ) == MOON_HALF_MOON_WANING );
    CHECK( get_moon_phase( zero + 22_days ) == MOON_HALF_MOON_WANING );
    CHECK( get_moon_phase( zero + 23_days ) == MOON_HALF_MOON_WANING );
    CHECK( get_moon_phase( zero + 24_days ) == MOON_WANING_CRESCENT );
    CHECK( get_moon_phase( zero + 25_days ) == MOON_WANING_CRESCENT );
    CHECK( get_moon_phase( zero + 26_days ) == MOON_WANING_CRESCENT );
    CHECK( get_moon_phase( zero + 27_days ) == MOON_WANING_CRESCENT );
    CHECK( get_moon_phase( zero + 28_days ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 29_days ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 30_days ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 31_days ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 32_days ) == MOON_WAXING_CRESCENT );
}




TEST_CASE( "moon_phase_changes_at_noon", "[calendar][moon][phase][change]" )
{
    calendar::set_season_length( 91 );
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

    const time_point zero = calendar::turn_zero;

    CHECK( get_moon_phase( zero + 1_days + 11_hours ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 1_days + 13_hours ) == MOON_WAXING_CRESCENT );

    CHECK( get_moon_phase( zero + 5_days + 11_hours ) == MOON_WAXING_CRESCENT );
    CHECK( get_moon_phase( zero + 5_days + 13_hours ) == MOON_HALF_MOON_WAXING );

    CHECK( get_moon_phase( zero + 9_days + 11_hours ) == MOON_HALF_MOON_WAXING );
    CHECK( get_moon_phase( zero + 9_days + 13_hours ) == MOON_WAXING_GIBBOUS );

    CHECK( get_moon_phase( zero + 12_days + 11_hours ) == MOON_WAXING_GIBBOUS );
    CHECK( get_moon_phase( zero + 12_days + 13_hours ) == MOON_FULL );

    CHECK( get_moon_phase( zero + 16_days + 11_hours ) == MOON_FULL );
    CHECK( get_moon_phase( zero + 16_days + 13_hours ) == MOON_WANING_GIBBOUS );

    CHECK( get_moon_phase( zero + 20_days + 11_hours ) == MOON_WANING_GIBBOUS );
    CHECK( get_moon_phase( zero + 20_days + 13_hours ) == MOON_HALF_MOON_WANING );

    CHECK( get_moon_phase( zero + 23_days + 11_hours ) == MOON_HALF_MOON_WANING );
    CHECK( get_moon_phase( zero + 23_days + 13_hours ) == MOON_WANING_CRESCENT );

    CHECK( get_moon_phase( zero + 27_days + 11_hours ) == MOON_WANING_CRESCENT );
    CHECK( get_moon_phase( zero + 27_days + 13_hours ) == MOON_NEW );

    CHECK( get_moon_phase( zero + 31_days + 11_hours ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 31_days + 13_hours ) == MOON_WAXING_CRESCENT );
}







TEST_CASE( "moonlight_at_dawn_and_dusk", "[calendar][moon][moonlight][dawn][dusk]" )
{
    calendar::set_season_length( 91 );
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

    WHEN( "moon is new" ) {
        time_point new_phase = calendar::turn_zero;

        time_point new_midnight = new_phase - time_past_midnight( new_phase );
        time_point new_sunrise = sunrise( new_midnight );
        time_point new_sunset = sunset( new_midnight );
        time_point new_noon = new_midnight + 12_hours;


        float daylight_level = sun_moon_light_at( new_noon );
        CHECK( daylight_level == Approx( 110 ).margin( 10 ) );
        float moonlight_level = 1.0f;

        THEN( "at night, light is only moonlight" ) {
            CHECK( sun_moon_light_at( new_sunset + 2_hours ) == moonlight_level );
            CHECK( sun_moon_light_at( new_midnight ) == moonlight_level );
            CHECK( sun_moon_light_at( new_sunrise - 2_hours ) == moonlight_level );
        }
        THEN( "at dawn, light increases from moonlight to daylight" ) {
            CHECK( sun_moon_light_at( new_sunrise ) > sun_moon_light_at( new_sunrise - 2_hours ) );
            CHECK( sun_moon_light_at( new_sunrise + 1_hours ) ==
                   sun_light_at( new_sunrise + 1_hours ) + moonlight_level );
        }
    }

    WHEN( "moon is full" ) {
        time_point full_phase = calendar::turn_zero + lunar_month() / 2;

        time_point full_midnight = full_phase - time_past_midnight( full_phase );
        time_point full_sunrise = sunrise( full_midnight );
        time_point full_sunset = sunset( full_midnight );
        time_point full_noon = full_midnight + 12_hours;


        float daylight_level = sun_moon_light_at( full_noon );
        CHECK( daylight_level == Approx( 120 ).margin( 10 ) );
        float moonlight_level = 7.0f;

        THEN( "at night, light is only moonlight" ) {
            CHECK( sun_moon_light_at( full_sunset + 2_hours ) == moonlight_level );
            CHECK( sun_moon_light_at( full_midnight ) == moonlight_level );
            CHECK( sun_moon_light_at( full_sunrise - 2_hours ) == moonlight_level );
        }
        THEN( "at dawn, light increases from moonlight to daylight" ) {
            CHECK( sun_moon_light_at( full_sunrise ) >
                   sun_moon_light_at( full_sunrise - 2_hours ) );
            CHECK( sun_moon_light_at( full_sunrise + 1_hours ) ==
                   sun_light_at( full_sunrise + 1_hours ) + moonlight_level );
        }
    }
}


static time_point lunar_month_night( const float phase_scale )
{
    REQUIRE( 0.0f <= phase_scale );
    REQUIRE( phase_scale <= 2.0f );


    time_point this_phase = calendar::turn_zero + phase_scale * lunar_month() / 2;


    time_point this_night;
    time_duration past_midnight = time_past_midnight( this_phase );


    if( past_midnight < 12_hours ) {
        this_night = this_phase - past_midnight;
    } else {
        this_night = this_phase - past_midnight + 24_hours;
    }

    return this_night;
}


static float phase_moonlight( const float phase_scale, const moon_phase expect_phase_enum )
{

    time_point this_night = lunar_month_night( phase_scale );


    CAPTURE( phase_scale );
    CHECK( io::enum_to_string( get_moon_phase( this_night ) ) == io::enum_to_string(
               expect_phase_enum ) );


    return sun_moon_light_at( this_night );
}


TEST_CASE( "moonlight_for_each_phase", "[calendar][moon][phase][moonlight]" )
{
    calendar::set_season_length( 91 );
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );


    SECTION( "moonlight increases as moon goes from new to full" ) {
        CHECK( 1.00f == phase_moonlight( 0.0f, MOON_NEW ) );
        CHECK( 2.5f == phase_moonlight( 0.25f, MOON_WAXING_CRESCENT ) );
        CHECK( 4.f == phase_moonlight( 0.5f, MOON_HALF_MOON_WAXING ) );
        CHECK( 5.5f == phase_moonlight( 0.75f, MOON_WAXING_GIBBOUS ) );
        CHECK( 7.0f == phase_moonlight( 1.0f, MOON_FULL ) );
        CHECK( 5.5f == phase_moonlight( 1.25f, MOON_WANING_GIBBOUS ) );
        CHECK( 4.f == phase_moonlight( 1.5f, MOON_HALF_MOON_WANING ) );
        CHECK( 2.5f == phase_moonlight( 1.75f, MOON_WANING_CRESCENT ) );
    }

    SECTION( "moonlight is constant during each phase" ) {


        CHECK( 1.0f == phase_moonlight( 0.0f, MOON_NEW ) );
        CHECK( 1.0f == phase_moonlight( 0.10f, MOON_NEW ) );



        CHECK( 2.5f == phase_moonlight( 0.11f, MOON_WAXING_CRESCENT ) );
        CHECK( 2.5f == phase_moonlight( 0.20f, MOON_WAXING_CRESCENT ) );
        CHECK( 2.5f == phase_moonlight( 0.30f, MOON_WAXING_CRESCENT ) );
    }
}
