#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_id.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "damage.h"
#include "effect.h"
#include "effect_source.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "monster.h"
#include "npc.h"
#include "player_helpers.h"
#include "point.h"
#include "stomach.h"
#include "type_id.h"

class Creature;

static const damage_type_id damage_cut( "cut" );

static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_debugged( "debugged" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_intensified( "intensified" );
static const efftype_id effect_max_effected( "max_effected" );
static const efftype_id effect_test_fatalism( "test_fatalism" );
static const efftype_id effect_test_int_remove( "test_int_remove" );
static const efftype_id effect_test_vitamineff( "test_vitamineff" );

static const itype_id itype_test_vitfood( "test_vitfood" );

static const mtype_id pseudo_debug_mon( "pseudo_debug_mon" );

static const vitamin_id vitamin_test_vitv( "test_vitv" );
static const vitamin_id vitamin_test_vitx( "test_vitx" );














static void check_effect_init( const std::string &eff_name,
                               const time_duration &requested_dur, const time_duration &expected_dur,
                               const std::string &bp_name, const bool permanent,
                               const int requested_int, const int expected_int,
                               const time_point &start_time )
{
    const efftype_id eff_id( eff_name );
    const effect_type &type = eff_id.obj();
    const bodypart_str_id bp( bp_name );

    effect effect_obj( effect_source::empty(), &type, requested_dur, bp, permanent, requested_int,
                       start_time );

    CHECK( expected_dur == effect_obj.get_duration() );
    CHECK( bp.id() == effect_obj.get_bp() );
    CHECK( permanent == effect_obj.is_permanent() );
    CHECK( expected_int == effect_obj.get_intensity() );
    CHECK( start_time == effect_obj.get_start_time() );
}

TEST_CASE( "effect_initialization_test", "[effect][init]" )
{


    check_effect_init( "debugged", 1_days, 1_hours, "head", false, 5, 5, calendar::turn_zero );
    check_effect_init( "debugged", 1_minutes, 1_minutes, "head", false, 5, 5, calendar::turn_zero );

    check_effect_init( "bite", 60_minutes, 60_minutes, "torso", true, 99, 2, calendar::turn );
    check_effect_init( "bite", 30_minutes, 30_minutes, "torso", true, 99, 1, calendar::turn );
    check_effect_init( "grabbed", 1_turns, 1_turns, "arm_r", false, 100, 100,
                       calendar::turn + 1_hours );
    check_effect_init( "grabbed", 100_turns, 100_turns, "arm_r", false, 1, 1,
                       calendar::turn + 1_hours );
    check_effect_init( "grabbed", 1_turns, 1_turns, "arm_r", false, 10, 10, calendar::turn + 1_hours );
}











TEST_CASE( "effect_duration", "[effect][duration]" )
{

    effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 1_turns,
                         bodypart_str_id::NULL_ID(),
                         false, 1, calendar::turn );


    REQUIRE( eff_debugged.get_duration() == 1_turns );
    REQUIRE( eff_debugged.get_max_duration() == 1_hours );

    SECTION( "duration can be set to a negative value" ) {
        eff_debugged.set_duration( -1_turns );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == -1 );
    }

    SECTION( "set_duration is capped at maximum duration" ) {
        eff_debugged.set_duration( 2_hours );
        CHECK( eff_debugged.get_duration() == 1_hours );
    }












    SECTION( "set_duration modifies intensity if effect is duration-based" ) {
        effect eff_intense( effect_source::empty(), &effect_intensified.obj(), 1_turns,
                            bodypart_str_id::NULL_ID(),
                            false, 1, calendar::turn );
        REQUIRE( eff_intense.get_int_dur_factor() == 1_minutes );


        eff_intense.set_duration( 0_seconds );
        CHECK( eff_intense.get_intensity() == 1 );


        eff_intense.set_duration( 1_minutes );
        CHECK( eff_intense.get_intensity() == 1 );


        eff_intense.set_duration( 2_minutes );
        CHECK( eff_intense.get_intensity() == 2 );


        eff_intense.set_duration( 2_minutes + 1_seconds );
        CHECK( eff_intense.get_intensity() == 3 );


        eff_intense.set_duration( 3_minutes );
        CHECK( eff_intense.get_intensity() == 3 );


        eff_intense.set_duration( 4_minutes );
        CHECK( eff_intense.get_intensity() == 3 );
    }
}













