#include <functional>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "creature.h"
#include "display.h"
#include "game_constants.h"
#include "monster.h"
#include "options.h"
#include "output.h"
#include "player_helpers.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units.h"

static const mtype_id mon_cow( "mon_cow" );
static const mtype_id mon_dog_gpyrenees( "mon_dog_gpyrenees" );
static const mtype_id mon_dog_gshepherd( "mon_dog_gshepherd" );
static const mtype_id mon_horse( "mon_horse" );
static const mtype_id mon_pig( "mon_pig" );

static const trait_id trait_HUGE( "HUGE" );
static const trait_id trait_LARGE( "LARGE" );
static const trait_id trait_SMALL( "SMALL" );
static const trait_id trait_SMALL2( "SMALL2" );





static float bmi_to_kcal_ratio( float bmi )
{
    return bmi / 5.0;
}


static void set_player_bmi( Character &dummy, float bmi )
{
    dummy.set_stored_kcal( dummy.get_healthy_kcal() * bmi_to_kcal_ratio( bmi ) );
    REQUIRE( dummy.get_bmi_fat() == Approx( bmi ).margin( 0.01f ) );
}











static float true_bmi_at_kcal_ratio( Character &dummy, float kcal_percent )
{
    dummy.set_stored_kcal( dummy.get_healthy_kcal() * kcal_percent );
    REQUIRE( dummy.get_kcal_percent() == Approx( kcal_percent ).margin( 0.01f ) );

    return dummy.get_bmi();
}


static int max_healthy_at_bmi( Character &dummy, float bmi )
{
    set_player_bmi( dummy, bmi );
    return dummy.get_max_healthy();
}


static int kcal_speed_penalty_at_bmi( Character &dummy, float bmi )
{
    set_player_bmi( dummy, bmi );
    return dummy.kcal_speed_penalty();
}


static std::string weight_string_at_bmi( Character &dummy, float bmi )
{
    set_player_bmi( dummy, bmi );
    return remove_color_tags( display::weight_string( dummy ) );
}


static float bodyweight_kg_at_bmi( Character &dummy, float bmi )
{
    set_player_bmi( dummy, bmi );
    return to_kilogram( dummy.bodyweight() );
}


static float metabolic_rate_with_mutation( Character &dummy, const std::string &trait_name )
{
    set_single_trait( dummy, trait_name );
    return dummy.metabolic_rate_base();
}


static int bmr_at_act_level( Character &dummy, float activity_level )
{
    dummy.reset_activity_level();
    dummy.set_stored_kcal( dummy.get_healthy_kcal() );
    dummy.update_body( calendar::turn, calendar::turn );
    dummy.update_body( calendar::turn, calendar::turn + 10_minutes );
    dummy.set_activity_level( activity_level );

    return dummy.get_bmr();
}

using avatar_ptr = std::shared_ptr<avatar>;


static std::map<creature_size, avatar_ptr> create_dummies_of_all_sizes( int init_height )
{
    const std::map<creature_size, std::string> size_mutations{
        { creature_size::tiny, "SMALL2" },
        { creature_size::small, "SMALL" },
        { creature_size::medium, "" },
        { creature_size::large, "LARGE" },
        { creature_size::huge, "HUGE" },
    };

    std::map<creature_size, avatar_ptr> dummies;
    for( const auto &pair : size_mutations ) {
        const creature_size &size = pair.first;
        const std::string &trait_name = pair.second;

        avatar_ptr dummy = std::make_shared<avatar>();
        clear_character( *dummy );

        if( !trait_name.empty() ) {
            set_single_trait( *dummy, trait_name );
        }
        REQUIRE( dummy->get_size() == size );

        dummy->mod_base_height( init_height - dummy->base_height() );
        REQUIRE( dummy->base_height() == init_height );

        dummies[size] = dummy;
    }
    return dummies;
}

static void for_each_size_category( const std::function< void( creature_size ) > &functor )
{
    for( int i = static_cast< int >( creature_size::tiny );
         i < static_cast< int >( creature_size::num_sizes ); ++i ) {
        creature_size size = static_cast< creature_size >( i );
        functor( size );
    }
}



