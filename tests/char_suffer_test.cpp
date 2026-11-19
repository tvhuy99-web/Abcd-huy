#include <map>
#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "flag.h"
#include "game.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "weather_type.h"

static const efftype_id effect_grabbed( "grabbed" );

static const itype_id itype_test_hazmat_suit( "test_hazmat_suit" );
static const itype_id itype_test_longshirt( "test_longshirt" );
static const itype_id itype_test_sunglasses( "test_sunglasses" );
static const itype_id itype_test_umbrella( "test_umbrella" );
static const itype_id itype_test_zentai( "test_zentai" );

static const ter_str_id ter_t_rock_wall( "t_rock_wall" );

static const trait_id trait_ALBINO( "ALBINO" );
static const trait_id trait_SUNBURN( "SUNBURN" );







static void test_suffer( Character &dummy, const time_duration &dur, bool update_body = false )
{
    const int num_turns = to_turns<int>( dur );
    for( int turn = 0; turn < num_turns; ++turn ) {
        dummy.suffer();
        if( update_body ) {
            dummy.update_body( calendar::turn, calendar::turn + 1_turns );
        }
    }
}


static int test_suffer_focus_lost( Character &dummy, const time_duration &dur )
{
    int focus_lost = 0;
    const int num_turns = to_turns<int>( dur );
    for( int turn = 0; turn < num_turns; ++turn ) {
        dummy.set_focus( 100 );
        dummy.suffer();
        focus_lost += 100 - dummy.get_focus();
    }
    return focus_lost;
}


static std::map<bodypart_id, int> test_suffer_bodypart_hp_lost( Character &dummy,
        const time_duration &dur )
{
    const std::vector<bodypart_id> body_parts_with_hp = dummy.get_all_body_parts(
                get_body_part_flags::only_main );

    std::map<bodypart_id, int> bp_hp_lost;
    for( const bodypart_id &bp : body_parts_with_hp ) {
        bp_hp_lost[bp] = 0;
    }

    const int num_turns = to_turns<int>( dur );
    for( int turn = 0; turn < num_turns; ++turn ) {
        dummy.healall( 100 );
        dummy.suffer();
        for( const bodypart_id &bp : body_parts_with_hp ) {
            bp_hp_lost[bp] += dummy.get_part_hp_max( bp.id() ) - dummy.get_part_hp_cur( bp.id() );
        }
    }
    return bp_hp_lost;
}


static int test_suffer_pain_felt( Character &dummy, const time_duration &dur )
{
    int pain_felt = 0;
    const int num_turns = to_turns<int>( dur );
    for( int turn = 0; turn < num_turns; ++turn ) {
        dummy.set_pain( 0 );
        dummy.suffer();
        pain_felt += dummy.get_pain();
    }
    return pain_felt;
}









TEST_CASE( "suffering_from_albinism", "[char][suffer][albino]" )
{
    clear_map_without_vision();
    clear_avatar();
    set_time_to_day();
    scoped_weather_override clear_weather( WEATHER_CLEAR );
    avatar &dummy = get_avatar();

    int focus_lost = 0;





    item shades( itype_test_sunglasses );

    item umbrella( itype_test_umbrella );


    item hazmat( itype_test_hazmat_suit );

    item zentai( itype_test_zentai );

    item longshirt( itype_test_longshirt );

    GIVEN( "avatar is in sunlight with the albino trait" ) {
        REQUIRE( g->is_in_sunlight( dummy.pos_bub() ) );

        dummy.toggle_trait( trait_ALBINO );
        REQUIRE( dummy.has_trait( trait_ALBINO ) );

        WHEN( "totally naked and exposed" ) {
            dummy.clear_worn();


            THEN( "they lose 80 to 280 focus per hour" ) {
                focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                CHECK( focus_lost == Approx( 180 ).margin( 100 ) );
            }









        }

        WHEN( "wielding an umbrella and wearing sunglasses" ) {
            dummy.wield( umbrella );
            REQUIRE( dummy.get_wielded_item()->has_flag( flag_RAIN_PROTECT ) );

            dummy.wear_item( shades, false );
            REQUIRE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

            THEN( "they suffer no pain or loss of focus" ) {
                focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                CHECK( dummy.get_pain() == 0 );
                CHECK( focus_lost == 0 );
            }
        }

        WHEN( "entire body is covered with clothing" ) {
            dummy.clear_worn();
            dummy.wear_item( zentai, false );
















            WHEN( "wearing sunglasses" ) {
                dummy.wear_item( shades, false );
                REQUIRE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

                THEN( "they suffer no pain or loss of focus" ) {
                    focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                    CHECK( dummy.get_pain() == 0 );
                    CHECK( focus_lost == 0 );
                }
            }
        }
    }
}