TEST_CASE( "effect_intensity", "[effect][intensity]" )
{
    effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 3_turns,
                         bodypart_str_id::NULL_ID(),
                         false, 1, calendar::turn );

    REQUIRE( eff_debugged.get_intensity() == 1 );
    REQUIRE( eff_debugged.get_max_intensity() == 10 );

    SECTION( "intensity cannot be set less than 0" ) {
        eff_debugged.set_intensity( 0 );
        CHECK( eff_debugged.get_intensity() == 0 );
        eff_debugged.set_intensity( -1 );
        CHECK( eff_debugged.get_intensity() == 0 );
    }

    SECTION( "intensity cannot be set greater than maximum" ) {

        eff_debugged.set_intensity( 11 );
        CHECK( eff_debugged.get_intensity() == 10 );

        eff_debugged.set_intensity( 9 );
        eff_debugged.mod_intensity( 2 );
        CHECK( eff_debugged.get_intensity() == 10 );
    }
}

TEST_CASE( "effect_intensity_removal", "[effect][intensity]" )
{
    effect eff_test_int_remove( effect_source::empty(), &effect_test_int_remove.obj(), 3_turns,
                                bodypart_str_id::NULL_ID(),
                                false, 1, calendar::turn );

    REQUIRE( eff_test_int_remove.get_intensity() == 1 );
    REQUIRE( eff_test_int_remove.get_int_decay_remove() == false );

    SECTION( "effects protected from intensity-based removal can't be set to less than 1 intensity" ) {
        eff_test_int_remove.set_intensity( 0 );
        CHECK( eff_test_int_remove.get_intensity() == 1 );
    }

}

TEST_CASE( "max_effective_intensity", "[effect][max][intensity]" )
{
    effect eff_maxed( effect_source::empty(), &effect_max_effected.obj(), 3_turns,
                      bodypart_str_id::NULL_ID(),
                      false, 1, calendar::turn );

    REQUIRE( eff_maxed.get_intensity() == 1 );
    REQUIRE( eff_maxed.get_max_effective_intensity() == 6 );
    REQUIRE( eff_maxed.get_max_intensity() == 10 );

    SECTION( "scaling_mods apply up to max_effective_intensity" ) {

        eff_maxed.set_intensity( 1 );
        CHECK( eff_maxed.get_mod( "INT" ) == 0 );

        eff_maxed.set_intensity( 2 );
        CHECK( eff_maxed.get_mod( "INT" ) == 1 );
        eff_maxed.set_intensity( 3 );
        CHECK( eff_maxed.get_mod( "INT" ) == 2 );
        eff_maxed.set_intensity( 4 );
        CHECK( eff_maxed.get_mod( "INT" ) == 3 );
        eff_maxed.set_intensity( 5 );
        CHECK( eff_maxed.get_mod( "INT" ) == 4 );
        eff_maxed.set_intensity( 6 );
        CHECK( eff_maxed.get_mod( "INT" ) == 5 );

        eff_maxed.set_intensity( 7 );
        CHECK( eff_maxed.get_mod( "INT" ) == 5 );
        eff_maxed.set_intensity( 8 );
        CHECK( eff_maxed.get_mod( "INT" ) == 5 );
        eff_maxed.set_intensity( 9 );
        CHECK( eff_maxed.get_mod( "INT" ) == 5 );
        eff_maxed.set_intensity( 10 );
        CHECK( eff_maxed.get_mod( "INT" ) == 5 );
    }
}