TEST_CASE( "body_mass_index_determines_weight_description", "[biometrics][bmi][weight]" )
{
    avatar dummy;

    CHECK( weight_string_at_bmi( dummy, 0.1f ) == "Skeletal" );
    CHECK( weight_string_at_bmi( dummy, 0.9f ) == "Skeletal" );

    CHECK( weight_string_at_bmi( dummy, 1.1f ) == "Emaciated" );
    CHECK( weight_string_at_bmi( dummy, 1.9f ) == "Emaciated" );

    CHECK( weight_string_at_bmi( dummy, 2.1f ) == "Underweight" );
    CHECK( weight_string_at_bmi( dummy, 2.9f ) == "Underweight" );

    CHECK( weight_string_at_bmi( dummy, 3.1f ) == "Normal" );
    CHECK( weight_string_at_bmi( dummy, 4.9f ) == "Normal" );

    CHECK( weight_string_at_bmi( dummy, 5.1f ) == "Overweight" );
    CHECK( weight_string_at_bmi( dummy, 9.9f ) == "Overweight" );

    CHECK( weight_string_at_bmi( dummy, 10.1f ) == "Obese" );
    CHECK( weight_string_at_bmi( dummy, 14.9f ) == "Obese" );

    CHECK( weight_string_at_bmi( dummy, 15.1f ) == "Very Obese" );
    CHECK( weight_string_at_bmi( dummy, 19.9f ) == "Very Obese" );

    CHECK( weight_string_at_bmi( dummy, 20.1f ) == "Morbidly Obese" );
    CHECK( weight_string_at_bmi( dummy, 21.0f ) == "Morbidly Obese" );

}


TEST_CASE( "stored_kcal_ratio_influences_body_mass_index", "[biometrics][kcal][bmi]" )
{
    avatar dummy;


    CHECK( true_bmi_at_kcal_ratio( dummy, 0.0f ) == Approx( 12.0f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 0.05f ) == Approx( 13.25f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 0.1f ) == Approx( 14.5f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 0.15f ) == Approx( 14.75f ).margin( 0.01f ) );

    CHECK( true_bmi_at_kcal_ratio( dummy, 0.2f ) == Approx( 16.0f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 0.25f ) == Approx( 17.25f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 0.3f ) == Approx( 17.5f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 0.35f ) == Approx( 18.75f ).margin( 0.01f ) );

    CHECK( true_bmi_at_kcal_ratio( dummy, 0.4f ) == Approx( 20.0f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 0.5f ) == Approx( 21.5f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 0.6f ) == Approx( 23.0f ).margin( 0.01f ) );

    CHECK( true_bmi_at_kcal_ratio( dummy, 0.7f ) == Approx( 23.5f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 0.8f ) == Approx( 24.0f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 0.9f ) == Approx( 24.5f ).margin( 0.01f ) );

    CHECK( true_bmi_at_kcal_ratio( dummy, 1.0f ) == Approx( 25.0f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 1.25f ) == Approx( 26.25f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 1.5f ) == Approx( 27.5f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 1.75f ) == Approx( 28.75f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 2.0f ) == Approx( 30.0f ).margin( 0.01f ) );

    CHECK( true_bmi_at_kcal_ratio( dummy, 2.25f ) == Approx( 31.25f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 2.5f ) == Approx( 32.5f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 2.75f ) == Approx( 33.75f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 3.0f ) == Approx( 35.0f ).margin( 0.01f ) );

    CHECK( true_bmi_at_kcal_ratio( dummy, 3.5f ) == Approx( 37.5f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 4.0f ) == Approx( 40.0f ).margin( 0.01f ) );

    CHECK( true_bmi_at_kcal_ratio( dummy, 4.5f ) == Approx( 42.5f ).margin( 0.01f ) );
    CHECK( true_bmi_at_kcal_ratio( dummy, 5.0f ) == Approx( 45.0f ).margin( 0.01f ) );
}

