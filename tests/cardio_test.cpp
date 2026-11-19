#include <memory>
#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "options.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"




























static const move_mode_id move_mode_run( "run" );

static const skill_id skill_swimming( "swimming" );

static const ter_str_id ter_t_pavement( "t_pavement" );


static const int base_cardio = 1000;


static const int base_stamina = 8500;


static void verify_default_cardio_options()
{
    const int max_stamina_base = get_option<int>( "PLAYER_MAX_STAMINA_BASE" );
    const int cardiofit_stamina_scaling = get_option<int>( "PLAYER_CARDIOFIT_STAMINA_SCALING" );
    REQUIRE( max_stamina_base == 3500 );
    REQUIRE( cardiofit_stamina_scaling == 5 );
}


static int running_steps( Character &they, const ter_str_id &terrain = ter_t_pavement )
{
    map &here = get_map();

    REQUIRE_FALSE( they.is_wearing_shoes() );
    REQUIRE_FALSE( they.is_npc() );

    const tripoint_bub_ms left = they.pos_bub();
    const tripoint_bub_ms right = left + tripoint::east;

    here.ter_set( left, terrain );
    here.ter_set( right, terrain );
    REQUIRE( here.ter( left ) == terrain );
    REQUIRE( here.ter( right ) == terrain );

    int steps = 0;
    int turns = 0;
    const int STOP_STEPS = 1000;

    int last_moves = they.get_speed();
    int last_stamina = they.get_stamina_max();

    they.set_moves( last_moves );
    they.set_stamina( last_stamina );
    they.set_movement_mode( move_mode_run );

    while( they.can_run() && steps < STOP_STEPS ) {

        if( steps % 2 == 0 ) {
            REQUIRE( they.pos_bub() == left );
            REQUIRE( g->walk_move( right, false, false ) );
        } else {
            REQUIRE( they.pos_bub() == right );
            REQUIRE( g->walk_move( left, false, false ) );
        }
        ++steps;


        REQUIRE( they.get_moves() < last_moves );
        const int move_cost = last_moves - they.get_moves();

        if( they.get_moves() <= 0 ) {

            they.mod_moves( they.get_speed() );
            calendar::turn += 1_turns;
            turns += 1;


            they.update_body();
            const int stamina_cost = last_stamina - they.get_stamina();

            CAPTURE( turns );
            CAPTURE( steps );
            CAPTURE( move_cost );
            CAPTURE( stamina_cost );
            REQUIRE( they.get_stamina() < last_stamina );
            last_stamina = they.get_stamina();
        }
        last_moves = they.get_moves();
    }

    they.setpos( here, left );
    return steps;
}


static void check_trait_cardio_stamina_run( Character &they, std::string trait_name,
        const int expect_cardio_fit, const int expect_stamina_max, const int expect_run_tiles )
{
    clear_avatar();
    if( trait_name.empty() ) {
        trait_name = "NONE";
    } else {
        set_single_trait( they, trait_name );
    }

    they.set_stored_kcal( they.get_healthy_kcal() );
    GIVEN( "trait: " + trait_name ) {
        CHECK( they.get_cardiofit() == Approx( expect_cardio_fit ).margin( 2 ) );
        CHECK( they.get_stamina_max() == Approx( expect_stamina_max ).margin( 5 ) );

        CHECK( running_steps( they ) == Approx( expect_run_tiles ).margin( 3 ) );
    }
}


TEST_CASE( "base_cardio", "[cardio][base]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();

    clear_map_without_vision();
    clear_avatar();


    REQUIRE( they.get_lifestyle() == 0 );
    REQUIRE( static_cast<int>( they.get_skill_level( skill_swimming ) ) == 0 );

    REQUIRE( they.get_cardiofit() == 1000 );

    they.recalc_limb_energy_usage();
    REQUIRE( they.get_working_leg_count() == 2 );
    REQUIRE( they.get_legs_power_use() == 0 );
    REQUIRE( they.get_legs_stam_mult() == 1 );

    SECTION( "Base character with no traits" ) {



        check_trait_cardio_stamina_run( they, "", base_cardio, base_stamina, 83 );
    }
}
















