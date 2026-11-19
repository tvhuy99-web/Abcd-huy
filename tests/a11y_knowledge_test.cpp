#include <cstddef>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "a11y_fixtures.h"
#include "a11y_perception.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "magic.h"
#include "pimpl.h"
#include "point.h"
#include "rng.h"
#include "string_id.h"













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

}

TEST_CASE( "a11y_knowledge_pair_publishes_identical_text_for_different_secrets",
           "[a11y][knowledge]" )
{
    const a11y_fixture::knowledge_pair_fixture a =
        a11y_fixture::build_knowledge_pair( a11y_fixture::knowledge_variant::a );
    const std::vector<std::string> published_a = published_descriptions( a );

    const a11y_fixture::knowledge_pair_fixture b =
        a11y_fixture::build_knowledge_pair( a11y_fixture::knowledge_variant::b );
    const std::vector<std::string> published_b = published_descriptions( b );


    CHECK( a.monster_pos != b.monster_pos );
    CHECK( a.hidden_trap_pos != b.hidden_trap_pos );
    CHECK( a.mine_pos != b.mine_pos );
    CHECK( a.crate_content != b.crate_content );




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


    rng_set_engine_seed( 2301 );
    constexpr int sample_count = 8;
    std::vector<int> expected;
    expected.reserve( sample_count );
    for( int i = 0; i < sample_count; i++ ) {
        expected.push_back( rng( 0, 1000000 ) );
    }

    rng_set_engine_seed( 2301 );
    for( int i = 0; i < 100; i++ ) {
        ( void )a11y::describe_tile( a11y::observe_tile( fx.monster_pos ) );
        ( void )a11y::describe_tile( a11y::observe_tile( fx.crate_pos ) );
        ( void )a11y::describe_tile( a11y::observe_memory_tile( fx.crate_pos ) );
    }
    std::vector<int> after_queries;
    after_queries.reserve( sample_count );
    for( int i = 0; i < sample_count; i++ ) {
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


    CHECK_FALSE( who.magic->knows_spell( stage1.known_spell ) );
    CHECK( who.magic->knows_spell( stage2.known_spell ) );
    CHECK( stage1.known_marker_pos != stage2.known_marker_pos );
}