TEST_CASE( "body_mass_index_determines_maximum_healthiness", "[biometrics][bmi][max]" )
{





    avatar dummy;


    CHECK( max_healthy_at_bmi( dummy, 0.1f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 0.4f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 0.8f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 1.0f ) == -200 );

    CHECK( max_healthy_at_bmi( dummy, 1.3f ) == -140 );
    CHECK( max_healthy_at_bmi( dummy, 1.6f ) == -80 );
    CHECK( max_healthy_at_bmi( dummy, 1.9f ) == -20 );

    CHECK( max_healthy_at_bmi( dummy, 2.1f ) == 20 );
    CHECK( max_healthy_at_bmi( dummy, 2.3f ) == 60 );
    CHECK( max_healthy_at_bmi( dummy, 2.5f ) == 100 );
    CHECK( max_healthy_at_bmi( dummy, 2.8f ) == 160 );
    CHECK( max_healthy_at_bmi( dummy, 3.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 3.5f ) == 200 );

    CHECK( max_healthy_at_bmi( dummy, 3.6f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 3.8f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 4.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 4.2f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 4.5f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 4.8f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 5.0f ) == 200 );

    CHECK( max_healthy_at_bmi( dummy, 6.0f ) == 175 );
    CHECK( max_healthy_at_bmi( dummy, 7.0f ) == 150 );
    CHECK( max_healthy_at_bmi( dummy, 8.0f ) == 125 );
    CHECK( max_healthy_at_bmi( dummy, 9.0f ) == 100 );

    CHECK( max_healthy_at_bmi( dummy, 10.0f ) == 75 );
    CHECK( max_healthy_at_bmi( dummy, 11.0f ) == 50 );
    CHECK( max_healthy_at_bmi( dummy, 12.0f ) == 25 );
    CHECK( max_healthy_at_bmi( dummy, 13.0f ) == 0 );
    CHECK( max_healthy_at_bmi( dummy, 14.0f ) == -25 );

    CHECK( max_healthy_at_bmi( dummy, 15.0f ) == -50 );
    CHECK( max_healthy_at_bmi( dummy, 16.0f ) == -75 );
    CHECK( max_healthy_at_bmi( dummy, 17.0f ) == -100 );
    CHECK( max_healthy_at_bmi( dummy, 18.0f ) == -125 );
    CHECK( max_healthy_at_bmi( dummy, 19.0f ) == -150 );

    CHECK( max_healthy_at_bmi( dummy, 20.0f ) == -175 );
    CHECK( max_healthy_at_bmi( dummy, 21.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 22.0f ) == -200 );
}

TEST_CASE( "character_height_should_increase_with_their_body_size",
           "[biometrics][height][mutation]" )
{
    using DummyMap = std::map<creature_size, avatar_ptr>;
    DummyMap dummies_default_height = create_dummies_of_all_sizes( Character::default_height() );
    DummyMap dummies_min_height = create_dummies_of_all_sizes( Character::min_height() );
    DummyMap dummies_max_height = create_dummies_of_all_sizes( Character::max_height() );

    auto test_absolute_heights = []( DummyMap & dummies ) {
        for_each_size_category( [ &dummies ]( creature_size size ) {
            CHECK( dummies[size]->height() >= Character::min_height( size ) );
            CHECK( dummies[size]->height() <= Character::max_height( size ) );
            if( size != creature_size::huge ) {
                creature_size next_size = static_cast< creature_size >( static_cast< int >( size ) + 1 );
                CHECK( dummies[size]->height() < dummies[next_size]->height() );
            }
        } );
    };

    GIVEN( "default height character" ) {
        test_absolute_heights( dummies_default_height );
    }
    GIVEN( "min height character" ) {
        test_absolute_heights( dummies_min_height );
    }
    GIVEN( "max height character" ) {
        test_absolute_heights( dummies_max_height );
    }
}

TEST_CASE( "default_character_175_cm_bodyweights_at_various_BMIs", "[biometrics][bodyweight]" )
{
    avatar dummy;
    clear_character( dummy );




    GIVEN( "character height started at 175cm" ) {
        dummy.mod_base_height( 175 - dummy.base_height() );
        REQUIRE( dummy.base_height() == 175 );

        WHEN( "character is normal-sized (medium)" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_SMALL2 ) );
            REQUIRE_FALSE( dummy.has_trait( trait_SMALL ) );
            REQUIRE_FALSE( dummy.has_trait( trait_LARGE ) );
            REQUIRE_FALSE( dummy.has_trait( trait_HUGE ) );
            REQUIRE( dummy.get_size() == creature_size::medium );

            THEN( "bodyweight varies from ~49-107kg" ) {


                CHECK( bodyweight_kg_at_bmi( dummy, 1.0 ) == Approx( 49.0 ).margin( 0.1f ) );

                CHECK( bodyweight_kg_at_bmi( dummy, 5.0 ) == Approx( 76.6 ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 15.0 ) == Approx( 107.2 ).margin( 0.1f ) );
            }
        }
    }
}