TEST_CASE( "effect_decay", "[effect][decay]" )
{
    std::vector<efftype_id> rem_ids;
    std::vector<bodypart_id> rem_bps;

    SECTION( "decay reduces effect duration by 1 turn and triggers intensity decay" ) {
        effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 2_turns,
                             bodypart_str_id::NULL_ID(),
                             false, 5, calendar::turn );

        REQUIRE( to_turns<int>( eff_debugged.get_duration() ) == 2 );
        REQUIRE_FALSE( eff_debugged.is_permanent() );


        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == 1 );

        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );


        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == 0 );

        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );


        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == 0 );

        CHECK( rem_ids.size() == 1 );
        CHECK( rem_bps.size() == 1 );

        CHECK( rem_ids.front() == eff_debugged.get_id() );
        CHECK( rem_bps.front() == bodypart_id( "bp_null" ) );
    }

    SECTION( "decay does not reduce paused/permanent effect duration" ) {
        effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 2_turns,
                             bodypart_str_id::NULL_ID(),
                             true, 1, calendar::turn );

        REQUIRE( to_turns<int>( eff_debugged.get_duration() ) == 2 );
        REQUIRE( eff_debugged.is_permanent() );


        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == 2 );
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == 2 );
    }

    SECTION( "intensity decay triggers on the appropriate turns" ) {
        effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 1_hours,
                             bodypart_str_id::NULL_ID(),
                             false, 10, calendar::turn );


        REQUIRE( eff_debugged.get_max_duration() == 1_hours );
        REQUIRE( eff_debugged.get_intensity() == 10 );
        REQUIRE( eff_debugged.get_int_decay_step() == -1 );
        REQUIRE( eff_debugged.get_int_decay_tick() == 2 );
        REQUIRE( eff_debugged.get_int_decay_remove() == true );

        calendar::turn = calendar::start_of_cataclysm;


        CHECK( eff_debugged.get_duration() == eff_debugged.get_max_duration() );
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( eff_debugged.get_intensity() == 10 );

        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );




        calendar::turn += 1_turns;
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( eff_debugged.get_intensity() == 10 );

        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );


        calendar::turn += 1_turns;
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( eff_debugged.get_intensity() == 9 );

        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );


        calendar::turn += 1_turns;
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( eff_debugged.get_intensity() == 9 );

        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );


        eff_debugged.set_intensity( 1 );
        calendar::turn += 1_turns;
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( eff_debugged.get_intensity() == 0 );

        CHECK( rem_ids.size() == 1 );
        CHECK( rem_bps.size() == 1 );

        REQUIRE( !rem_ids.empty() );
        CHECK( rem_ids.front() == eff_debugged.get_id() );
        REQUIRE( !rem_ids.empty() );
        CHECK( rem_bps.front() == bodypart_id( "bp_null" ) );

    }

    SECTION( "int_decay_remove == false protects an effect from removal" ) {
        effect eff_test_int_remove( effect_source::empty(), &effect_test_int_remove.obj(), 3_turns,
                                    bodypart_str_id::NULL_ID(),
                                    false, 3, calendar::turn );

        REQUIRE( eff_test_int_remove.get_int_decay_step() == -2 );
        REQUIRE( eff_test_int_remove.get_int_decay_tick() == 1 );
        REQUIRE( eff_test_int_remove.get_int_decay_remove() == false );

        REQUIRE( to_turns<int>( eff_test_int_remove.get_duration() ) == 3 );
        REQUIRE_FALSE( eff_test_int_remove.is_permanent() );


        eff_test_int_remove.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_test_int_remove.get_duration() ) == 2 );
        CHECK( eff_test_int_remove.get_intensity() == 1 );

        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );


        calendar::turn += 1_turns;
        eff_test_int_remove.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_test_int_remove.get_duration() ) == 1 );
        CHECK( eff_test_int_remove.get_intensity() == 1 );

        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );


        calendar::turn += 1_turns;
        eff_test_int_remove.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_test_int_remove.get_duration() ) == 0 );
        CHECK( eff_test_int_remove.get_intensity() == 1 );

        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );


        calendar::turn += 1_turns;
        eff_test_int_remove.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_test_int_remove.get_duration() ) == 0 );
        CHECK( eff_test_int_remove.get_intensity() == 1 );

        CHECK( rem_ids.size() == 1 );
        CHECK( rem_bps.size() == 1 );

        REQUIRE( !rem_ids.empty() );
        CHECK( rem_ids.front() == eff_test_int_remove.get_id() );
        REQUIRE( !rem_ids.empty() );
        CHECK( rem_bps.front() == bodypart_id( "bp_null" ) );

    }
}





TEST_CASE( "display_short_description", "[effect][desc]" )
{
    effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 1_turns, body_part_arm_r,
                         false, 1,
                         calendar::turn );



    CHECK( eff_debugged.disp_short_desc( false ) == "You have been debugged!\n"
           "Everything is working perfectly now." );

    CHECK( eff_debugged.disp_short_desc( true ) == "You have been debugged!\n"
           "Everything is working perfectly now." );
}
