TEST_CASE( "suffering_from_sunburn", "[char][suffer][sunburn]" )
{
    clear_map_without_vision();
    clear_avatar();
    set_time_to_day();
    scoped_weather_override clear_weather( WEATHER_CLEAR );
    Character &dummy = get_player_character();
    const std::vector<bodypart_id> body_parts_with_hp = dummy.get_all_body_parts(
                get_body_part_flags::only_main );

    int focus_lost = 0;
    int pain_felt = 0;

    item shades( itype_test_sunglasses );
    item umbrella( itype_test_umbrella );
    item zentai( itype_test_zentai );
    item longshirt( itype_test_longshirt );

    GIVEN( "avatar is in sunlight with the solar sensitivity trait" ) {
        REQUIRE( g->is_in_sunlight( dummy.pos_bub() ) );

        dummy.toggle_trait( trait_SUNBURN );
        REQUIRE( dummy.has_trait( trait_SUNBURN ) );

        std::map<bodypart_id, int> bp_hp_lost;
        WHEN( "totally naked and exposed, with sunglasses" ) {
            dummy.clear_worn();
            dummy.wear_item( shades, false );
            REQUIRE( dummy.worn_with_flag( flag_SUN_GLASSES ) );












            THEN( "they suffer pain several times a minute" ) {

                pain_felt = test_suffer_pain_felt( dummy, 1_hours );
                CHECK( pain_felt == Approx( 15 * 11 ).margin( 14 * 11 ) );
            }
        }

        WHEN( "naked and wielding an umbrella, with sunglasses" ) {
            dummy.clear_worn();
            dummy.wield( umbrella );
            REQUIRE( dummy.get_wielded_item()->has_flag( flag_RAIN_PROTECT ) );
            dummy.wear_item( shades, false );
            REQUIRE( dummy.worn_with_flag( flag_SUN_GLASSES ) );


            THEN( "they suffer no injury" ) {
                bp_hp_lost = test_suffer_bodypart_hp_lost( dummy, 1_hours );
                for( const bodypart_id &bp : body_parts_with_hp ) {
                    CAPTURE( bp.id().str() );
                    CHECK( bp_hp_lost[bp] == 0 );
                }
            }
            THEN( "they suffer no pain" ) {
                pain_felt = test_suffer_pain_felt( dummy, 1_hours );
                CHECK( pain_felt == 0 );
            }
        }

        WHEN( "wielding an umbrella, without sunglasses" ) {
            dummy.clear_worn();
            dummy.wield( umbrella );
            REQUIRE( dummy.get_wielded_item()->has_flag( flag_RAIN_PROTECT ) );
            REQUIRE_FALSE( dummy.worn_with_flag( flag_SUN_GLASSES ) );
            THEN( "they suffer only head injury" ) {
                bp_hp_lost = test_suffer_bodypart_hp_lost( dummy, 1_hours );
                for( const bodypart_id &bp : body_parts_with_hp ) {
                    CAPTURE( bp.id().str() );




                    if( bp != bodypart_id( "head" ) ) {
                        CHECK( bp_hp_lost[bp] == 0 );
                    }
                }
            }





        }

        WHEN( "torso and arms are 90% covered" ) {
            dummy.clear_worn();
            dummy.wear_item( longshirt, false );

            THEN( "damage to torso is 0 and halved for arms" ) {
                time_duration t = 1_hours;

                bp_hp_lost = test_suffer_bodypart_hp_lost( dummy, t );
                for( const bodypart_id &bp : body_parts_with_hp ) {
                    CAPTURE( bp.id().str() );
                    if( bp.id().str() == "torso" ) {
                        CHECK( bp_hp_lost[bp] == 0 );
                    }









                }
            }
        }

        WHEN( "entire body is covered" ) {
            dummy.clear_worn();
            dummy.wear_item( zentai, false );

            WHEN( "not wearing sunglasses" ) {
                REQUIRE_FALSE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

                THEN( "they suffer loss of focus" ) {




                    focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                    CHECK( focus_lost == Approx( 39 ).margin( 30 ) );
                }






            }

            WHEN( "wearing sunglasses" ) {
                dummy.wear_item( shades, false );
                REQUIRE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

                THEN( "they suffer no injury" ) {
                    bp_hp_lost = test_suffer_bodypart_hp_lost( dummy, 10_minutes );
                    for( const bodypart_id &bp : body_parts_with_hp ) {
                        CAPTURE( bp.id().str() );
                        CHECK( bp_hp_lost[bp] == 0 );
                    }
                }

                THEN( "they suffer no pain or loss of focus" ) {
                    focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                    CHECK( dummy.get_pain() == 0 );
                    CHECK( focus_lost == 0 );
                }
            }
        }
    }
}

