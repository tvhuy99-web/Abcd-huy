#include <array>
#include <functional>
#include <memory>
#include <string>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "item.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"

static const itype_id itype_balclava( "balclava" );
static const itype_id itype_bandana( "bandana" );
static const itype_id itype_boots( "boots" );
static const itype_id itype_boots_winter( "boots_winter" );
static const itype_id itype_coat_winter( "coat_winter" );
static const itype_id itype_gloves_fingerless( "gloves_fingerless" );
static const itype_id itype_gloves_liner( "gloves_liner" );
static const itype_id itype_gloves_winter( "gloves_winter" );
static const itype_id itype_gloves_wool( "gloves_wool" );
static const itype_id itype_goggles_ski( "goggles_ski" );
static const itype_id itype_hat_ball( "hat_ball" );
static const itype_id itype_hat_hunting( "hat_hunting" );
static const itype_id itype_hat_knit( "hat_knit" );
static const itype_id itype_jeans( "jeans" );
static const itype_id itype_long_underpants( "long_underpants" );
static const itype_id itype_pants_army( "pants_army" );
static const itype_id itype_pants_fur( "pants_fur" );
static const itype_id itype_sneakers( "sneakers" );
static const itype_id itype_socks( "socks" );
static const itype_id itype_socks_wool( "socks_wool" );
static const itype_id itype_test_jumpsuit_cotton( "test_jumpsuit_cotton" );
static const itype_id itype_test_jumpsuit_fur( "test_jumpsuit_fur" );
static const itype_id itype_test_jumpsuit_lycra( "test_jumpsuit_lycra" );
static const itype_id itype_trenchcoat( "trenchcoat" );
static const itype_id itype_tshirt( "tshirt" );
static const itype_id itype_under_armor( "under_armor" );
static const itype_id itype_vest( "vest" );



static void temperature_check( Character *p, const int ambient_temp,
                               const units::temperature target_temp )
{
    p->set_body();
    get_weather().temperature = units::from_fahrenheit( ambient_temp );
    p->set_all_parts_temp_cur( BODYTEMP_NORM );
    p->set_all_parts_temp_conv( BODYTEMP_NORM );

    units::temperature prev_temp = 0_K;
    units::temperature_delta prev_diff = 0_C_delta;
    for( int i = 0; i < 10000; i++ ) {
        const units::temperature torso_temp_cur = p->get_part_temp_cur( bodypart_id( "torso" ) );
        if( prev_diff != prev_temp - torso_temp_cur ) {
            prev_diff = prev_temp - torso_temp_cur;
        } else if( prev_temp == torso_temp_cur ) {
            break;
        }
        prev_temp = torso_temp_cur;
        p->update_bodytemp();
    }
    const units::temperature high = target_temp + 0.2_C_delta;
    const units::temperature low = target_temp - 0.2_C_delta;
    CHECK( low < p->get_part_temp_cur( bodypart_id( "torso" ) ) );
    CHECK( high > p->get_part_temp_cur( bodypart_id( "torso" ) ) );
}



static void temperature_and_sweat_check( Character *p, const int ambient_temp,
        const units::temperature target_temp )
{

    weather_manager &weather = get_weather();

    weather.temperature = units::from_fahrenheit( ambient_temp );

    for( int i = 0; i < 1000; i++ ) {
        p->process_effects();
        p->update_bodytemp();
        p->update_body_wetness( *weather.weather_precise );
    }
    const units::temperature high = target_temp + 0.2_C_delta;
    const units::temperature low = target_temp - 0.2_C_delta;
    CHECK( low < p->get_part_temp_cur( bodypart_id( "torso" ) ) );
    CHECK( high > p->get_part_temp_cur( bodypart_id( "torso" ) ) );
}

static void equip_clothing( Character *p, const itype_id &clothing )
{
    const item article( clothing, calendar::turn_zero );
    p->wear_item( article );
}



