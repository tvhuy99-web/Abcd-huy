#include <string>

#include "a11y_body.h"
#include "avatar.h"
#include "bodypart.h"
#include "cata_catch.h"
#include "flag.h"
#include "item.h"
#include "player_helpers.h"
#include "type_id.h"

// Written during the coding phase. These require real character/item data;
// execute them with the full suite only after the coding phase is complete.
TEST_CASE( "reader_health_does_not_disclose_hp_within_one_visual_band", "[a11y][medical]" )
{
    clear_avatar();
    avatar &who = get_avatar();
    const bodypart_id head( "head" );
    const int maximum = who.get_part_hp_max( head );
    REQUIRE( maximum >= 50 );
    const int first = maximum * 81 / 100;
    const int second = maximum * 85 / 100;
    REQUIRE( first != second );
    who.set_part_hp_cur( head, first );
    const std::string before = a11y::limb_health_description( who, head );
    CHECK( who.get_part_hp_cur( head ) == first );
    who.set_part_hp_cur( head, second );
    CHECK( a11y::limb_health_description( who, head ) == before );
    CHECK( who.get_part_hp_cur( head ) == second );
}

TEST_CASE( "reader_does_not_describe_a_destroyed_limb_as_a_full_health_bar", "[a11y][medical]" )
{
    clear_avatar();
    avatar &who = get_avatar();
    const bodypart_id arm( "arm_l" );
    const std::string healthy = a11y::limb_health_description( who, arm );
    who.set_part_hp_cur( arm, 0 );
    REQUIRE( who.is_limb_broken( arm ) );
    CHECK( a11y::limb_health_description( who, arm ) == "Broken." );
    CHECK( a11y::limb_health_description( who, arm ) != healthy );
    CHECK( who.get_part_hp_cur( arm ) == 0 );
}

TEST_CASE( "reader_respects_unknown_mending_progress_with_pain_immunity", "[a11y][medical]" )
{
    clear_avatar();
    avatar &who = get_avatar();
    const bodypart_id arm( "arm_l" );
    who.toggle_trait( trait_id( "NOPAIN" ) );
    REQUIRE( who.has_flag( json_character_flag( "PAIN_IMMUNE" ) ) );
    who.set_part_hp_cur( arm, 0 );
    who.wear_item( item( itype_id( "arm_splint" ) ) );
    REQUIRE( who.worn_with_flag( flag_SPLINT, arm ) );
    CHECK( a11y::limb_health_description( who, arm ) == "Splinted. Mending progress is unknown." );
    CHECK( who.get_part_hp_cur( arm ) == 0 );
}
