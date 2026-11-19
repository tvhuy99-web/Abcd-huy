#include <map>
#include <string>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "player_helpers.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "type_id.h"








static const itype_id itype_cotton_patchwork( "cotton_patchwork" );
static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_string_6( "string_6" );
static const itype_id itype_test_knotted_string_ball( "test_knotted_string_ball" );
static const itype_id itype_test_multitool( "test_multitool" );
static const itype_id itype_test_rag_bundle( "test_rag_bundle" );

static time_point midday = calendar::turn_zero + 12_hours;


static Character &setup_uncraft_character()
{
    Character &they = get_player_character();
    clear_avatar();
    clear_map_without_vision();
    set_time( midday );

    they.worn.wear_item( they, item( itype_debug_backpack ), false, false );
    they.i_add( item( itype_test_multitool ) );
    return they;
}



static std::map<itype_id, int> repeat_uncraft( Character &they, const itype_id &dis_itype,
        const recipe &dis_recip, int repeat = 1, int damage = 0 )
{

    std::map<itype_id, int> yield_items;


    item_location it_loc;
    item_location it_dis_loc;

    for( int rep = 0; rep < repeat; ++rep ) {

        item_location it = they.i_add( item( dis_itype ) );
        if( damage > 0 ) {
            it->set_damage( damage );
        }
        it_dis_loc = they.create_in_progress_disassembly( it );


        clear_map_without_vision();

        set_time( midday );

        they.complete_disassemble( it_dis_loc, dis_recip );


        for( item &it : get_map().i_at( they.pos_bub() ) ) {
            if( yield_items.count( it.typeId() ) == 0 ) {
                yield_items[it.typeId()] = it.count();
            } else {
                yield_items[it.typeId()] += it.count();
            }
        }
    }

    return yield_items;
}



static int uncraft_yield( Character &they, const itype_id whole_itype, const itype_id part_itype,
                          const int difficulty, const int skill_level )
{
    recipe uncraft_recipe = recipe_dictionary::get_uncraft( whole_itype );
    uncraft_recipe.difficulty = difficulty;
    they.set_skill_level( uncraft_recipe.skill_used, skill_level );

    std::map<itype_id, int> yield = repeat_uncraft( they, whole_itype, uncraft_recipe, 1 );
    return yield[part_itype];
}


































TEST_CASE( "uncraft_difficulty_and_character_skill", "[uncraft][difficulty][skill]" )
{
    Character &they = setup_uncraft_character();


    const itype_id decon_it = itype_test_knotted_string_ball;
    const itype_id part_it = itype_string_6;

    std::map<itype_id, int> yield;

    SECTION( "uncraft recipe with difficulty 0" ) {

        CHECK( uncraft_yield( they, decon_it, part_it, 0, 0 ) == 1000 );
        CHECK( uncraft_yield( they, decon_it, part_it, 0, 1 ) == 1000 );
        CHECK( uncraft_yield( they, decon_it, part_it, 0, 2 ) == 1000 );
        CHECK( uncraft_yield( they, decon_it, part_it, 0, 3 ) == 1000 );
        CHECK( uncraft_yield( they, decon_it, part_it, 0, 4 ) == 1000 );
    }

    constexpr int margin = 55;

    SECTION( "uncraft recipe with difficulty 1" ) {
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 0 ) == Approx( 830 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 1 ) == Approx( 1000 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 2 ) == Approx( 1000 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 3 ) == Approx( 1000 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 4 ) == Approx( 1000 ).margin( margin ) );
    }

    SECTION( "uncraft recipe with difficulty 5" ) {
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 0 ) == Approx( 20 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 1 ) == Approx( 700 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 2 ) == Approx( 990 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 3 ) == Approx( 1000 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 4 ) == Approx( 1000 ).margin( margin ) );
    }

    SECTION( "uncraft recipe with difficulty 10" ) {
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 0 ) == Approx( 0 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 1 ) == Approx( 30 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 2 ) == Approx( 500 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 3 ) == Approx( 930 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 4 ) == Approx( 1000 ).margin( margin ) );
    }
}


TEST_CASE( "uncraft_from_a_damaged_item", "[uncraft][damage]" )
{
    Character &they = setup_uncraft_character();

    recipe uncraft_rags = recipe_dictionary::get_uncraft( itype_test_rag_bundle );
    REQUIRE( uncraft_rags.difficulty == 0 );

    struct uncraft_test_preset {
        const int damage;
        const int damage_level;
        const int yield;
        const int margin;
    };




    const int margin = 60;
    const std::vector<uncraft_test_preset> presets = {
        {    0, 0, 1000,      0 },
        {  999, 1,  800, margin },
        { 1999, 2,  640, margin },
        { 2999, 3,  512, margin },
        { 3999, 4,  410, margin },
        { 4000, 5,  328, margin },
    };

    for( const uncraft_test_preset &preset : presets ) {
        CAPTURE( preset.damage, preset.damage_level, preset.yield, preset.margin );

        item test_item( itype_test_rag_bundle, calendar::turn );
        test_item.set_damage( preset.damage );
        CHECK( test_item.damage_level() == preset.damage_level );

        const int yield = repeat_uncraft( they, itype_test_rag_bundle, uncraft_rags, 10, preset.damage )
                          .at( itype_cotton_patchwork );
        CHECK( yield == Approx( preset.yield ).margin( preset.margin ) );
    }
}