TEST_CASE( "suffering_from_asphyxiation", "[char][suffer][oxygen][grab]" )
{
    clear_map_without_vision();
    clear_avatar();
    Character &dummy = get_player_character();
    dummy.set_underwater( false );
    dummy.oxygen = dummy.get_oxygen_max();
    REQUIRE( dummy.oxygen == 46 );

    GIVEN( "character is not grabbed or underwater" ) {
        REQUIRE( !dummy.is_underwater() );
        REQUIRE( !dummy.has_effect( effect_grabbed, body_part_torso ) );
        dummy.oxygen = 0;

        WHEN( "at full stamina" ) {
            REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() );

            THEN( "recover 5 stamina per turn" ) {
                test_suffer( dummy, 3_turns, true );
                CHECK( dummy.oxygen == 15 );
            }
        }

        WHEN( "at 3/4 stamina" ) {
            dummy.set_stamina( ( dummy.get_stamina_max() * 3 ) / 4 );
            REQUIRE( dummy.get_stamina() == ( dummy.get_stamina_max() * 3 ) / 4 );

            THEN( "recover 3 stamina per turn" ) {
                test_suffer( dummy, 3_turns, true );
                CHECK( dummy.oxygen == 9 );
            }
        }

        WHEN( "at 1/2 stamina" ) {
            dummy.set_stamina( dummy.get_stamina_max() / 2 );
            REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() / 2 );

            THEN( "recover 2 stamina per turn" ) {
                test_suffer( dummy, 3_turns, true );
                CHECK( dummy.oxygen == 6 );
            }
        }

        WHEN( "at 1/4 stamina" ) {
            dummy.set_stamina( dummy.get_stamina_max() / 4 );
            REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() / 4 );

            THEN( "recover 1 stamina per turn" ) {
                test_suffer( dummy, 3_turns, true );
                CHECK( dummy.oxygen == 3 );
            }
        }

        WHEN( "at 1/10 stamina" ) {
            dummy.set_stamina( dummy.get_stamina_max() / 10 );
            REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() / 10 );

            THEN( "recover 1 stamina per turn" ) {
                test_suffer( dummy, 3_turns, true );
                CHECK( dummy.oxygen == 3 );
            }
        }

        WHEN( "at 0 stamina" ) {
            dummy.set_stamina( 0 );
            REQUIRE( dummy.get_stamina() == 0 );

            THEN( "recover 1 stamina per turn" ) {
                test_suffer( dummy, 3_turns, true );
                CHECK( dummy.oxygen == 3 );
            }
        }
    }

    GIVEN( "character is underwater" ) {
        dummy.set_underwater( true );
        REQUIRE( dummy.is_underwater() );
        REQUIRE( dummy.oxygen == 46 );
        REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() );

        THEN( "they lose 1 oxygen per turn" ) {
            test_suffer( dummy, 10_turns, true );
            CHECK( dummy.oxygen == 36 );
        }
    }

    GIVEN( "character is grabbed" ) {
        REQUIRE( dummy.oxygen == 46 );
        REQUIRE( !dummy.is_underwater() );
        REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() );

        spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::east );
        spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::west );
        dummy.add_effect( effect_grabbed, 20_turns, body_part_torso, false, 2, true );
        REQUIRE( dummy.has_effect( effect_grabbed, body_part_torso ) );
        WHEN( "two grabbers" ) {

            THEN( "they lose 0 or 1 oxygen per turn" ) {
                test_suffer( dummy, 10_turns, true );
                CHECK( dummy.oxygen == Approx( 41 ).margin( 5 ) );
            }
        }

        WHEN( "four grabbers" ) {
            spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::north );
            spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::south );
            THEN( "they lose 1 oxygen per turn" ) {
                test_suffer( dummy, 10_turns, true );
                CHECK( dummy.oxygen == 36 );
            }
        }

        WHEN( "six grabbers" ) {
            spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::north );
            spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::south );
            spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::north_west );
            spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::south_west );
            THEN( "they lose 1 or 2 oxygen per turn" ) {
                test_suffer( dummy, 10_turns, true );
                CHECK( dummy.oxygen == Approx( 31 ).margin( 5 ) );
            }
        }

        WHEN( "eight grabbers" ) {
            spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::north );
            spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::south );
            spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::north_west );
            spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::south_west );
            spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::north_east );
            spawn_test_monster( "mon_debug_memory", dummy.pos_bub() + tripoint::south_east );
            THEN( "they lose 2 oxygen per turn" ) {
                test_suffer( dummy, 10_turns, true );
                CHECK( dummy.oxygen == 26 );
            }
        }

        map &here = get_map();
        WHEN( "crushed against two walls by two grabbers" ) {
            here.ter_set( dummy.pos_bub() + tripoint::south, ter_t_rock_wall );
            here.ter_set( dummy.pos_bub() + tripoint::north, ter_t_rock_wall );
            REQUIRE( here.impassable( dummy.pos_bub() + tripoint::south ) );
            REQUIRE( here.impassable( dummy.pos_bub() + tripoint::north ) );

            THEN( "they lose 1 oxygen per turn, just like four grabbers" ) {
                test_suffer( dummy, 10_turns, true );
                CHECK( dummy.oxygen == 36 );
            }
        }
    }
}