TEST_CASE( "effect_display_and_speed_name_may_vary_with_intensity",
           "[effect][intensity][disp_name][speed_name]" )
{
    GIVEN( "effect with names for each intensity level" ) {



        effect eff_intense( effect_source::empty(), &effect_intensified.obj(), 1_turns,
                            bodypart_str_id::NULL_ID(), false, 1, calendar::turn );
        REQUIRE( eff_intense.get_max_intensity() == 3 );


        const effect_type *etype_intense = eff_intense.get_effect_type();
        REQUIRE( etype_intense->use_name_ints() );

        THEN( "disp_name has the name for the current intensity" ) {
            eff_intense.set_intensity( 1 );
            CHECK( eff_intense.disp_name() == "Whoa" );
            eff_intense.set_intensity( 2 );
            CHECK( eff_intense.disp_name() == "Wut?" );
            eff_intense.set_intensity( 3 );
            CHECK( eff_intense.disp_name() == "Wow!" );
        }



        THEN( "get_speed_name has the name for the current intensity" ) {
            eff_intense.set_intensity( 1 );
            CHECK( eff_intense.get_speed_name() == "Whoa" );
            eff_intense.set_intensity( 2 );
            CHECK( eff_intense.get_speed_name() == "Wut?" );
            eff_intense.set_intensity( 3 );
            CHECK( eff_intense.get_speed_name() == "Wow!" );
        }
    }

    GIVEN( "effect with a single name and explicit speed_name" ) {



        effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 1_minutes,
                             bodypart_str_id::NULL_ID(), false, 1, calendar::turn );

        THEN( "disp_name has the name, and current intensity if > 1" ) {
            eff_debugged.set_intensity( 1 );
            CHECK( eff_debugged.disp_name() == "Debugged" );
            eff_debugged.set_intensity( 2 );
            CHECK( eff_debugged.disp_name() == "Debugged [2]" );
            eff_debugged.set_intensity( 3 );
            CHECK( eff_debugged.disp_name() == "Debugged [3]" );
        }

        THEN( "get_speed_name is the speed name for all intensity levels" ) {
            eff_debugged.set_intensity( 1 );
            CHECK( eff_debugged.get_speed_name() == "Optimized" );
            eff_debugged.set_intensity( 2 );
            CHECK( eff_debugged.get_speed_name() == "Optimized" );
            eff_debugged.set_intensity( 3 );
            CHECK( eff_debugged.get_speed_name() == "Optimized" );
        }
    }
}







TEST_CASE( "effect_permanence", "[effect][permanent]" )
{

    effect eff_grabbed( effect_source::empty(), &effect_grabbed.obj(), 1_turns, body_part_arm_r, false,
                        1,
                        calendar::turn );
    CHECK_FALSE( eff_grabbed.is_permanent() );

    eff_grabbed.pause_effect();
    CHECK( eff_grabbed.is_permanent() );

    eff_grabbed.pause_effect();
    CHECK( eff_grabbed.is_permanent() );

    eff_grabbed.unpause_effect();
    CHECK_FALSE( eff_grabbed.is_permanent() );

    eff_grabbed.unpause_effect();
    CHECK_FALSE( eff_grabbed.is_permanent() );
}






TEST_CASE( "effect_body_part", "[effect][bodypart]" )
{

    effect eff_grabbed( effect_source::empty(), &effect_grabbed.obj(), 1_turns, body_part_arm_r, false,
                        1,
                        calendar::turn );
    CHECK( eff_grabbed.get_bp() == body_part_arm_r.id() );

    eff_grabbed.set_bp( body_part_arm_l );
    CHECK( eff_grabbed.get_bp() == body_part_arm_l.id() );
    CHECK_FALSE( eff_grabbed.get_bp() == body_part_arm_r.id() );

    eff_grabbed.set_bp( body_part_arm_r );
    CHECK( eff_grabbed.get_bp() == body_part_arm_r.id() );
    CHECK_FALSE( eff_grabbed.get_bp() == body_part_arm_l.id() );
}










