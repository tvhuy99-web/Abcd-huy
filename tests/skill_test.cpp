#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "player_helpers.h"
#include "skill.h"

TEST_CASE( "skill_rust_occurs", "[character][skill]" )
{
    Character &guy = get_player_character();
    clear_avatar();


    for( const Skill &skill : Skill::skills ) {
        INFO( skill.ident().str() );
        guy.set_skill_level( skill.ident(), 2 );
        REQUIRE( guy.get_skill_level( skill.ident() ) == 2.f );
        REQUIRE( guy.get_knowledge_level( skill.ident() ) == 2 );
        REQUIRE_FALSE( guy.get_skill_level_object( skill.ident() ).isRusty() );
    }


    for( int i = 0; i < to_seconds<int>( 3_days ); ++i ) {
        calendar::turn += 1_seconds;
        guy.update_body();
        if( calendar::once_every( 4_hours ) ) {

            guy.set_stored_kcal( guy.get_healthy_kcal() );

            guy.environmental_revert_effect();

            guy.set_sleep_deprivation( 0 );
        }
    }


    for( const Skill &skill : Skill::skills ) {
        INFO( skill.ident().str() );

        CHECK( guy.get_skill_level( skill.ident() ) < 2.f );
        CHECK( guy.get_skill_level( skill.ident() ) > 1.5f );
        CHECK( guy.get_knowledge_level( skill.ident() ) == 2 );
        CHECK( guy.get_skill_level_object( skill.ident() ).isRusty() );
    }
}