TEST_CASE( "cardio_is_and_is_not_affected_by_certain_traits", "[cardio][traits]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();

    clear_map_without_vision();
    clear_avatar();


    REQUIRE( they.get_lifestyle() == 0 );
    REQUIRE( static_cast<int>( they.get_skill_level( skill_swimming ) ) == 0 );

    REQUIRE( they.get_cardiofit() == 1000 );

    they.recalc_limb_energy_usage();
    REQUIRE( they.get_working_leg_count() == 2 );
    REQUIRE( they.get_legs_power_use() == 0 );
    REQUIRE( they.get_legs_stam_mult() == 1 );

    SECTION( "Base character with no traits" ) {

        check_trait_cardio_stamina_run( they, "", base_cardio, base_stamina, 81 );
    }


    SECTION( "Traits affecting body size" ) {
        check_trait_cardio_stamina_run( they, "SMALL2", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "SMALL", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "LARGE", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "HUGE", base_cardio, base_stamina, 81 );
    }

    SECTION( "Traits with CARDIO_MULTIPLIER" ) {




        check_trait_cardio_stamina_run( they, "BADCARDIO", 0.7 * base_cardio, 7000, 66 );

        check_trait_cardio_stamina_run( they, "GOODCARDIO", 1.3 * base_cardio, 10000, 103 );

        check_trait_cardio_stamina_run( they, "GOODCARDIO2", 1.6 * base_cardio, 11500, 121 );
    }

    SECTION( "Traits with metabolism_modifier AND STAMINA_REGEN_MOD" ) {

        check_trait_cardio_stamina_run( they, "HUNGER", base_cardio, base_stamina, 83 );

        check_trait_cardio_stamina_run( they, "HUNGER2", base_cardio, base_stamina, 85 );

        check_trait_cardio_stamina_run( they, "HUNGER3", base_cardio, base_stamina, 88 );
    }

    SECTION( "Traits with ONLY STAMINA_REGEN_MOD" ) {
        check_trait_cardio_stamina_run( they, "PERSISTENCE_HUNTER", base_cardio, base_stamina, 83 );
        check_trait_cardio_stamina_run( they, "PERSISTENCE_HUNTER2", base_cardio, base_stamina, 84 );
    }

    SECTION( "Traits with ONLY metabolism_modifier" ) {
        check_trait_cardio_stamina_run( they, "COLDBLOOD", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "COLDBLOOD2", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "COLDBLOOD3", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "COLDBLOOD4", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "LIGHTEATER", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "MET_RAT", base_cardio, base_stamina, 81 );
    }
}

TEST_CASE( "cardio_affects_stamina_regeneration", "[cardio][stamina]" )
{



}

TEST_CASE( "cardio_affects_weariness", "[cardio][weariness]" )
{

}

TEST_CASE( "cardio_is_affected_by_activity_level_each_day", "[cardio][activity]" )
{









}

TEST_CASE( "cardio_is_not_affected_by_character_height", "[cardio][height]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();
    clear_avatar();

    REQUIRE( they.size_class == creature_size::medium );

    SECTION( "No difference in cardio between heights" ) {
        they.set_base_height( Character::default_height( they.size_class ) );
        CHECK( they.get_cardiofit() == base_cardio );
        they.set_base_height( Character::max_height( they.size_class ) );
        CHECK( they.get_cardiofit() == base_cardio );
        they.set_base_height( Character::min_height( they.size_class ) );
        CHECK( they.get_cardiofit() == base_cardio );
    }
}

TEST_CASE( "cardio_is_affected_by_character_weight", "[cardio][weight]" )
{

}

TEST_CASE( "cardio_is_affected_by_character_health", "[cardio][health]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();
    clear_avatar();

    SECTION( "Hidden health stat adds directly to cardio fitness" ) {
        they.set_lifestyle( 0 );
        CHECK( they.get_cardiofit() == base_cardio );
        they.set_lifestyle( 200 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.20f ) );
        they.set_lifestyle( -200 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 0.60f ) );
    }
}

TEST_CASE( "cardio_is_affected_by_athletics_skill", "[cardio][athletics]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();
    clear_avatar();

    SECTION( "Athletics skill adds 10 per level to cardio fitness" ) {
        they.set_skill_level( skill_swimming, 0 );
        CHECK( they.get_cardiofit() == base_cardio );
        they.set_skill_level( skill_swimming, 1 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.01f ) );
        they.set_skill_level( skill_swimming, 2 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.02f ) );
        they.set_skill_level( skill_swimming, 3 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.03f ) );
        they.set_skill_level( skill_swimming, 4 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.04f ) );
        they.set_skill_level( skill_swimming, 5 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.05f ) );
        they.set_skill_level( skill_swimming, 6 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.06f ) );
        they.set_skill_level( skill_swimming, 7 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.07f ) );
        they.set_skill_level( skill_swimming, 8 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.08f ) );
        they.set_skill_level( skill_swimming, 9 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.09f ) );
        they.set_skill_level( skill_swimming, 10 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.10f ) );
    }
}