static void test_temperature_spread( Character *p, const std::array<int, 7> &ambient_temps )
{
    temperature_check( p, ambient_temps[0], BODYTEMP_FREEZING );
    temperature_check( p, ambient_temps[1], BODYTEMP_VERY_COLD );
    temperature_check( p, ambient_temps[2], BODYTEMP_COLD );
    temperature_check( p, ambient_temps[3], BODYTEMP_NORM );
    temperature_check( p, ambient_temps[4], BODYTEMP_HOT );
    temperature_check( p, ambient_temps[5], BODYTEMP_VERY_HOT );
    temperature_check( p, ambient_temps[6], BODYTEMP_SCORCHING );
}

TEST_CASE( "player_body_temperatures_converge_on_expected_values", "[.bodytemp]" )
{

    Character &dummy = get_player_character();


    dummy.remove_worn_items_with( []( const item & ) {
        return true;
    } );











    SECTION( "Nude target temperatures." ) {

        test_temperature_spread( &dummy, {{ -12, 15, 40, 64, 78, 90, 101 }} );
    }

    SECTION( "Lightly clothed target temperatures" ) {
        equip_clothing( &dummy, itype_hat_ball );
        equip_clothing( &dummy, itype_bandana );
        equip_clothing( &dummy, itype_tshirt );
        equip_clothing( &dummy, itype_gloves_fingerless );
        equip_clothing( &dummy, itype_jeans );
        equip_clothing( &dummy, itype_socks );
        equip_clothing( &dummy, itype_sneakers );


        test_temperature_spread( &dummy, {{ -17, 10, 35, 62, 77, 90, 100 }} );
    }

    SECTION( "Heavily clothed target temperatures" ) {
        equip_clothing( &dummy, itype_hat_knit );
        equip_clothing( &dummy, itype_tshirt );
        equip_clothing( &dummy, itype_vest );
        equip_clothing( &dummy, itype_trenchcoat );
        equip_clothing( &dummy, itype_gloves_wool );
        equip_clothing( &dummy, itype_long_underpants );
        equip_clothing( &dummy, itype_pants_army );
        equip_clothing( &dummy, itype_socks_wool );
        equip_clothing( &dummy, itype_boots );


        test_temperature_spread( &dummy, {{ -39, -14, 17, 46, 70, 84, 96 }} );
    }

    SECTION( "Arctic gear target temperatures" ) {
        equip_clothing( &dummy, itype_balclava );
        equip_clothing( &dummy, itype_goggles_ski );
        equip_clothing( &dummy, itype_hat_hunting );
        equip_clothing( &dummy, itype_under_armor );
        equip_clothing( &dummy, itype_vest );
        equip_clothing( &dummy, itype_coat_winter );
        equip_clothing( &dummy, itype_gloves_liner );
        equip_clothing( &dummy, itype_gloves_winter );
        equip_clothing( &dummy, itype_long_underpants );
        equip_clothing( &dummy, itype_pants_fur );
        equip_clothing( &dummy, itype_socks_wool );
        equip_clothing( &dummy, itype_boots_winter );


        test_temperature_spread( &dummy, {{ -115, -87, -54, -6, 36, 64, 80 }} );
    }
}

TEST_CASE( "sweating", "[char][suffer][.bodytemp]" )
{
    avatar &dummy = get_avatar();
    clear_character( dummy );


    item fur_jumper( itype_test_jumpsuit_fur );
    item lycra_jumper( itype_test_jumpsuit_lycra );
    item cotton_jumper( itype_test_jumpsuit_cotton );

    GIVEN( "avatar wears outfit and sweats for an hour" ) {
        WHEN( "wearing fur" ) {
            dummy.clear_worn();
            dummy.wear_item( fur_jumper, false );

            temperature_and_sweat_check( &dummy, 100, 43.2_C );
        }
        WHEN( "wearing cotton" ) {
            dummy.clear_worn();
            dummy.wear_item( cotton_jumper, false );

            temperature_and_sweat_check( &dummy, 100, 42.8_C );
        }
        WHEN( "wearing lycra" ) {
            dummy.clear_worn();
            dummy.wear_item( lycra_jumper, false );

            temperature_and_sweat_check( &dummy, 100, 41_C );
        }

    }
}