TEST_CASE( "character_weight_should_increase_with_their_body_size_and_BMI",
           "[biometrics][bodyweight][mutation]" )
{
    using DummyMap = std::map<creature_size, avatar_ptr>;
    DummyMap dummies_default_height = create_dummies_of_all_sizes( Character::default_height() );
    DummyMap dummies_min_height = create_dummies_of_all_sizes( Character::min_height() );
    DummyMap dummies_max_height = create_dummies_of_all_sizes( Character::max_height() );

    auto test_weights = []( DummyMap & dummies ) {
        for_each_size_category( [&dummies]( creature_size size ) {
            if( size != creature_size::huge ) {
                creature_size next_size = static_cast< creature_size >( static_cast< int >( size ) + 1 );
                CHECK( dummies[size]->bodyweight() < dummies[next_size]->bodyweight() );
            }
            avatar_ptr &dummy = dummies[size];
            CHECK( bodyweight_kg_at_bmi( *dummy, 3.0f ) < bodyweight_kg_at_bmi( *dummy, 5.0f ) );
            CHECK( bodyweight_kg_at_bmi( *dummy, 5.0f ) < bodyweight_kg_at_bmi( *dummy, 15.0f ) );
        } );
    };

    GIVEN( "default height character" ) {
        test_weights( dummies_default_height );
    }
    GIVEN( "min height character" ) {
        test_weights( dummies_min_height );
    }
    GIVEN( "max height character" ) {
        test_weights( dummies_max_height );
    }
}

TEST_CASE( "riding_various_creatures_at_various_sizes", "[avatar][bodyweight]" )
{
    monster cow( mon_cow );
    monster horse( mon_horse );
    monster pig( mon_pig );
    monster large_dog( mon_dog_gpyrenees );
    monster average_dog( mon_dog_gshepherd );

    using DummyMap = std::map<creature_size, avatar_ptr>;
    DummyMap dummies_default_height = create_dummies_of_all_sizes( Character::default_height() );
    DummyMap dummies_min_height = create_dummies_of_all_sizes( Character::min_height() );
    DummyMap dummies_max_height = create_dummies_of_all_sizes( Character::max_height() );

    auto can_mount = []( const avatar_ptr & dummy, const monster & steed ) {
        dummy->set_stored_kcal( dummy->get_healthy_kcal() );
        return dummy->bodyweight() <= steed.get_weight() * steed.get_mountable_weight_ratio();
    };

    SECTION( "default height character can ride all large steeds but not the small ones" ) {
        avatar_ptr dummy = dummies_default_height[creature_size::medium];
        CHECK( can_mount( dummy, cow ) );
        CHECK( can_mount( dummy, horse ) );
        CHECK( !can_mount( dummy, pig ) );
        CHECK( !can_mount( dummy, large_dog ) );
        CHECK( !can_mount( dummy, average_dog ) );
    }

    SECTION( "characters of any starting height can ride a horse" ) {
        CHECK( can_mount( dummies_min_height[creature_size::medium], horse ) );
        CHECK( can_mount( dummies_default_height[creature_size::medium], horse ) );
        CHECK( can_mount( dummies_max_height[creature_size::medium], horse ) );
    }

    SECTION( "huge characters can't ride a horse" ) {
        CHECK( !can_mount( dummies_min_height[creature_size::huge], horse ) );
        CHECK( !can_mount( dummies_default_height[creature_size::huge], horse ) );
        CHECK( !can_mount( dummies_max_height[creature_size::huge], horse ) );
    }

    SECTION( "only short tiny characters can ride large dogs" ) {
        CHECK( can_mount( dummies_min_height[creature_size::tiny], large_dog ) );
        CHECK( !can_mount( dummies_default_height[creature_size::tiny], large_dog ) );
        CHECK( !can_mount( dummies_max_height[creature_size::tiny], large_dog ) );
    }

    SECTION( "nobody can ride smaller dogs" ) {
        CHECK( !can_mount( dummies_min_height[creature_size::tiny], average_dog ) );
        CHECK( !can_mount( dummies_default_height[creature_size::tiny], average_dog ) );
        CHECK( !can_mount( dummies_max_height[creature_size::tiny], average_dog ) );
    }
}


static std::string condensed_spaces( const std::string &text )
{
    std::regex spaces( " +" );
    return std::regex_replace( text, spaces, " " );
}


static void test_activity_duration( avatar &dummy, const float at_level,
                                    const time_duration &how_long )
{

    for( int turn = 0; turn < to_turns<int>( how_long ); ++turn ) {

        dummy.set_activity_level( at_level );
        calendar::turn += 1_turns;
        dummy.update_body();
    }
}

