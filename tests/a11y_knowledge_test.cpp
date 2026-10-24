#include <string>
#include <vector>

#include "a11y_fixtures.h"
#include "a11y_perception.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "magic.h"
#include "player_helpers.h"
#include "rng.h"

// Written during the coding phase (R22.2). Integration oracles over the
// F-01/F-15 fixtures (tests/a11y_fixtures.h):
//  * Knowledge boundary (T22/T24/T25/T30): what the reader publishes for a
//    tile must be a function of what the character knows. Two worlds with
//    identical knowledge but different secrets must publish identical
//    descriptions, while the recorded ground truth differs.
//  * Query purity (T21, plan invariant 15.3.1): observing and describing
//    tiles must not consume game time or RNG and must not move the avatar.
//  * Lifecycle isolation (T28): after a session replacement wipe, none of
//    the previous life's knowledge may remain.
// Execute with the full suite only after the coding phase is complete.

namespace
{

std::vector<std::string> published_descriptions( const a11y_fixture::knowledge_pair_fixture &fx )
{
    std::vector<std::string> out;
    for( const tripoint_bub_ms &pos : { fx.monster_pos, fx.hidden_trap_pos, fx.mine_pos, fx.crate_pos } ) {
        out.push_back( a11y::describe_tile( a11y::observe_tile( pos ) ) );
    }
    return out;
}

} // namespace

TEST_CASE( "a11y_knowledge_pair_publishes_identical_text_for_different_secrets",
           "[a11y][knowledge]" )
{
    const a11y_fixture::knowledge_pair_fixture a =
        a11y_fixture::build_knowledge_pair( a11y_fixture::knowledge_variant::a );
    const std::vector<std::string> published_a = published_descriptions( a );

    const a11y_fixture::knowledge_pair_fixture b =
        a11y_fixture::build_knowledge_pair( a11y_fixture::knowledge_variant::b );
    const std::vector<std::string> published_b = published_descriptions( b );

    // Ground truth really differs between the two worlds...
    CHECK( a.monster_pos != b.monster_pos );
    CHECK( a.hidden_trap_pos != b.hidden_trap_pos );
    CHECK( a.mine_pos != b.mine_pos );
    CHECK( a.crate_content != b.crate_content );
    // ...while everything the reader publishes about those tiles is the
    // same, position by position (monster tile, trap tile, mine tile,
    // closed crate). Any difference here is an information leak, not a
    // wording preference.
    REQUIRE( published_a.size() == published_b.size() );
    for( std::size_t i = 0; i < published_a.size(); i++ ) {
        CHECK( published_a[i] == published_b[i] );
    }
}

TEST_CASE( "a11y_tile_queries_do_not_consume_time_moves_or_rng", "[a11y][knowledge]" )
{
    const a11y_fixture::knowledge_pair_fixture fx =
        a11y_fixture::build_knowledge_pair( a11y_fixture::knowledge_variant::a );
    avatar &who = get_avatar();
    const tripoint_bub_ms pos_before = who.pos_bub();
    const time_point turn_before = calendar::turn;

    // Record the RNG stream the engine would produce from a fixed seed.
    rng_set_engine_seed( 2301 );
    std::vector<int> expected;
    for( int i = 0; i < 8; i++ ) {
        expected.push_back( rng( 0, 1000000 ) );
    }

    rng_set_engine_seed( 2301 );
    for( int i = 0; i < 100; i++ ) {
        ( void )a11y::describe_tile( a11y::observe_tile( fx.monster_pos ) );
        ( void )a11y::describe_tile( a11y::observe_tile( fx.crate_pos ) );
        ( void )a11y::describe_tile( a11y::observe_memory_tile( fx.crate_pos ) );
    }
    std::vector<int> after_queries;
    for( int i = 0; i < 8; i++ ) {
        after_queries.push_back( rng( 0, 1000000 ) );
    }

    CHECK( after_queries == expected );
    CHECK( calendar::turn == turn_before );
    CHECK( who.pos_bub() == pos_before );
}

TEST_CASE( "a11y_lifecycle_stages_do_not_share_knowledge", "[a11y][knowledge]" )
{
    const a11y_fixture::lifecycle_stage_fixture stage1 =
        a11y_fixture::build_lifecycle_stage( 1 );
    avatar &who = get_avatar();
    REQUIRE( who.magic->knows_spell( stage1.known_spell ) );

    const a11y_fixture::lifecycle_stage_fixture stage2 =
        a11y_fixture::build_lifecycle_stage( 2 );
    // After the replacement wipe, stage 1's spell is gone and only stage
    // 2's knowledge is present -- the in-process half of T28's isolation.
    CHECK_FALSE( who.magic->knows_spell( stage1.known_spell ) );
    CHECK( who.magic->knows_spell( stage2.known_spell ) );
    CHECK( stage1.known_marker_pos != stage2.known_marker_pos );
}