TEST_CASE( "effect_modifiers", "[effect][modifier]" )
{
    SECTION( "base_mods apply equally for any intensity" ) {
        effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 1_minutes,
                             bodypart_str_id::NULL_ID(),
                             false, 1, calendar::turn );

        CHECK( eff_debugged.get_mod( "STR" ) == 1 );
        CHECK( eff_debugged.get_mod( "DEX" ) == 2 );
        CHECK( eff_debugged.get_mod( "PER" ) == 3 );
        CHECK( eff_debugged.get_mod( "INT" ) == 4 );
        CHECK( eff_debugged.get_amount( "HEAL_RATE" ) == 2 );
        CHECK( eff_debugged.get_amount( "HEAL_HEAD" ) == 200 );
        CHECK( eff_debugged.get_amount( "HEAL_TORSO" ) == 150 );
    }


    SECTION( "scaling_mods vary based on intensity" ) {
        effect eff_intense( effect_source::empty(), &effect_intensified.obj(), 1_turns,
                            bodypart_str_id::NULL_ID(),
                            false, 1, calendar::turn );
        REQUIRE( eff_intense.get_max_intensity() == 3 );


        eff_intense.set_intensity( 1 );
        CHECK( eff_intense.get_mod( "INT" ) == 0 );
        CHECK( eff_intense.get_mod( "PER" ) == 0 );
        eff_intense.set_intensity( 2 );
        CHECK( eff_intense.get_mod( "INT" ) == -1 );
        CHECK( eff_intense.get_mod( "PER" ) == -2 );
        eff_intense.set_intensity( 3 );
        CHECK( eff_intense.get_mod( "INT" ) == -2 );
        CHECK( eff_intense.get_mod( "PER" ) == -4 );

        eff_intense.set_intensity( 4 );
        CHECK( eff_intense.get_mod( "INT" ) == -2 );
        CHECK( eff_intense.get_mod( "PER" ) == -4 );
    }
}

TEST_CASE( "bleed_effect_attribution", "[effect][bleed][monster]" )
{
    const auto spawn_npc = [&]( const point_bub_ms & p, const std::string & npc_class ) {
        const string_id<npc_template> test_guy( npc_class );
        const character_id model_id = get_map().place_npc( p, test_guy );
        g->load_npcs();

        npc *guy = g->find_npc( model_id );
        REQUIRE( guy != nullptr );
        CHECK( !guy->in_vehicle );
        return guy;
    };

    static const tripoint_bub_ms target_location{ 5, 0, 0 };
    clear_npcs();
    clear_vehicles();
    clear_map_without_vision();
    clear_avatar();
    Character &player = get_player_character();
    const damage_instance cut_damage = damage_instance( damage_cut, 50, 50 );

    GIVEN( "player and monster" ) {
        clear_npcs();
        clear_creatures();
        monster &test_monster = spawn_test_monster( "mon_zombie", target_location );
        WHEN( "when player cuts monster" ) {
            REQUIRE( test_monster.get_hp() == test_monster.get_hp_max() );
            THEN( "bleed effect gets attributed to player" ) {
                test_monster.deal_damage( player.as_character(), bodypart_str_id::NULL_ID(), cut_damage );
                const effect &bleed = test_monster.get_effect( effect_bleed );
                CHECK( test_monster.get_hp() < test_monster.get_hp_max() );
                CHECK( !bleed.is_null() );
                const Creature *bleed_source = bleed.get_source().resolve_creature();
                CHECK( bleed_source == player.as_character() );
            }
        }
        WHEN( "when player cuts npc" ) {

            npc &test_npc = *spawn_npc( player.pos_bub().xy() + point::south_west, "thug" );
            test_npc.clear_worn();
            REQUIRE( test_npc.get_hp() == test_npc.get_hp_max() );
            THEN( "bleed effect gets attributed to player" ) {
                test_npc.deal_damage( player.as_character(), body_part_torso, cut_damage );
                const effect &bleed = test_npc.get_effect( effect_bleed, body_part_torso );
                CHECK( test_npc.get_hp() < test_npc.get_hp_max() );
                CHECK( !bleed.is_null() );
                const Creature *bleed_source = bleed.get_source().resolve_creature();
                CHECK( bleed_source == player.as_character() );
            }
        }
    }
    GIVEN( "two npcs" ) {
        npc &npc_src = *spawn_npc( player.pos_bub().xy() + point::south, "bandit" );
        npc &npc_dst = *spawn_npc( player.pos_bub().xy() + point::south_east, "thug" );
        npc_dst.clear_worn();
        WHEN( "when npc_src cuts npc_dst" ) {
            REQUIRE( npc_dst.get_hp() == npc_dst.get_hp_max() );
            THEN( "bleed effect gets attributed to npc_src" ) {
                const dealt_damage_instance dealt = npc_dst.deal_damage( npc_src.as_character(), body_part_torso,
                                                    cut_damage );
                REQUIRE( dealt.total_damage() > 0 );
                const effect &bleed = npc_dst.get_effect( effect_bleed, body_part_torso );
                CHECK( npc_dst.get_hp() < npc_dst.get_hp_max() );
                CHECK( !bleed.is_null() );
                const Creature *bleed_source = bleed.get_source().resolve_creature();
                CHECK( bleed_source == npc_src.as_character() );
            }
        }
    }
}