TEST_CASE( "activity_levels_and_calories_in_daily_diary", "[avatar][biometrics][activity][diary]" )
{

    calendar::turn = calendar::turn_zero + 60_days;

    avatar dummy;
    clear_character( dummy );

    SECTION( "shows all zero at start of day 61" ) {
        CHECK( condensed_spaces( dummy.total_daily_calories_string() ) ==
               "<color_c_white> Minutes at each exercise level Calories per day</color>\n"
               "<color_c_yellow> Day Sleep None Light Moderate Brisk Active Extra Gained Spent Total</color>\n"
               "<color_c_light_gray> 61 0 0 0 0 0 0 0 0 0</color><color_c_light_gray> 0</color>\n" );
    }

    SECTION( "shows time at each activity level for the current day" ) {
        dummy.reset_activity_level();
        test_activity_duration( dummy, NO_EXERCISE, 59_minutes );
        test_activity_duration( dummy, LIGHT_EXERCISE, 45_minutes );
        test_activity_duration( dummy, MODERATE_EXERCISE, 30_minutes );
        test_activity_duration( dummy, BRISK_EXERCISE, 20_minutes );
        test_activity_duration( dummy, ACTIVE_EXERCISE, 15_minutes );
        test_activity_duration( dummy, EXTRA_EXERCISE, 10_minutes );
        test_activity_duration( dummy, NO_EXERCISE, 1_minutes );

        int expect_gained_kcal = 1283;
        int expect_net_kcal = 527;
        int expect_spent_kcal = 756;

        CHECK( condensed_spaces( dummy.total_daily_calories_string() ) == string_format(
                   "<color_c_white> Minutes at each exercise level Calories per day</color>\n"
                   "<color_c_yellow> Day Sleep None Light Moderate Brisk Active Extra Gained Spent Total</color>\n"
                   "<color_c_light_gray> 61 0 60 45 30 20 20 5 %d %d</color><color_c_light_blue> %d</color>\n",
                   expect_gained_kcal, expect_spent_kcal, expect_net_kcal ) );
    }
}

TEST_CASE( "mutations_may_affect_character_metabolic_rate", "[biometrics][metabolism]" )
{
    avatar dummy;
    dummy.set_body();



    const float normal_metabolic_rate = get_option<float>( "PLAYER_HUNGER_RATE" );
    dummy.clear_mutations();
    CHECK( dummy.metabolic_rate_base() == normal_metabolic_rate );



    REQUIRE( normal_metabolic_rate == 1.0f );














    CHECK( metabolic_rate_with_mutation( dummy, "MET_RAT" ) == Approx( 1.333f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "HUNGER" ) == Approx( 1.5f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "HUNGER2" ) == Approx( 2.0f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "HUNGER3" ) == Approx( 3.0f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "LIGHTEATER" ) == Approx( 0.667f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "COLDBLOOD" ) == Approx( 0.667f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "COLDBLOOD2" ) == Approx( 0.5f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "COLDBLOOD3" ) == Approx( 0.5f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "COLDBLOOD4" ) == Approx( 0.5f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "TEST_ZERO_METABOLIC" ) == 0.0f );
}

TEST_CASE( "basal_metabolic_rate_with_various_size_and_metabolism", "[biometrics][bmr]" )
{
    avatar dummy;
    dummy.set_body();



    REQUIRE( dummy.metabolic_rate_base() == 1.0f );


    set_player_bmi( dummy, 5.0f );
    REQUIRE( dummy.get_bmi() == Approx( 25.0f ).margin( 0.001f ) );







    SECTION( "normal body size" ) {
        REQUIRE( dummy.get_size() == creature_size::medium );

        SECTION( "normal metabolism" ) {
            CHECK( 1738 == bmr_at_act_level( dummy, NO_EXERCISE ) );
            CHECK( 6952 == bmr_at_act_level( dummy, MODERATE_EXERCISE ) );
            CHECK( 17380 == bmr_at_act_level( dummy, EXTRA_EXERCISE ) );
        }

        SECTION( "very fast metabolism" ) {
            set_single_trait( dummy, "HUNGER2" );
            REQUIRE( dummy.metabolic_rate_base() == 2.0f );

            CHECK( 3476 == bmr_at_act_level( dummy, NO_EXERCISE ) );
            CHECK( 13904 == bmr_at_act_level( dummy, MODERATE_EXERCISE ) );
            CHECK( 34760 == bmr_at_act_level( dummy, EXTRA_EXERCISE ) );
        }

        SECTION( "very slow (cold-blooded) metabolism" ) {
            set_single_trait( dummy, "COLDBLOOD3" );
            REQUIRE( dummy.metabolic_rate_base() == 0.5f );

            CHECK( 869 == bmr_at_act_level( dummy, NO_EXERCISE ) );
            CHECK( 3476 == bmr_at_act_level( dummy, MODERATE_EXERCISE ) );
            CHECK( 8690 == bmr_at_act_level( dummy, EXTRA_EXERCISE ) );
        }
    }


    std::map<creature_size, avatar_ptr> dummies = create_dummies_of_all_sizes( 175 );
    const std::vector<float> excercise_levels{ NO_EXERCISE, MODERATE_EXERCISE, EXTRA_EXERCISE };
    for( float level : excercise_levels ) {
        CHECK( bmr_at_act_level( *dummies[creature_size::tiny], level ) <
               bmr_at_act_level( *dummies[creature_size::small], level ) );
        CHECK( bmr_at_act_level( *dummies[creature_size::small], level ) <
               bmr_at_act_level( *dummies[creature_size::medium], level ) );
        CHECK( bmr_at_act_level( *dummies[creature_size::medium], level ) <
               bmr_at_act_level( *dummies[creature_size::large], level ) );
        CHECK( bmr_at_act_level( *dummies[creature_size::large ], level ) <
               bmr_at_act_level( *dummies[creature_size::huge], level ) );
    }


    const std::vector<creature_size> sizes{ creature_size::tiny, creature_size::small, creature_size::medium, creature_size::large, creature_size::huge };
    for( creature_size size : sizes ) {
        CHECK( bmr_at_act_level( *dummies[size], NO_EXERCISE ) <
               bmr_at_act_level( *dummies[size], LIGHT_EXERCISE ) );
        CHECK( bmr_at_act_level( *dummies[size], LIGHT_EXERCISE ) <
               bmr_at_act_level( *dummies[size], MODERATE_EXERCISE ) );
        CHECK( bmr_at_act_level( *dummies[size], MODERATE_EXERCISE ) <
               bmr_at_act_level( *dummies[size], BRISK_EXERCISE ) );
        CHECK( bmr_at_act_level( *dummies[size], BRISK_EXERCISE ) <
               bmr_at_act_level( *dummies[size], ACTIVE_EXERCISE ) );
        CHECK( bmr_at_act_level( *dummies[size], ACTIVE_EXERCISE ) <
               bmr_at_act_level( *dummies[size], EXTRA_EXERCISE ) );
    }
}

TEST_CASE( "body_mass_effect_on_speed", "[bmi][speed]" )
{
    avatar dummy;


    CHECK( kcal_speed_penalty_at_bmi( dummy, 0.1f ) == -86 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 0.2f ) == -82 );

    CHECK( kcal_speed_penalty_at_bmi( dummy, 0.4f ) == -74 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 0.5f ) == -70 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 0.6f ) == -66 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 0.7f ) == -62 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 0.9f ) == -54 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 1.0f ) == -50 );

    CHECK( kcal_speed_penalty_at_bmi( dummy, 1.3f ) == -43 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 1.6f ) == -35 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 1.8f ) == -30 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 2.0f ) == -25 );

    CHECK( kcal_speed_penalty_at_bmi( dummy, 2.2f ) == -20 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 2.5f ) == -13 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 2.8f ) == -5 );

    CHECK( kcal_speed_penalty_at_bmi( dummy, 3.1f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 3.4f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 3.5f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 3.7f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 3.9f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 4.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 4.2f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 4.4f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 4.6f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 4.8f ) == 0 );

    CHECK( kcal_speed_penalty_at_bmi( dummy, 5.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 6.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 7.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 9.0f ) == 0 );

    CHECK( kcal_speed_penalty_at_bmi( dummy, 10.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 11.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 12.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 13.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 14.0f ) == 0 );

    CHECK( kcal_speed_penalty_at_bmi( dummy, 15.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 16.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 17.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 18.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 19.0f ) == 0 );

    CHECK( kcal_speed_penalty_at_bmi( dummy, 20.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 21.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 22.0f ) == 0 );
}