TEST_CASE( "Vitamin_Effects", "[effect][vitamins]" )
{
    Character &subject = get_avatar();
    clear_avatar();
    subject.stomach.empty();
    subject.guts.empty();
    subject.clear_effects();


    const efftype_id vits = effect_test_vitamineff;
    const vitamin_id vitv = vitamin_test_vitv;
    const vitamin_id vitx = vitamin_test_vitx;

    effect vitamin_effect( effect_source::empty(), &( *vits ), 5_hours, body_part_torso, false, 1,
                           calendar::turn );
    subject.add_effect( vitamin_effect );


    item food1( itype_test_vitfood );
    item food2( food1 );



    subject.vitamin_set( vitv, 0 );
    REQUIRE( subject.vitamin_get( vitv ) == 0 );
    REQUIRE( subject.vitamin_get( vitx ) == 0 );


    subject.consume( food1 );
    for( time_duration turn = 0_turns; turn < 5_hours; turn += 1_turns ) {
        calendar::turn += 1_turns;
        subject.update_body();
        subject.process_effects();
    }
    const int posteffect_vitv = subject.vitamin_get( vitv );
    const int posteffect_vitx = subject.vitamin_get( vitx );


    clear_avatar();
    REQUIRE( subject.vitamin_get( vitv ) == 0 );
    REQUIRE( subject.vitamin_get( vitx ) == 0 );
    subject.consume( food2 );
    for( time_duration turn = 0_turns; turn < 5_hours; turn += 1_turns ) {
        calendar::turn += 1_turns;
        subject.update_body();
        subject.process_effects();
    }
    const int post_vitv = subject.vitamin_get( vitv );
    const int post_vitx = subject.vitamin_get( vitx );


    CHECK( posteffect_vitx == Approx( 22 ).margin( 3 ) );
    CHECK( post_vitx == 46 );


    CHECK( posteffect_vitv == 120 );
    CHECK( post_vitv == 0 );
}

static void test_deadliness( const effect &applied, const int expected_dead, const int margin )
{
    creature_tracker &creatures = get_creature_tracker();
    clear_map_without_vision();
    std::vector<monster *> mons;


    for( int i = 0; i < 10; ++i ) {
        for( int j = 0; j < 10; ++j ) {
            tripoint_bub_ms cursor( i + 20, j + 20, 0 );

            mons.push_back( g->place_critter_at( pseudo_debug_mon, cursor ) );

            CHECK( creatures.creature_at<Creature>( cursor ) != nullptr );
        }
    }

    for( monster *const mon : mons ) {
        mon->add_effect( applied );
    }


    for( int i = 0; i < 5; ++i ) {
        calendar::turn += 1_turns;
        for( monster *const mon : mons ) {
            mon->process_effects();
        }
    }


    int alive = 0;
    for( int i = 0; i < 10; ++i ) {
        for( int j = 0; j < 10; ++j ) {
            tripoint_bub_ms cursor( i + 20, j + 20, 0 );

            alive += creatures.creature_at<Creature>( cursor ) != nullptr;
        }
    }

    const int dead = 100 - alive;
    CHECK( dead == Approx( expected_dead ).margin( margin ) );
}

TEST_CASE( "Death_Effects", "[effect][death]" )
{
    effect placebo_effect( effect_source::empty(), &( *effect_test_fatalism ), 5_seconds,
                           body_part_torso, false, 1,
                           calendar::turn );
    effect deadly_effect( effect_source::empty(), &( *effect_test_fatalism ), 5_seconds,
                          body_part_torso, false, 2,
                          calendar::turn );
    effect fatal_effect( effect_source::empty(), &( *effect_test_fatalism ), 5_seconds, body_part_torso,
                         false, 3,
                         calendar::turn );

    test_deadliness( placebo_effect, 0, 0 );


    test_deadliness( deadly_effect, 50, 25 );
    test_deadliness( fatal_effect, 100, 0 );
}
